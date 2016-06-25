local rect = {0,0,99999,99999,0,0}
local rects = {}
local bitmapcache = {}


-- helper functions
function GetPath(pathname)
	if pathname == nil then return end
	local t = string.reverse(pathname)
	t = string.sub(t, string.find(t, "\\") or string.find(t, "/") or 1)
	return string.reverse(t)
end
function GetName(pathname)
	if pathname == nil then return end
	local t = string.reverse(pathname)
	t = string.sub(t, 1, ((string.find(t, "\\") or string.find(t, "/")  or t:len())-1))
	return string.reverse(t)
end
local lua_file = core.loading_file
local lua_path = GetPath(lua_file)

function debug_print(...)
	--print("--DEBUG", ...)
end

function info(...)
	--print("--INFO", ...)
end

function error(...)
	print("--ERROR:", ...)
end


function BeginChild(left, top, right, bottom, alpha)
	local left_unclip, top_unclip = left, top
	left = math.max(rect[1], left)
	top = math.max(rect[2], top)
	right = math.min(rect[3], right)
	bottom = math.min(rect[4], bottom)
	local new_rect = {left, top, right, bottom, left_unclip, top_unclip, alpha}
	table.insert(rects, rect)
	rect = new_rect

	dx9.set_clip_rect_core(left, top, right, bottom)
end

function EndChild(left, top, right, bottom)
	rect = table.remove(rects)
	dx9.set_clip_rect_core(rects[1], rects[2], rects[3], rects[4])
end

function IsCurrentDrawingVisible()
	return rect[3]>rect[1] and rect[4]>rect[2]
end


-- global window or dshow or etc callback
function OnDirectshowEvents(event_code, param1, param2)
	print("OnDirectshowEvents", event_code, param1, param2)

	local EC_COMPLETE = 1
	if event_code == EC_COMPLETE then
		if playlist:current_pos() >= playlist:count() then
			player.stop()
			player.seek(0)
		else
			playlist:next()
		end
	end
end
function OnMove() end
function OnSize() end

-- pre render
local last_pre_render_time = 0
function PreRender(view)
	local delta_time = 0;
	if last_pre_render_time > 0 then delta_time = core.GetTickCount() - last_pre_render_time end
	last_pre_render_time = core.GetTickCount();
	if view ==0 then root:BroadCastEvent("PreRender", last_pre_render_time, delta_time) end
end

-- the Main Render function
local last_render_time = 0
function RenderUI(view)
	local delta_time = 0;
	if last_render_time > 0 then delta_time = core.GetTickCount() - last_render_time end
	last_render_time = core.GetTickCount();
	local t = core.GetTickCount()
	if view == 0 then root:BroadCastEvent("PreRenderUI", last_render_time, delta_time) end
	local dt = core.GetTickCount() -t
	t = core.GetTickCount()

	root:render(view)

	local dt2 = core.GetTickCount() -t

	if dt > 0 or dt2 > 0 then
		info(string.format("slow RenderUI() : PreRender() cost %dms, render() cost %dms", dt, dt2))
	end
end

local last_update_time = 0
function UpdateUI()
	local delta_time = 0;
	if last_render_time > 0 then delta_time = core.GetTickCount() - last_render_time end
	root:BroadCastEvent("OnUpdate", last_update_time, delta_time)
	last_update_time = core.GetTickCount()
end

-- resource base class
local resource_base ={}

function resource_base:create(handle, width, height)
	local o = {handle = handle, width = width, height = height}
	setmetatable(o, self)
	self.__index = self	
	
	return o
end

function resource_base:commit()
	return dx9.commit_resource_core(self.handle)
end

function resource_base:decommit()
	return dx9.decommit_resource_core(self.handle)
end

function resource_base:release()
	return dx9.release_resource_core(self.handle)
end


-- GPU resource management
function OnInitCPU()
	debug_print("OnInitCPU")
	root:BroadCastEvent("OnInitCPU", last_render_time, delta_time)
	-- load resources here (optional)
end

function OnInitGPU()
	debug_print("OnInitGPU")
	root:BroadCastEvent("OnInitGPU", last_render_time, delta_time)
	root:BroadCastEvent("OnLayoutChange")

	-- commit them to GPU (optional)
	-- handle resize changes here (must)
end

function OnReleaseGPU()
	-- decommit all GPU resources (must)
	debug_print("OnReleaseGPU")
	root:BroadCastEvent("OnReleaseGPU", last_render_time, delta_time)
	releaseCache(true)
end

function OnReleaseCPU()
	-- release all resources (must)
	debug_print("OnReleaseCPU")
	root:BroadCastEvent("OnReleaseCPU", last_render_time, delta_time)
	releaseCache(false)
end

function releaseCache(is_decommit)
	if is_decommit then
		for _,v in pairs(bitmapcache) do
			dx9.decommit_resource_core(v.handle)
		end
	else
		for _,v in pairs(bitmapcache) do
			dx9.release_resource_core(v.handle)
		end
		bitmapcache = {}
	end
end

function test_get_text_bitmap(...)
	local res, width, height = dx9.draw_font_core(...)		-- width is also used as error msg output.
	if not res then
		error(width, filename)
		return
	end
	return resource_base:create(res, width, height)
end

DrawText = test_get_text_bitmap

function get_bitmap(filename, reload)
	if reload then unload_bitmap(filename) end
	if bitmapcache[filename] == nil then
		local res, width, height = dx9.load_bitmap_core(filename)		-- width is also used as error msg output.

		bitmapcache[filename] = resource_base:create(res, width, height)
		bitmapcache[filename].filename = filename
		if not res then
			error(width, filename)
			bitmapcache[filename].width = nil
			bitmapcache[filename].error_msg = width
		end
		--print("loaded", filename, width, height)
	end

	-- reset ROI
	local rtn = bitmapcache[filename]
	rtn.left = 0
	rtn.right = 0
	rtn.top = 0
	rtn.bottom = 0
	return rtn
end

function unload_bitmap(filename, is_decommit)
	local tbl = bitmapcache[filename]
	if not tbl or type(tbl)~="table" then return end

	if is_decommit then
		dx9.decommit_resource_core(bitmapcache[filename].handle)
	else
		dx9.release_resource_core(bitmapcache[filename].handle)
		bitmapcache[filename] = nil
	end
end

-- set region of interest
function set_bitmap_rect(bitmap, left, top, right, bottom)
	if not bitmap or "table" ~= type(bitmap) then return end
	bitmap.left = left
	bitmap.right = right
	bitmap.top = top
	bitmap.bottom = bottom
end

-- paint
bilinear_mipmap_minus_one = 0
lanczos = 1
bilinear_no_mipmap = 2
lanczos_onepass = 3
bilinear_mipmap = 4

function paint(left, top, right, bottom, bitmap, alpha, resampling_method)
	if not bitmap or not bitmap.handle then return end
	local x,y  = rect[5], rect[6]
	local a = alpha or 1.0
	return dx9.paint_core(left+x, top+y, right+x, bottom+y, bitmap.handle, bitmap.left, bitmap.top, bitmap.right, bitmap.bottom, a, resampling_method or bilinear_no_mipmap)
end

-- native threading support
Thread = {}

function Thread:Create(func, ...)

	local o = {}
	setmetatable(o, self)
	self.__index = self
	self.handle = core.CreateThread(func, ...)

	return o
end

-- these two native method is not usable because it will keep root state machine locked and thus deadlock whole lua state machine
function Thread:Resume()
	return core.ResumeThread(self.handle)
end

function Thread:Suspend()
	return core:SuspendThread(self.handle)
end

--function Thread:Terminate(exitcode)
--	return core.TerminateThread(self.handle, exitcode or 0)
--end

function Thread:Wait(timeout)
	return core.WaitForSingleObject(self.handle, timeout)
end

function Thread:Sleep(timeout)		-- direct use of core.Sleep() is recommended
	return core.Sleep(timeout)
end

-- helper functions and settings
function merge_table(op, tomerge)
	for k,v in pairs(tomerge) do
		if type(v) == "table" and type(op[k]) == "table" then
			merge_table(op[k], v)
		else
			op[k] = v
		end
	end
end

function ReloadUI(legacy)
	print("ReloadUI()", legacy, root)
	OnReleaseGPU()
	OnReleaseCPU()
	root = BaseFrame:Create()

	if legacy then return end

	print(core.execute_luafile(lua_path .. (core.v and "3dvplayer" or "DWindow2" ).. "\\render.lua"))
	--print(core.execute_luafile(lua_path .. "Tetris\\Tetris.lua"))
	--v3dplayer_add_button()
end

function set_setting(name, value)
	setting[name] = value
	core.ApplySetting(name)
end

function apply_adv_settings()
	player.set_output_channel(setting.AudioChannel)
	player.set_normalize_audio(setting.NormalizeAudio > 1)
	player.set_input_layout(setting.InputLayout)
	player.set_mask_mode(setting.MaskMode)
	player.set_output_mode(setting.OutputMode)
	player.set_topmost(setting.Topmost)
end

function format_table(t, level)
	level = level or 1
	leveld1 = level > 1 and level -1 or 0
	local o = {}
	local lead = ""
	for i=1,level do
		lead = "\t" .. lead
	end
	
	local leadd1 = ""
	for i=1,leveld1 do
		leadd1 = "\t" .. leadd1
	end

	table.insert(o, leadd1 .. "{")
	for k,v in pairs(t) do
		local vv
		if type(v) == "table" then
			vv = format_table(v, (level or 0) + 1)
		elseif type(v) == "string" then
			v = string.gsub(v, "\\", "\\\\")
			v = string.gsub(v, "\r", "\\r")
			v = string.gsub(v, "\n", "\\n")
			vv = "\"" .. v .."\""
		else
			vv = tostring(v)
		end
		
		if type(k) == "string" then
			k = string.gsub(k, "\\", "\\\\")
			k = string.gsub(k, "\r", "\\r")
			k = string.gsub(k, "\n", "\\n")
			k = "[\"" .. k .."\"]"
			table.insert(o, string.format("%s = %s,", k, vv))
		else
			table.insert(o, string.format("%s,", vv))
		end
	end
	
	return "\r\n" .. table.concat(o, "\r\n"..lead) .."\r\n".. leadd1 .. "}"
end

function core.save_settings()
	print("SAVING SETTINGS")
	
	return core.WriteConfigFile("tmpsetting = " .. format_table(setting) .. "\r\nmerge_table(setting, tmpsetting)\r\n")
end

function core.load_settings()
	print("LOADING SETTINGS")
	
	return core.execute_luafile(core.GetConfigFile())
end

function core.set_setting(key, value)
	setting[key] = value
	core.ApplySetting(key)
end

function core.get_setting(key)
	return setting[key]
end

-- load base_frame
if core and core.execute_luafile then
	print(core.execute_luafile(lua_path .. "base_frame.lua"))
	print(core.execute_luafile(lua_path .. "3dvplayer.lua"))
	print(core.execute_luafile(lua_path .. "Container.lua"))
	print(core.execute_luafile(lua_path .. "playlist.lua"))
	print(core.execute_luafile(lua_path .. "menu.lua"))
	print(core.execute_luafile(lua_path .. "default_setting.lua"))
	core.load_settings();
	print(core.execute_luafile(lua_path .. "language.lua"))
end

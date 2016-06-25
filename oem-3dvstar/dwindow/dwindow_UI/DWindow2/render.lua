local lua_file = core.loading_file
local lua_path = GetPath(lua_file)
core.execute_luafile(lua_path .. "menu.lua")

local alpha = 0.5
local UI_fading_time = 300
local UI_show_time = 2000
local playlist_list
local playlist_button = BaseFrame:Create()
local playlist_close = BaseFrame:Create()
local font = dx9.CreateFont({name="宋体", height = 12, weight=0})
local font_black = dx9.CreateFont({name="宋体", height = 16, weight=0})

setting.MAX_WIDTH = 417
setting.MAX_HEIGHT = 277
core.ApplySetting("MAX_WIDTH")
core.ApplySetting("MAX_HEIGHT")
player.set_window_text(L("DWindow"), L("DWindow"))



-- black background and right mouse reciever
local oroot = BaseFrame:Create()
root:AddChild(oroot)
function oroot:OnInitGPU(t, dt)
	self:SetSize(ui.width, ui.height)
end

function oroot:OnMouseDown(x, y, button)
	if button ~= VK_RBUTTON and not player.is_fullscreen() then
		ui.StartDragging()
	end	
	playlist_list:hide()
	return true
end

tttt = 0
function oroot:OnClick(x,y,button)
	if button == VK_RBUTTON then
		if tttt % 2 == 1 then
			--player.popup_menu()
		else
			--player.popup_menu()
			--popup3dvstar();
		end
		popup_dwindow2()
		tttt = tttt + 1
	end
	return true
end


-- background
local logo = BaseFrame:Create()
oroot:AddChild(logo)
logo:SetPoint(CENTER)
logo:SetSize(1024,600)

function logo:RenderThis()
	if not player.movie_loaded then
		local res = get_bitmap(lua_path .. "bg.png")
		paint(0,0,1024,600, res)
	end
end

-- bottom bar
local bottombar = BaseFrame:Create()
oroot:AddChild(bottombar)
bottombar:SetPoint(BOTTOMLEFT)
bottombar:SetPoint(BOTTOMRIGHT)
bottombar:SetHeight(44)

function bottombar:RenderThis()
	local res = get_bitmap(lua_path .. "bar.png")
	paint(0,0,ui.width,44, res)
end


-- top bar
local topbar = BaseFrame:Create()
oroot:AddChild(topbar)
topbar:SetPoint(TOPLEFT)
topbar:SetPoint(TOPRIGHT)
topbar:SetHeight(30)
topbar.item = " "

function topbar:PreRender()
	local left, right = playlist:current_item();
	local dis = GetName(left) or " "
	if right then dis = dis .. " + " .. GetName(right) end
	if not player.movie_loaded then dis = " " end
	if self.item ~= dis then
		self.item = dis
		
		if self.res then
			self.res:release()
		end
		self.res = DrawText(self.item, font_black)
	end
end

function topbar:RenderThis()
	local res = get_bitmap(lua_path .. "bar.png")
	paint(0,0,ui.width,34, res)
	
	if self.res then 
		local h = (34-self.res.height)/2
		paint(15, h, 15+self.res.width, h+self.res.height, self.res)
	end
end

-- playlist button
bottombar:AddChild(playlist_button)
playlist_button:SetPoint(BOTTOMLEFT, bottombar, BOTTOMLEFT, 14, -7)
playlist_button:SetSize(36,30)

function playlist_button:RenderThis()
	paint(0,0,36,30, get_bitmap(lua_path .. (self:IsMouseOver() and "playlist_mouseover.png" or "playlist_normal.png")))	
end

function playlist_button:OnClick()
	-- TODO: show playlist
	if playlist_list.showing then
		playlist_list:hide()
	else
		playlist_list:show()
	end
	
	return true
end

function playlist_button:HitTest(x, y)
	return x >= 10 and y>= 8 and x<= 25 and y <= 21
end


-- setting_buttons button
local setting_button = BaseFrame:Create()
bottombar:AddChild(setting_button)
setting_button:SetPoint(LEFT, playlist_button, RIGHT, -18, 0)
setting_button:SetSize(36,30)

function setting_button:RenderThis()
	paint(0,0,36,30, get_bitmap(lua_path .. (self:IsMouseOver() and "setting_mouseover.png" or "setting_normal.png")))	
end

function setting_button:HitTest(x, y)
	return x >= 10 and y>= 8 and x<= 25 and y <= 21
end

function setting_button:OnClick()
	player.show_setting()
	return true
end

-- 3d/2d button
local b3d = BaseFrame:Create()
bottombar:AddChild(b3d)
b3d:SetPoint(LEFT, setting_button, RIGHT, -18, 0)
b3d:SetSize(36,30)

function b3d:RenderThis()
	if not player.movie_loaded then return end
	
	if dx9.is2DMovie() then
		paint(0,0,36,30, get_bitmap(lua_path .. (self:IsMouseOver() and "2d_mouseover.png" or "2d_normal.png")))	
	else
		paint(0,0,36,30, get_bitmap(lua_path .. (dx9.is2DRendering() and "3d_normal.png" or "3d_mouseover.png")))	
	end
end

function b3d:HitTest(x, y)
	return x >= 10 and y>= 8 and x<= 25 and y <= 21
end

function b3d:OnClick()
	if not player.movie_loaded then return end
	player.toggle_3d()
	return true
end

-- LR button
local LR = BaseFrame:Create()
bottombar:AddChild(LR)
LR:SetPoint(LEFT, b3d, RIGHT, -18, 0)
LR:SetSize(36,30)

function LR:RenderThis()
	if not player.movie_loaded or dx9.is2DMovie() then return end
	
	if setting.SwapEyes then
		paint(0,0,36,30, get_bitmap(lua_path .. (self:IsMouseOver() and "RL_mouseover.png" or "RL_normal.png")))
	else
		paint(0,0,36,30, get_bitmap(lua_path .. (self:IsMouseOver() and "LR_mouseover.png" or "LR_normal.png")))			
	end
end

function LR:HitTest(x, y)
	return x >= 10 and y>= 8 and x<= 25 and y <= 21
end

function LR:OnClick()
	if not player.movie_loaded or dx9.is2DMovie() then return end
	player.set_swapeyes(not setting.SwapEyes)
	return true
end


-- fullscreen button
local full = BaseFrame:Create()
bottombar:AddChild(full)
full:SetPoint(BOTTOMRIGHT, bottombar, nil, -14, -7)
full:SetSize(33,31)

function full:RenderThis()
	paint(0,0,33,31, get_bitmap(lua_path .. (self:IsMouseOver() and "fullscreen_mouseover.png" or "fullscreen.png")))
end

function full:OnClick()
	player.toggle_fullscreen()
	return true
end


-- play/pause button
local play = BaseFrame:Create()
bottombar:AddChild(play)
play:SetPoint(BOTTOM)
play:SetSize(109,38)

function play:RenderThis()
	if player.is_playing() then
		paint(0,0,109,38, get_bitmap(lua_path .. (self:IsMouseOver() and "pause_mousedown.png" or "pause_mouseover.png")))	
	else
		paint(0,0,109,38, get_bitmap(lua_path .. (self:IsMouseOver() and "play_mousedown.png" or "play_mouseover.png")))	
	end
end

function play:OnClick()
	if not player.movie_loaded then
		if playlist:count() > 0 then
			playlist:play_item(playlist:current_pos())
		else
			local file = ui.OpenFile()
			if file then
				playlist:play(file)
			end
		end
	else
		player.pause()
	end
	
	return true
end


-- progress bar
local progress = BaseFrame:Create()
oroot:AddChild(progress)
progress:SetPoint(TOPLEFT, bottombar, nil, 0, -8)
progress:SetPoint(TOPRIGHT, bottombar, nil, 0, -8)
progress:SetHeight(16+6)

function progress:RenderThis()
	local v = (player.tell() or 0) / (player.total() or 1)
	v = math.max(math.min(v, 1), 0)
	local l,_,r = self:GetRect()
	
	paint(0,8,r-l, 8+(self:IsMouseOver() and 6 or 2), get_bitmap(lua_path .. "progress_bg.png"))
	paint(0,8,(r-l)*v,8+6, get_bitmap(lua_path .. (self:IsMouseOver() and "progress_thick.png" or "progress.png")))
end

function progress:OnMouseMove(x,y,button)
	if not self.dragging then return end
	local l,_,r=self:GetRect()
	local w = r-l
	local v = x/w
	v = math.max(0,math.min(v,1))
	
	player.seek(player.total()*v)
end

function progress:OnMouseUp(x, y, button)
	self.dragging = false;
end

function progress:OnMouseDown(...)
	self.dragging = true
	self:OnMouseMove(...)
	
	return true
end


-- progress slider
local progress_slider = BaseFrame:Create()
oroot:AddChild(progress_slider)
progress_slider:SetSize(17, 17)
progress_slider:SetPoint(CENTER, progress, LEFT)

function progress_slider:RenderThis()
	if progress:IsMouseOver() then
		paint(0,0,17, 17, get_bitmap(lua_path .. "slider.png"))
	end
end

function progress_slider:PreRender()
	local v = (player.tell() or 0) / (player.total() or 1)
	v = math.max(math.min(v, 1), 0)
	local l,_,r = progress:GetRect()
	self:SetPoint(CENTER, progress, LEFT, (r-l)*v, 0.5)
end

function progress_slider:HitTest()
	return false
end


-- volume bar
-- enlarged hittest width
local volume = BaseFrame:Create()
bottombar:AddChild(volume)
volume:SetPoint(RIGHT, full, LEFT, -20+9, 0)
volume:SetSize(68+18, 16+4)

function volume:RenderThis()
	local v = math.max(math.min(player.get_volume(), 1), 0)
	local l,_,r = self:GetRect()
	
	paint(9,8,9+r-l-18, 8+4, get_bitmap(lua_path .. "volume_bg.png"))
	paint(9,8,9+(r-l-18)*v,8+4, get_bitmap(lua_path .. "volume.png"))
end

function volume:OnMouseMove(x,y,button)
	if not self.dragging then return end
	local l,_,r=self:GetRect()
	local w = r-l-18
	local v = (x-9)/w
	v = math.max(0,math.min(v,1))
	
	player.set_volume(v)
end

function volume:OnMouseUp(x, y, button)
	self.dragging = false;
end

function volume:OnMouseDown(...)
	self.dragging = true
	self:OnMouseMove(...)
	
	return true
end


-- volume slider
local volume_slider = BaseFrame:Create()
local volume_slider = BaseFrame:Create()
oroot:AddChild(volume_slider)
volume_slider:SetSize(17, 17)
volume_slider:SetPoint(CENTER, volume, LEFT, 9)

function volume_slider:RenderThis()
	paint(0,0,17, 17, get_bitmap(lua_path .. "slider.png"))
end

function volume_slider:HitTest()
	return false;
end

function volume_slider:PreRender()
	local l,_,r = volume:GetRect()
	self:SetPoint(CENTER, volume, LEFT, 9+(r-l-18)*player.get_volume(), 0.5)
end


-- volume button
local volume_button = BaseFrame:Create()
bottombar:AddChild(volume_button)
volume_button:SetPoint(RIGHT, volume, LEFT, 5, 0)
volume_button:SetSize(25, 24)

function volume_button:RenderThis()
	paint(0,0,25, 24, get_bitmap(lua_path .. (player.get_volume() > 0 and "volume_button_normal.png" or "volume_button_mute.png")))
end

function volume_button:OnClick()
	if player.get_volume() > 0 then
		self.saved_volume = player.get_volume()
		player.set_volume(0)
	else
		player.set_volume(self.saved_volume or 1)
		self.saved_volume = nil
	end
end


-- open file button
local open = BaseFrame:Create()
oroot:AddChild(open)
open:SetPoint(CENTER)
open:SetSize(165,36)

function open:RenderThis()
	if not player.movie_loaded then
		paint(0,0,165,36, get_bitmap(lua_path .. "open.png"))
		local x = math.floor((165-self.caption.width)/2)
		local y = math.floor((36-self.caption.height)/2)
		paint(x,y,x+self.caption.width, y+self.caption.height, self.caption)
	end
end

function open:OnInitGPU()
	local font = dx9.CreateFont({name="宋体", weight=0, height=13})
	self.caption = DrawText(L("Open File..."), font, 0x00ffff)
	dx9.ReleaseFont(font)
end

function open:OnReleaseGPU()
	if (self.caption) then
		self.caption:release()
		self.caption = nil
	end	
end

function open:OnLanguageChange()
	dx9.lock_frame()
	self:OnReleaseGPU()
	self:OnInitGPU()
	dx9.unlock_frame()
end

function open:HitTest()
	return not player.movie_loaded
end

function open:OnClick()
	local file = ui.OpenFile()
	if file then
		playlist:play(file)
	end

	return true
end

-- playlist items
local playlist_item = BaseFrame:Create()
function playlist_item:Create(text)
	local o = BaseFrame:Create()
	setmetatable(o, self)
	self.__index = self
	o:SetHeight(28)
	o.text = text
	
	return o
end

function playlist_item:OnReleaseGPU()
	if self.res then
		self.res:release()
	end
	self.res = nil
end

function playlist_item:RenderThis()

	self.res = self.res or DrawText(self.text, font, 0x1a6eb0)
	local l,t,r,b = self:GetRect()
	l, t, r, b = 0, 0, r-l, b-t
	
	if self.id == playlist:current_pos() then
		paint(l,t,r,b, get_bitmap(lua_path .. "menu_item.png"))
	end
	
	if self.res and self.res.width and self.res.height then
		local dy = (28-self.res.height)/2
		local dx = 10
		paint(dx,dy, dx+self.res.width, dy+self.res.height, self.res, 1, bilinear_mipmap_minus_one)
	end

end

function playlist_item:OnDoubleClick()
	playlist:play_item(self.id)
	
	return true
end

function playlist_item:OnClick(x, y, button)
	if button ~= VK_RBUTTON then return end
	local m = 
	{
		{
			string = "删除该条目",
			id = self.id,
			on_command = function(v)
				playlist:remove_item(v.id)
			end
		},
		{
			string = "播放该条目",
			id = self.id,
			on_command = function(v)
				playlist:play_item(v.id)
			end
		},
		{
			string = "清空",
			id = self.id,
			on_command = function(v)
				playlist:clear()
			end
		},
	}
	
	m = menu_builder:Create(m)
	m:popup()
	m:destroy()
	return true
end
-- the "playlist"
playlist_list = BaseFrame:Create()
oroot:AddChild(playlist_list)
playlist_list.dragging = false
setting.playlist_width = 180
playlist_list:SetWidth(setting.playlist_width)
playlist_list:SetPoint(TOPLEFT, topbar, BOTTOMLEFT, -setting.playlist_width)
playlist_list:SetPoint(BOTTOMLEFT, bottombar, TOPLEFT, -setting.playlist_width)

function playlist_list:RenderThis()
	local l,t,r,b = self:GetRect()
	local res = get_bitmap(lua_path .. "blue.bmp")
	--paint(0,0,r-l,b-t, )
	paint(0, 0, r-l, b-t, get_bitmap(lua_path .. "playlist_bg.png"))
	paint(0, 0, r-l, 2, res)
	--paint(0, 0, 2, b-t, res)
	paint(0, b-t-2, r-l, b-t, res)
	paint(r-l-2, 0, r-l, b-t, res)
end

function playlist_list:PreRender(t, dt)
	self.pos = self.pos or 0
	local pre_pos = self.pos
	local da = dt/UI_fading_time
	if self.showing then
		self.pos = self.pos + da
	else
		self.pos = self.pos - da
	end
	
	self.pos = math.max(0, self.pos)
	self.pos = math.min(1, self.pos)
	
	if self.pos ~= pre_pos then
		local l,_,r = self:GetRect()
		playlist_list:SetPoint(TOPLEFT, topbar, BOTTOMLEFT, -setting.playlist_width *(1 - self.pos) )
		playlist_list:SetPoint(BOTTOMLEFT, bottombar, TOPLEFT, -setting.playlist_width *(1 - self.pos) )
	end
end

function playlist_list:OnMouseDown(x, y)
	if x > setting.playlist_width - 5 then
		self.dragging = true
	end
	
	return true
end

function playlist_list:OnPlaylistChange()
	print("playlist_list:OnPlaylistChange()")
	self:RemoveAllChilds()
	
	for i=1, playlist:count() do
		local item = playlist:item(i)
		local text = GetName(item.L) or " "
		if item.R then text = text .. " + " .. GetName(item.R) end
		if text == "" then text = item.L end
		
		local obj = playlist_item:Create(text)
		obj.id = i
		self:AddChild(obj)
		if i == 1 then
			obj:SetPoint(TOPLEFT)
			obj:SetPoint(TOPRIGHT)
		else
			local last = self:GetChild(i-1)
			obj:SetPoint(TOPLEFT, last, BOTTOMLEFT)
			obj:SetPoint(TOPRIGHT, last, BOTTOMRIGHT)
		end
	end
	
	playlist_list:AddChild(playlist_close)
end

function playlist_list:OnMouseMove(x, y)
	if self.dragging then
		local l = self:GetRect()
		local x = ui.get_mouse_pos()
		setting.playlist_width = (x - l)
		setting.playlist_width = math.max(10, setting.playlist_width)
		
		self:SetWidth(setting.playlist_width)
	end
end

function playlist_list:OnMouseUp()
	self.dragging = false
	return true
end

function playlist_list:hide()
	if self.dragging then return end
	self.showing = false
end

function playlist_list:show()
	if self.dragging then return end
	self.showing = true
end

-- load the playlist now
playlist_list:OnPlaylistChange()

-- the close button
playlist_close:SetSize(30,30)
playlist_close:SetPoint(RIGHT)

function playlist_close:RenderThis()
	paint(0, 0, 30, 30, get_bitmap(lua_path .. "playlist_close.png"))
end

function playlist_close:OnClick()
	playlist_list:hide()
	
	return true
end


-- open file button drop down
local open2 = BaseFrame:Create()
oroot:AddChild(open2)
open2:SetPoint(LEFT, open, RIGHT)
open2:SetSize(31,36)

function open2:RenderThis()
	if not player.movie_loaded then
		paint(0,0,31,36, get_bitmap(lua_path .. "open_more.png"))
	end
end

function open2:HitTest()
	return not player.movie_loaded
end

function open2:OnClick()
	local m = 
	{
		{
			string = "打开URL...",
			on_command = function()
				local url = ui.OpenURL()
				if url then
					playlist:play(url)
				end
			end
		},
		{
			string = "打开左右分离文件...",
			on_command = function()
				local left, right = ui.OpenDoubleFile()
				if left and right then
					playlist:play(left, right)
				end
			end
		},
		{
			string = "打开文件夹...",
			on_command = function()
				local folder = ui.OpenFolder()
				if folder then
					playlist:play(folder)
				end
			end
		},
	}
	
	m = menu_builder:Create(m)
	m:popup(open:GetPoint(BOTTOMLEFT))
	m:destroy()
	return true
end


-- event handler
local event = BaseFrame:Create()
oroot:AddChild(event)
function event:PreRender(t, dt)
	if player.is_fullscreen() then
		dx9.set_movie_rect(nil)
	else
		local l, t = topbar:GetPoint(BOTTOMLEFT)
		local r, b = bottombar:GetPoint(TOPRIGHT)
		dx9.set_movie_rect(l, t, r, b)
	end
end


local mouse_hider = BaseFrame:Create()
root:AddChild(mouse_hider)
local last_mousemove =  0
local mousex = -999
local mousey = -999

function mouse_hider:OnUpdate(t, dt)

	local px, py = ui.get_mouse_pos()
	if (mousex-px)*(mousex-px)+(mousey-py)*(mousey-py) > 100 or ((mousex-px)*(mousex-px)+(mousey-py)*(mousey-py) > 0 and alpha > 0.5) then	
		last_mousemove = core.GetTickCount()
		mousex, mousey = ui.get_mouse_pos()
	end
		
	local da = dt/UI_fading_time
	local old_alpha = alpha
	local mouse_in_pannel = (px>=0 and px<ui.width and py>=ui.height-44 and py<ui.height) -- bottombar
	mouse_in_pannel = mouse_in_pannel or (px>=0 and px<ui.width and py>0 and py<30)	-- topbar
	mouse_in_pannel = mouse_in_pannel or (playlist_list.showing and px >= 0 and px <= setting.playlist_width)	-- playlist
	local hide_mouse = (not mouse_in_pannel) and (t > last_mousemove + UI_show_time) and (player.is_fullscreen())
	player.show_mouse(not hide_mouse)
	
	if hide_mouse then
		alpha = alpha - da
	else
		alpha = alpha + da
	end
	alpha = math.min(1, math.max(alpha, 0))
	
	topbar:SetPoint(TOPLEFT, nil, nil, 0, -30+30*alpha)
	topbar:SetPoint(TOPRIGHT, nil, nil, 0, -30+30*alpha)
	bottombar:SetPoint(BOTTOMLEFT, nil, nil, 0, 44-44*alpha)
	bottombar:SetPoint(BOTTOMRIGHT, nil, nil, 0, 44-44*alpha)
	
	if (alpha < 0.1) then
		playlist_list:hide()
	end
	
	if (px < 5 and px >= 0 and py >= 0 and py < ui.height) then
		playlist_list:show()
	end
end
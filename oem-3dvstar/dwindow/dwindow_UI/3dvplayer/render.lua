local lua_file = core.loading_file
local lua_path = GetPath(lua_file)
volume_bar = BaseFrame:Create()

core.execute_luafile(lua_path .. "menu.lua")
setting.MAX_WIDTH = 746
setting.MAX_HEIGHT = 461
core.ApplySetting("MAX_WIDTH")
core.ApplySetting("MAX_HEIGHT")
local UI_fading_time = 300
local UI_show_time = 2000
local alpha = 1
local last_mousemove =  0
local mousex = -999
local mousey = -999
player.set_window_text("3DVPlayer", "3DVPlayer")

-- 3dvplayer UI renderer

local button_size = 40;
local margin_button_right = 32;
local margin_button_bottom = 8;
local space_of_each_button = 62;
local toolbar_height = 65;
local width_progress_left = 5;
local width_progress_right = 6;
local margin_progress_right = 522;
local margin_progress_left = 37;
local progress_height = 21;
local progress_margin_bottom = 27;
local volume_base_width = 84;
local volume_base_height = 317;
local volume_margin_right = (156 - 84);
local volume_margin_bottom = (376 - 317);
local volume_button_zero_point = 26;
local volume_bar_height = 265;
local numbers_left_margin = 21;
local numbers_right_margin = 455+62;
local numbers_width = 12;
local numbers_height = 20;
local numbers_bottom_margin = 26;
local hidden_progress_width = 72;

logo = BaseFrame:Create()
logo.name = "LOGO"
root:AddChild(logo)
logo:SetPoint(CENTER, nil, nil, 0, -20)
logo:SetSize(1920,1080)

function logo:RenderThis()
	if not player.movie_loaded then
		local res = get_bitmap(lua_path .. "logo_bg.png")
		paint(0,0,1920,1080, res)
	end
end

function logo:OnClick(x, y, key)
	if key == VK_RBUTTON then
		popup3dvstar()
	end
	
	return false
end

function logo:OnMouseDown(x, y, button)
	if button ~= VK_RBUTTON and not player.is_fullscreen() then
		ui.StartDragging()
	end	
	return true
end

function logo:OnUpdate(t, dt)

	local px, py = ui.get_mouse_pos()
	if (mousex-px)*(mousex-px)+(mousey-py)*(mousey-py) > 100 or ((mousex-px)*(mousex-px)+(mousey-py)*(mousey-py) > 0 and alpha > 0.5) then	
		last_mousemove = core.GetTickCount()
		mousex, mousey = ui.get_mouse_pos()
	end
		
	local da = dt/UI_fading_time
	local old_alpha = alpha
	local mouse_in_pannel = px>=0 and px<ui.width and py>=ui.height-toolbar_height and py<ui.height
	local hide_mouse = not mouse_in_pannel and (t > last_mousemove + UI_show_time)
	player.show_mouse(not hide_mouse)
	
	if hide_mouse then
		alpha = alpha - da
	else
		alpha = alpha + da
	end
	alpha = math.min(1, math.max(alpha, 0))	
end

local open = BaseFrame:Create()
root:AddChild(open)
open:SetPoint(CENTER, nil, nil, 0, 60)
open:SetSize(230,70)

function open:RenderThis()
	if not player.movie_loaded then
		paint(0,0,230,70, get_bitmap(lua_path .. (setting.LCID == "简体中文" and "open.png" or "open_en.png")))
	end
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


logo_hot = BaseFrame:Create()
logo_hot.name = "LOGO HOT AREA"
logo:AddChild(logo_hot)
logo_hot:SetPoint(CENTER, nil, nil, 0, -50)
logo_hot:SetSize(400,171)

function logo_hot:RenderThis()
	if not player.movie_loaded then
		local res = get_bitmap(lua_path .. "logo_hot.png")
		paint(0,0,400,171, res)
	end
end

toolbar_bg = BaseFrame:Create()
toolbar_bg.name = "toolbar_bg"
root:AddChild(toolbar_bg)
toolbar_bg:SetPoint(BOTTOMLEFT)
toolbar_bg:SetPoint(BOTTOMRIGHT)
toolbar_bg:SetSize(nil, toolbar_height)
print("toolbar_height=", toolbar_height)

function toolbar_bg:RenderThis()
	local l,t,r,b = self:GetRect()
	local res = get_bitmap(lua_path .. "toolbar_background.png");
	paint(0,0,r-l,b-t, res, alpha)
end

-- buttons
local button_pictures =
{
	"fullscreen.png", "",
	"volume.png", "",
	"next.png", "",
	"play.png", "pause.png",
	"previous.png", "",
	"stop.png", "",
	"3d.png", "2d.png",
	"setting.png", "",
}

function show_volume_bar()
	volume_bar:show()
end

function start_play_or_pause()
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
end

local button_functions = 
{
	player.toggle_fullscreen,
	show_volume_bar,
	playlist.next,
	start_play_or_pause,
	playlist.previous,
	player.reset,
	player.toggle_3d,
	player.show_setting,
}


local function button_GetRect(self)
	return 0, 0, space_of_each_button, button_size, self.dx, self.dy
end

local function button_RenderThis(self)
	paint(11,0,button_size+11,button_size, get_bitmap(lua_path .. self.pic[1]), alpha)
end

local function button_OnClick(self)
	return button_functions[self.id]()
end


local x = - margin_button_right
local y = - margin_button_bottom
local buttons = {}

for i=1,#button_pictures/2 do
	local button = buttons[i] or BaseFrame:Create()
	buttons[i] = button
	toolbar_bg:AddChild(button)
	button.pic = {button_pictures[i*2-1], button_pictures[i*2]}
	button.RenderThis = button_RenderThis
	button.OnClick = button_OnClick
	button.name = button.pic[1]
	button:SetPoint(BOTTOMRIGHT, nil, nil, x, y)
	button:SetSize(space_of_each_button, button_size)
	button.id = i

	x = x - space_of_each_button	
end

buttons[4].RenderThis = function(self)
	paint(11,0,button_size+11,button_size, get_bitmap(lua_path .. self.pic[player.is_playing() and 2 or 1]), alpha)
end


buttons[7].RenderThis = function(self)
	paint(11,0,button_size+11,button_size, get_bitmap(lua_path .. self.pic[dx9.is2DRendering() and 2 or 1]), alpha)
end


progressbar = BaseFrame:Create()
progressbar.name = "progressbar"
toolbar_bg:AddChild(progressbar)
progressbar:SetPoint(BOTTOMLEFT, nil, nil, margin_progress_left, -progress_margin_bottom+progress_height)
progressbar:SetPoint(BOTTOMRIGHT, nil, nil, -margin_progress_right, -progress_margin_bottom+progress_height)
progressbar:SetSize(nil, progress_height)

local progress_pic =
{
	"progress_left_base.png",
	"progress_center_base.png",
	"progress_right_base.png",
	"progress_left_top.png",
	"progress_center_top.png",
	"progress_right_top.png",
}

function file(n)
	return progress_pic[n]
end

function progressbar:OnClick(x)
	local l,_,r = self:GetRect()
	local fv = x/(r-l)
	player.seek(player.total()*fv)	
end

function progressbar:RenderThis()
	local l,t,r,b = self:GetRect()
	l,r,t,b = 0,r-l,0,b-t
	local fv = player.tell() / player.total()
	if fv > 1 then fv = 1 end
	if fv < 0 then fv = 0 end
	local v = fv * r
	
	-- draw bottom
	paint(0,0, width_progress_left, b, get_bitmap(lua_path .. file(1)), alpha)
	paint(width_progress_left,0, r-width_progress_right, b, get_bitmap(lua_path .. file(2)), alpha)
	paint(r-width_progress_right,0, r, b, get_bitmap(lua_path .. file(3)), alpha)

	-- draw top
	if (v > 1.5) then
		local bmp = get_bitmap(lua_path .. file(4))
		paint(0,0,math.min(width_progress_left, v),b, bmp, alpha)
	end

	if (v > width_progress_left) then
		local r = math.min(r-width_progress_right, v)
		local bmp = get_bitmap(lua_path .. file(5))
		paint(width_progress_left, 0, r, b, bmp, alpha)
	end

	if (v > r - width_progress_right) then
		local bmp = get_bitmap(lua_path .. file(6))
		paint(r-width_progress_right, 0, v, b, bmp, alpha)
	end
end

number_current = BaseFrame:Create()
number_current.name = "number_current"
toolbar_bg:AddChild(number_current)
number_current:SetPoint(BOTTOMLEFT, nil, nil, numbers_left_margin, - numbers_bottom_margin)
number_current:SetSize(numbers_width * 8, numbers_height)

function number_current:RenderThis()
	if not player.movie_loaded then return end

	local t = player.total()
	if self.name == "number_current" then t= player.tell() end
	local ms = t % 1000
	local s = t / 1000
	local h = (s / 3600) % 100
	local h1 = h/10
	local h2 = h%10
	local m1 = (s/60 /10) % 6
	local m2 = (s/60) % 10
	local s1 = (s/10) % 6
	local s2 = s%10

	local numbers =
	{
		h1, h2, 10, m1, m2, 10, s1, s2,   -- 10 = : symbol in time
	}

	local x = 0
	for i=1,#numbers do
		paint(x, 0, x+numbers_width, numbers_height, get_bitmap(lua_path .. math.floor(numbers[i]) .. ".png"), alpha)
		x = x + numbers_width
	end

end

number_total = BaseFrame:Create()
number_total.name = "number_total"
toolbar_bg:AddChild(number_total)
number_total:SetPoint(BOTTOMRIGHT, nil, nil, -numbers_right_margin, - numbers_bottom_margin)
number_total.t = 23456000
number_total:SetSize(numbers_width * 8, numbers_height)
number_total.RenderThis = number_current.RenderThis


root:AddChild(volume_bar)
volume_bar:SetPoint(BOTTOMRIGHT, nil, nil, 84-156, 317-376)
volume_bar:SetSize(84, 317)
volume_bar.alpha = 0
volume_bar.volume = player.get_volume()
function volume_bar:RenderThis()
	if self.alpha < 0.05 then return end
	paint(0,0,84,317, get_bitmap(lua_path .. "volume_base.png"), self.alpha)
	
	-- the button
	local ypos = volume_button_zero_point + (317-volume_button_zero_point*2) * (1-player.get_volume());
	paint (22, ypos-20, 62, ypos + 20, get_bitmap(lua_path .. "volume_button.png"), self.alpha)
end

function volume_bar:hide()
	self.showing = false
end

function volume_bar:show()
	self.last_show = core.GetTickCount()
	self.showing = true
end

function volume_bar:OnUpdate(t, dt)
	local da = dt/UI_fading_time
	if self.showing or self.dragging then
		self.alpha = self.alpha + da
	else
		self.alpha = self.alpha - da
	end
	
	self.alpha = math.max(0, self.alpha)
	self.alpha = math.min(1, self.alpha)	
	
	if self.volume ~= player.get_volume() then
		self:show()
		self.volume = player.get_volume()
	elseif core.GetTickCount() > (self.last_show or 0)+UI_show_time  then
		self:hide()
	end
end

function volume_bar:HitTest()
	return self.alpha > 0
end


function volume_bar:OnMouseMove(x,y,button)
	if not self.dragging then return end
	local _,t,_,b=self:GetRect()
	local h = b-t-volume_button_zero_point*2
	local v = 1-(y-volume_button_zero_point)/h
	v = math.max(0,math.min(v,1))
		
	player.set_volume(v)
end

function volume_bar:OnMouseUp(x, y, button)
	self.dragging = false;
end

function volume_bar:OnMouseDown(...)
	self.dragging = true
	self:OnMouseMove(...)
	
	return true
end






grow = BaseFrame:Create()
grow.x = 0
grow.y  = 0
local last_in_time = 0
local last_out_time = 0
local last_in = false
local alpha_tick = 0
toolbar_bg:AddChild(grow)
grow.name = "GROW"
grow:SetSize(250, 250)
grow:SetPoint(BOTTOMLEFT, nil, nil, -125, 125)

function grow:Stick(dt)
	local frame = root:GetFrameByPoint(self.px or 0, self.py or 0)
	local isbutton = false
	for _,v in ipairs(buttons) do
		if v == frame then
			isbutton = true
		end
	end
	if not isbutton then return end
	
	local l,t,r,b = frame:GetRect()
	local dx = (l+r)/2 - self.x
	self.x = self.x + 0.65 * dx
	self:SetPoint(BOTTOMLEFT, nil, nil, self.x-125, 125)
end

function grow:PreRender()
	self.px, self.py = ui.get_mouse_pos()
	local px,py = self.px,self.py
	
	local r,b = toolbar_bg:GetPoint(BOTTOMRIGHT)
	r,b = r - margin_button_right, b - margin_button_bottom
	local l,t = toolbar_bg:GetPoint(BOTTOMRIGHT)
	l,t = l - margin_progress_right, t - margin_button_bottom - button_size
	local dt = 0;
	if l<=px and px<r and t<=py and py<b then
		if not last_in then 
			self:OnEnter()
		else
			alpha_tick = alpha_tick + (core.GetTickCount() - last_in_time)+2
			dt = core.GetTickCount() - last_in_time
		end
		last_in = true
		last_in_time = core.GetTickCount()
	else
		if last_in then
			self:OnLeave()
		else
			alpha_tick = alpha_tick - (core.GetTickCount() - last_out_time)*0.8
			dt = core.GetTickCount() - last_out_time
		end
		last_in = false
		last_out_time = core.GetTickCount()
	end
	
	alpha_tick = math.max(alpha_tick, 0)
	alpha_tick = math.min(alpha_tick, 300)
	
	if dt > 0 then self:Stick(dt) end
end

function grow:OnEnter()
end

function grow:OnLeave()
end

function grow:RenderThis()
	local res = get_bitmap(lua_path .. "grow.png")
	local alpha = alpha_tick / 200
	paint(0, 0, 250, 250, res, alpha)
end

function grow:HitTest()
	return false
end

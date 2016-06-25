local lua_file = core.loading_file
local lua_path = GetPath(lua_file)
local function GetCurrentLuaPath(offset)
	return lua_path
end

local alpha = 0.5
local UI_fading_time = 500
local UI_show_time = 2000

-- toolbar background
local bg = BaseFrame:Create()
root:AddChild(bg)
bg:SetPoint(BOTTOMLEFT)
bg:SetSize(ui.width, 64)

function bg:PreRender()
	bg:SetSize(ui.width, 64)
end

function bg:RenderThis()
	local res = get_bitmap(lua_path .. "bg.png")
	paint(0,0,ui.width,64,res,alpha)
end

function bg:HitTest()
end

-- right mouse button reciever
local rbutton = BaseFrame:Create()
root:AddChild(rbutton)
rbutton:SetPoint(CENTER)
rbutton:SetSize(ui.width,ui.height)

function rbutton:PreRender()
	rbutton:SetSize(ui.width,ui.height)
end

function rbutton:OnClick(x,y,button)
	if button == VK_RBUTTON then
		alpha = 1
		player.show_mouse(true)
		player.popup_menu()
		return true
	end
end

-- LOGO
local logo = BaseFrame:Create()
rbutton:AddChild(logo)

function logo:PreRender()
	self.w = math.min(math.min(ui.width, ui.height-40)*3/5, 512)
	logo:SetPoint(CENTER, rbutton, CENTER, 0.0125*self.w,-20)
	logo:SetSize(self.w+0.025*self.w, self.w)	
end

function logo:RenderThis(view)
	local d = self.d
	if not player.movie_loaded then
		local delta = (1-view) * 0.025*self.w
		paint(0+delta,0,self.w+delta,self.w,get_bitmap(lua_path .. "logo.png"))
	end
end

function logo:HitTest(x, y)
	if player.movie_loaded then return end
	
	local r = self.w/2
	local dx = x-self.w/2
	local dy = y-self.w/2
	
	return dx*dx+dy*dy < r*r
end

function logo:OnClick()
	alpha = 1
	player.show_mouse(true)
	player.popup_menu()
	return true
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
	local mouse_in_pannel = px>0 and px<ui.width and py>ui.height-64 and py<ui.height
	local hide_mouse = not mouse_in_pannel and (t > last_mousemove + UI_show_time)
	player.show_mouse(not hide_mouse)
	
	if hide_mouse then
		alpha = alpha - da
	else
		alpha = alpha + da
	end
	alpha = math.min(1, math.max(alpha, 0))	
end

-- Play/Pause button
local play = BaseFrame:Create()
root:AddChild(play)
play:SetPoint(BOTTOMLEFT, nil, nil, 14, -8)
play:SetSize(14, 14)

function play:RenderThis()
	local res = get_bitmap(lua_path .. "ui.png")
	if player.is_playing() then
		set_bitmap_rect(res, 110,0,110+14,14)
	else
		set_bitmap_rect(res, 96,0,96+14,14)
	end
	paint(0,0,14,14,res,alpha)	
end

function play:OnClick()
	player.pause()
end

-- Fullscreen button
local full = BaseFrame:Create()
root:AddChild(full)
full:SetPoint(BOTTOMRIGHT, nil, nil, -14, -8)
full:SetSize(14, 14)

function full:RenderThis()
	local res = get_bitmap(lua_path .. "ui.png")
	set_bitmap_rect(res, 192,0,192+14,14)
	paint(0,0,14,14,res,alpha)
end

function full:OnClick()
	print("FULL")
	player.toggle_fullscreen()
end

-- Volume
local volume = BaseFrame:Create()
root:AddChild(volume)
volume:SetPoint(RIGHT, full, LEFT, -14+3, 0)
volume:SetSize(34+6,14)							--本应该是34x14, 宽松3个像素来方便0%和100%

function volume:RenderThis()
	local volume =  player.get_volume()
	volume = math.min(volume, 1)
	volume = math.max(volume, 0)
	local res = get_bitmap(lua_path .. "ui.png")
	set_bitmap_rect(res, 124+34,0,124+34+34,14)
	paint(0+3,0,34+3,14,res,alpha)
	set_bitmap_rect(res, 124,0,124+34*volume,14)
	paint(0+3,0,3+34*volume,14,res,alpha)
end

function volume:OnClick(x, y)
	v = math.min(math.max(0, (x-3)/34), 1)
	player.set_volume(v)
end

-- Previous button
local previous = BaseFrame:Create()
root:AddChild(previous)
previous:SetPoint(LEFT, play, RIGHT, 14)
previous:SetSize(14, 14)

function previous:RenderThis()
	local res = get_bitmap(lua_path .. "ui2.png")
	set_bitmap_rect(res, 0,0,14,14)
	paint(0,0,14,14,res,alpha)	
end

function previous:OnClick()
	playlist:previous()
end

-- next button
local next = BaseFrame:Create()
root:AddChild(next)
next:SetPoint(LEFT, previous, RIGHT, 14)
next:SetSize(14, 14)

function next:RenderThis()
	local res = get_bitmap(lua_path .. "ui2.png")
	set_bitmap_rect(res, 14,0,28,14)
	paint(0,0,14,14,res,alpha)	
end

function next:OnClick()
	playlist:next()
end

-- numbers
local number_current = BaseFrame:Create()
root:AddChild(number_current)
number_current:SetPoint(LEFT, next, RIGHT, 14)
number_current:SetSize(9*5+6*2,14)

function number_current:GetTime()
	return player.tell()
end

function number_current:RenderThis()
	local t = self:GetTime()
	local ms = t % 1000
	local s = t / 1000
	local h = (s / 3600) % 10
	local m1 = (s/60 /10) % 6
	local m2 = (s/60) % 10
	local s1 = (s/10) % 6
	local s2 = s%10

	local numbers =
	{
		h, -1, m1, m2, -1, s1, s2,   -- -1 = : symbol in time
	}

	local res = get_bitmap(lua_path .. "ui.png")
	local x = 0
	for i=1,#numbers do
		local tex = 6 + math.floor(numbers[i])*9
		local ctex = 9
		if tex<6 then
			tex =  0
			ctex = 6
		end
		
		set_bitmap_rect(res, tex,0,tex+ctex,14)
		paint(x, 0, x+ctex, 14, res,alpha)
		x = x + ctex
	end
end

local number_total = BaseFrame:Create()
root:AddChild(number_total)
number_total:SetPoint(RIGHT, volume, LEFT, -14+3)
number_total:SetSize(9*5+6*2,14)
number_total.RenderThis = number_current.RenderThis
function number_total:GetTime()
	return player.total()
end


-- progress bar
local progress = BaseFrame:Create()
root:AddChild(progress)
progress:SetPoint(LEFT, number_current, RIGHT, 14-3, 0)
progress:SetPoint(RIGHT, number_total, LEFT, -14, 0)
progress:SetHeight(14)

function progress:RenderThis()
	local l,_,r=self:GetRect()
	local w = r-l
	local fv = player.tell() / player.total()
	fv = math.max(0,math.min(fv,1))
	
	local res = get_bitmap(lua_path .. "ui.png")
	set_bitmap_rect(res, 216,0,220,14)
	paint(3,0,w+3,14,res,alpha)
	set_bitmap_rect(res, 208,0,212,14)
	paint(3,0,fv*w+3,14,res,alpha)
end

function progress:OnClick(x,y,button)
	local l,_,r=self:GetRect()
	local w = r-l
	local v = (x-3) / w
	v = math.max(0,math.min(v,1))
	
	player.seek(player.total()*v)
end

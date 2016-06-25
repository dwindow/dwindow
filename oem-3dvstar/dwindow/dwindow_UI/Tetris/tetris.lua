local lua_file = core.loading_file
local lua_path = GetPath(lua_file)
local function GetCurrentLuaPath(offset)
	return lua_path
end

local ww = 10
local ww = 10
local hh = 16
local typecount = 8
local max_shape_len = 5
local max_dis = -100


local CHECK_LEFT = 1
local CHECK_RIGHT = 2
local CHECK_BOTTOM = 4
local CHECK_HIT = 8

local blocks =
{
}

local posx = ww/2
local posy = 1
local shape = {}

if bit32 == nil and require then bit32 = require("bit") end

local function check()

	local out = 0
	local x,y
	for k=1,max_shape_len do
		if (shape[k].x > max_dis and shape[k].y > max_dis) then

			x=posx + shape[k].x;
			y=posy + shape[k].y;

			if (x<1 or x>ww or y<1 or y>hh) then
				out = bit32.bor(out, CHECK_HIT)
			else
				if (x<=1) then
					out = bit32.bor(out, CHECK_LEFT);
				elseif (blocks[(x-1)][y] > 0) then
					out = bit32.bor(out, CHECK_LEFT);
				end

				if (x>=ww) then
					out = bit32.bor(out, CHECK_RIGHT);
				elseif (blocks[x+1][y] > 0) then
					out = bit32.bor(out, CHECK_RIGHT);
				end

				if (y>=hh) then
					out = bit32.bor(out, CHECK_BOTTOM);
				elseif (blocks[x][y+1] > 0) then
					out = bit32.bor(out, CHECK_BOTTOM);
				end

				if (x>0 and x<=ww and y>0 and y<=hh) then
					if (blocks[x][y] > 0) then
						out = bit32.bor(out, CHECK_HIT);
					end
				end
			end
		end
	end

	return out;
end


-- init blocks
local function init()
	for x=1,ww do
		blocks[x] = {}
		for y=1,hh do
			blocks[x][y] = 0
		end
	end

	type2shape()
end


local function clone(t)
	if t == nil then return end
	local o = {}
	for k,v in ipairs(t) do
		o[k] = v
	end
	return o
end


function type2shape(type)
	if not type then type = math.random(typecount) end
	local p1 =
	{
		{{1,0}, {1,1},             {max_dis,max_dis}},		--2格竖条
		{{0,1}, {1,1},{1,0},{1,-1},{max_dis,max_dis}},		--左拐L拐条
		{{1,1}, {0,1},{0,0},{0,-1},{max_dis,max_dis}},		--右拐L拐条
		{{0,0}, {0,1},{1,1},{1,0}, {max_dis,max_dis}},		--方块
		{{0,0}, {0,1},{1,0},{0,-1},{max_dis,max_dis}},		--K条
		{{0,-1},{0,0},{0,1},{0,2}, {max_dis,max_dis}},		--4格竖长条
		{{0,-1},{0,0},{1,0},{1,1}, {max_dis,max_dis}},		--右拐Z拐条
		{{1,-1},{1,0},{0,0},{0,1}, {max_dis,max_dis}},		--左拐Z拐条
	}

	for i=1,max_shape_len do
		shape[i] = {x= (p1[type][i] or {max_dis,max_dis})[1], y = (p1[type][i] or {max_dis,max_dis})[2]}
	end
	return 0
end


local function Step()
	local r = check();
	if( bit32.band(r, CHECK_BOTTOM) > 0) then
		local x,y
		for i=1,max_shape_len do
			if (shape[i].x>max_dis or shape[i].y>max_dis) then

				x=posx + shape[i].x;
				y=posy + shape[i].y;

				blocks[x][y] = 1;

			end
		end

		-- check bottom
		local f = true;
		for j=hh,1,-1 do
			f = true;
			while f do
				for i=1,ww do
					f = f and (blocks[i][j] >0)
				end

				if f then
					for k=j,2,-1 do
						for i=1,ww do
							blocks[i][k] = blocks[i][k-1]
						end
					end
				end
			end
		end

		posy = 1;
		posx = ww/2;
		type2shape();
		r = check();
		print("check=", r)
		if bit32.band(r,CHECK_BOTTOM) > 0 then
			-- game over...
			--DrawBlocks();
			--MessageBox(0, "Game Over", "....", MB_OK);
			init();
		end
	else
		posy = posy+1;
	end

	return 0;
end

local function rotateBlock()
	-- 以(0.5,0.5)为中心旋转
	local x;

	for i=1,max_shape_len do
		if (shape[i].x>max_dis or shape[i].y>max_dis) then
			x = shape[i].x *2 -1;
			shape[i].x = (-(shape[i].y*2-1) +1 )/2;
			shape[i].y = (x + 1 )/2;
		end
	end

	--如果有碰撞，旋转回去
	local r = check();
	if bit32.band(r,CHECK_HIT) > 0 then
		for i=1,max_shape_len do
			if (shape[i].x>max_dis or shape[i].y>max_dis) then
			x = shape[i].x *2 -1;
			shape[i].x = ((shape[i].y*2-1) +1 )/2;
			shape[i].y = (-x + 1 )/2;
			end
		end
		return -1;
	end
	return 0;
end

local VK_LEFT = 0x25
local VK_UP = 0x26
local VK_RIGHT = 0x27
local VK_DOWN = 0x28

function OnKeyDown(key, id)
	print(key, id)
	local r = check()
	if key == VK_LEFT and bit32.band(r, CHECK_LEFT) == 0 then
		posx = posx - 1
	elseif key == VK_RIGHT and bit32.band(r, CHECK_RIGHT) == 0 then
		posx = posx + 1
	elseif key == VK_DOWN and bit32.band(r, CHECK_BOTTOM) == 0 then
		posy = posy + 1
	elseif key == VK_UP then
		rotateBlock()
	else
	end
end


init()

local tetris = BaseFrame:Create()
tetris.name = "TETRIS"
tetris:SetPoint(TOPLEFT)
root:AddChild(tetris)
tetris:SetSize(40*ww+40,40*hh+40)

function tetris:RenderThis()
	local res = get_bitmap(lua_path .. "fullscreen.png")
	local res2 = get_bitmap(lua_path .. "stop.png")
	set_bitmap_rect(res2, 0,0,100,100)

	-- paint border
	for i=1,hh+1 do
		paint(ww*40, (i-1)*40, (ww+1)*40, i*40, res2)
	end

	for i=1,ww do
		paint((i-1)*40, hh*40, i*40, (hh+1)*40, res2)
	end

	-- paint static blocks
	for y=1,hh do
		for x=1,ww do
			if blocks[x][y] > 0 then
				local xx = (x-1)*40
				local yy = (y-1)*40
				paint(xx,yy,xx+40,yy+40,res)
			end
		end
	end

	-- paint that shape
	for k=1,max_shape_len do
		if (shape[k].x>max_dis or shape[k].y>max_dis) then
			local i=posx + shape[k].x;
			local j=posy + shape[k].y;

			paint((i-1)*40,(j-1)*40,(i)*40,(j)*40,res)
		end
	end
end

function tetris:PreRender()
	if core.GetTickCount() - (self.tick or 0)  > 400 then
		Step()
		self.tick = core.GetTickCount()
	end
end


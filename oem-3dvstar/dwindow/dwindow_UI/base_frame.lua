-- Base Frame

LEFT = 1
RIGHT = 2
TOP = 4
BOTTOM = 8
CENTER = 0
TOPLEFT = TOP + LEFT
TOPRIGHT = TOP + RIGHT
BOTTOMLEFT = BOTTOM + LEFT
BOTTOMRIGHT = BOTTOM + RIGHT

BaseFrame ={}

function BaseFrame:Create(parent)
	local o = {}
	o.childs = {}
	o.anchors = {}
	o.layout_childs = {}		-- frames which are relatived to this frame
	o.layout_parents = {}		-- frames which this frame relatives to
	setmetatable(o, self)
	self.__index = self
	
	if parent then
		parent:AddChild(o)
	end
	
	return o
end

function BaseFrame:render(...)

	self:RenderThis(...)

	for i=1,#self.childs do
		local v = self.childs[i]
		if v and v.render then
			local l,t,r,b = v:GetRect();
			BeginChild(l,t,r,b)
			if IsCurrentDrawingVisible() then v:render(...) end
			EndChild(l,t,r,b)
		end
	end

end

-- these size / width / height is the desired values
-- and anchor points may overwite them
-- to get displayed size(and position), use GetRect()
function BaseFrame:GetSize()
	return self.width, self.height
end

function BaseFrame:SetSize(width, height)
	if self.width ~= width or self.height ~= height then
		self.width = width
		self.height = height	
		self:BroadcastLayoutEvent("OnLayoutChange");
	end
end

function BaseFrame:SetWidth(width)
	if self.width ~= width then
		self.width = width
		self:BroadcastLayoutEvent("OnLayoutChange");
	end
end

function BaseFrame:SetHeight(height)
	if self.height ~= height then
		self.height = height
		self:BroadcastLayoutEvent("OnLayoutChange");
	end
end

function BaseFrame:SetAlpha(alpha)
	self.alpha = alpha
end


-- override this function to draw your frame
function BaseFrame:RenderThis(...)
end

-- Parent and Child relationship functions
function BaseFrame:AddChild(frame, pos)
	if frame == nil then return end
	if frame.parent ~= nil then
		error("illegal AddChild(), frame already have a parent : adding " .. tostring(frame) .. " to ".. tostring(self))
		return false
	end

	frame.parent = self;
	if pos ~= nil then
		table.insert(self.childs, pos, frame)
	else
		table.insert(self.childs, frame)
	end
	
	return true
end

function BaseFrame:IsParentOf(frame, includeParentParent)
	if frame == nil then
		return false
	end
	
	if frame.parent == self then
		return true
	end
		
	if not includeParentParent then 
		return false
	else
		return self:IsParentOf(frame.parent) 
	end
end

function BaseFrame:RemoveChild(frame)
	if frame == nil then return end
	if frame.parent ~= self then		-- illegal removal
		error("illegal RemoveChild() " .. tostring(frame) .. " from ".. tostring(self))
		return false
	end

	for i=1,#self.childs do
		if self.childs[i] == frame then
			table.remove(self.childs, i)
			break;
		end
	end

	frame.parent = nil
	
	return true
end

function BaseFrame:RemoveAllChilds()
	local t = {}
	for _,v in ipairs(self.childs) do
		table.insert(t, v)
	end
	for _,v in ipairs(t) do
		v:RemoveFromParent()
	end
end

function BaseFrame:RemoveFromParent()
	if self.parent then
		self.parent:RemoveChild(self)
	else
		debug_print("illegal RemoveFromParent() : " .. tostring(self) .. " has no parent")
	end
end

function BaseFrame:GetParent()
	return self.parent
end

function BaseFrame:GetChild(n)
	return self.childs[n]
end

function BaseFrame:GetChildCount()
	return #self.childs
end

-- Layout Parent and Child relationship functions
-- caller usually don't need to call these functions
-- they are used to maintain layout relationship
function BaseFrame:AddLayoutChild(frame)
	if frame == nil then return end
	if self:IsLayoutParentOf(frame) then
		info("illegal AddLayoutChild() : the child to add is already parent of this frame " .. tostring(frame) .. " from ".. tostring(self))
		--core.track_back()
		return false
	end

	table.insert(frame.layout_parents, self)
	table.insert(self.layout_childs, frame)
	
	return true
end

function BaseFrame:IsLayoutParentOf(frame)
	if frame == nil then
		return false
	end
	
	for _,v in ipairs(frame.layout_parents) do
		if v == self then
			return true
		end
	end
	
	return false
end

function BaseFrame:RemoveLayoutChild(frame)
	if frame == nil then return end
	if not self:IsLayoutParentOf(frame) then		-- illegal removal
		error("illegal RemoveLayoutChild() " .. tostring(frame) .. " from ".. tostring(self))
		return
	end

	for i=1,#self.layout_childs do
		if self.layout_childs[i] == frame then
			table.remove(self.layout_childs, i)
			break;
		end
	end

	for i=1,#frame.layout_parents do
		if frame.layout_parents[i] == self then
			table.remove(frame.layout_parents, i)
			break;
		end
	end
end

function BaseFrame:GetLayoutParents()
	return self.layout_parents
end

function BaseFrame:GetLayoutChild(n)
	return self.layout_childs[n]
end

function BaseFrame:GetLayoutChildCount()
	return #self.layout_childs
end


-- Relative function
-- return true on success
-- return false on fail (mostly due to loop reference )
function BaseFrame:SetPoint(point, frame, anchor, dx, dy)
	frame = frame or self.parent
	anchor = anchor or point

	if self==frame or self:IsParentOf(frame) then
		error("SetPoint() failed: target is same or parent of this frame")
		return false
	end
	
	if not frame then
		error("SetPoint(): frame=nil and self.parent = nil, no target to relative, FAILED")
		return false
	end
	
	if self.anchors[point] then
		self.anchors[point].frame:RemoveLayoutChild(self)
	end
	
	frame:AddLayoutChild(self)
	
	if not frame_has_loop_reference(self) then	
		self.anchors[point] = {frame = frame, anchor = anchor, dx = dx or 0, dy = dy or 0}
		self.relative_to = frame;
		self.relative_point = point;
		self.anchor = anchor;
		
		self:BroadcastLayoutEvent("OnLayoutChange")		
	else
		frame:RemoveLayoutChild(self)
		
		error("SetPoint(): FAILED: loop relative")
		return false
	end
	
	return true
end


function frame_has_loop_reference(t, checker_table)
	checker_table = checker_table or t

	if checker_table == nil then return false end;

	for k,v in pairs(t.layout_childs) do

		if type(v) == "table" then

			if v == checker_table then
				return true
			end


			if frame_has_loop_reference(v, checker_table) then				
				return true
			end

		end
	end

	return false
end

function BaseFrame:BringToTop(include_parent)
	if not self.parent then return end
	local parent = self.parent
	self:RemoveFromParent()
	parent:AddChild(self)
	
	if include_parent then parent:BringToTop(include_parent) end
end

function BaseFrame:HitTest(x, y)	-- client point
	-- default hittest: by Rect
	local l,t,r,b = self:GetRect()
	l,t,r,b = 0,0,r-l,b-t
	info("HitTest", x, y, self, l, t, r, b)
	if 0<=x and x<r and 0<=y and y<b then
		return true
	else
		return false
	end
end

function BaseFrame:GetFrameByPoint(x, y) -- abs point
	local result = nil
	local l,t,r,b = self:GetRect()
	if l<=x and x<r and t<=y and y<b then
		if self:HitTest(x-l, y-t) then
			result = self
		end
		for i=1,self:GetChildCount() do
			result = self:GetChild(i):GetFrameByPoint(x, y) or result
		end
		
	end
	
	return result
end

function BaseFrame:GetPoint(point)
	local l, t, r, b = self:GetRect()
	local w, h = r-l, b-t
	local px, py = l+w/2, t+h/2
	if bit32.band(point, LEFT) == LEFT then
		px = l
	elseif bit32.band(point, RIGHT) == RIGHT then
		px = l+w
	end

	if bit32.band(point, TOP) == TOP then
		py = t
	elseif bit32.band(point, BOTTOM) == BOTTOM then
		py = t+h
	end
	
	return px, py
end

-- GetRect in Screen space

function BaseFrame:GetRect()
	if not(self.l and self.r and self.t and self.b) then
		self:CalculateAbsRect()
	end
	
	return self.l, self.t, self.r, self.b
end

function BaseFrame:DebugAbsRect()
	self.debug = true
	self:CalculateAbsRect()
	self.debug = false;
end

function BaseFrame:CalculateAbsRect()
	
	local left, right, xcenter, top, bottom, ycenter

	local default_anchors = {}
	default_anchors[TOPLEFT]={frame=nil, anchor=nil, dx=0, dy=0}
	local anchors = default_anchors
	for k,v in pairs(self.anchors) do
		anchors = self.anchors
	end
	
	if anchors == default_anchors and self.debug then
		print("using default anchor")
	end

	for point, parameter in pairs(anchors) do
		
		local frame = parameter.frame or self.parent					-- use parent as default relative_to frame
		local anchor = parameter.anchor or point
		
		if frame then
			local x, y = frame:GetPoint(anchor)
			x = x + parameter.dx
			y = y + parameter.dy
			
			if self.debug then
				print("x, y, anchor, point=", x, y, anchor, point)
			end

			
			if bit32.band(point, LEFT) == LEFT then
				left = x
			elseif bit32.band(point, RIGHT) == RIGHT then
				right = x			
			else
				xcenter = x
			end

			if bit32.band(point, TOP) == TOP then
				top = y
			elseif bit32.band(point, BOTTOM) == BOTTOM then
				bottom = y
			else
				ycenter = y
			end
		else
			left, top, right, bottom = 0,0,ui.width or 500,ui.height or 500		-- use screen as default relative_to frame if no parent & relative (mostly the root)
		end	
	end
	
	local width = self.width
	local height = self.height
	
	if left and right then
		width = right - left
		--xcenter = (right+left)/2	--useless
	elseif left and xcenter then
		width = (xcenter - left) * 2
		right = left + width
	elseif right and xcenter then
		width = (right - xcenter) * 2
		left = right - width
	elseif left and width then
		right = left + width
	elseif right and width then
		left = right - width
	elseif xcenter and width then
		left = math.floor(xcenter - width/2)
		right = math.floor(xcenter + width/2)
	end
	
	if top and bottom then
		height = bottom - top
		--ycenter = (bottom+top)/2	--useless
	elseif top and ycenter then
		height = (ycenter - top) * 2
		bottom = top + height
	elseif bottom and ycenter then
		height = (bottom - ycenter) * 2
		top = bottom - height
	elseif top and height then
		bottom = top + height
	elseif bottom and height then
		top = bottom - height
	elseif ycenter and height then
		top = math.floor(ycenter - height/2)
		bottom = math.floor(ycenter + height/2)
	end
	
	self.l, self.t, self.r, self.b = left or 0, top or 0, right or 0, bottom or 0
	
	if self.debug then
		print("left, right, xcenter, top, bottom, ycenter, width, height=", left, right, xcenter, top, bottom, ycenter, width, height)
	end
end

-- CONSTANTS
VK_RBUTTON = 2

-- time events, usally happen on next render
function BaseFrame:PreRender(time, delta_time) end

-- time events, happen on window thread timer
function BaseFrame:PreRender(time, delta_time) end

-- for these mouse events or focus related events, return anything other than false and nil to block it from being sent to its parents
function BaseFrame:OnMouseDown(button, x, y) return self.OnClick end
function BaseFrame:OnMouseUp(button, x, y) end
--function BaseFrame:OnClick(button, x, y) end
function BaseFrame:OnDoubleClick(button, x, y) end
function BaseFrame:OnMouseOver() end
function BaseFrame:OnMouseLeave() end
function BaseFrame:OnMouseMove() end
function BaseFrame:OnMouseWheel() end
function BaseFrame:OnKeyDown() end
function BaseFrame:OnKeyUp() end
function BaseFrame:OnKillFocus() end

function BaseFrame:OnMouseEvent(event, x, y, ...)
	local l, t = self:GetRect()
	return self[event] and self[event](self, x-l, y-t, ...)
end
function BaseFrame:OnDropFile(x, y, ...)
	local files = table.pack(...)
	if #files == 1 and player.load_subtitle(files[1]) then
		return true
	end
	for _,file in ipairs(files) do
		playlist:add(file)
	end
	playlist:play(files[1])
	
	return true		-- block it from being sent to parents to avoid duplicated loading.
end

function BaseFrame:IsMouseOver()
	return self == root:GetFrameByPoint(ui.get_mouse_pos())
end


--- default mouse event delivering
local focus
function OnMouseEvent(event,x,y,...)
	if event == "OnMouseDown" then
		focus = root:GetFrameByPoint(x,y)
	end
	
	local frame = focus or root:GetFrameByPoint(x,y)
	
	if event == "OnMouseUp" or event == "OnKillFocus" then
		local upframe = root:GetFrameByPoint(x,y)
		if upframe and upframe == focus then
			upframe:OnEvent("OnMouseEvent", "OnClick", x, y, ...)
		end
		focus = nil
	end

	if frame then
		return frame:OnEvent("OnMouseEvent", event, x, y, ...)
	end
end

-- for these mouse events or focus related events, return anything other than false and nil to block the event from being sent to its parents


-- Frame layout events, default handling is recalculate layout
function BaseFrame:OnLayoutChange()
	self:CalculateAbsRect();
end

-- event delivering function
function BaseFrame:OnEvent(event, ...)
	return (self[event] and self[event](self, ...)) or (self.parent and self.parent:OnEvent(event, ...))
end

function BaseFrame:BroadCastEvent(event, ...)
	if self[event] then
		self[event](self, ...)
	end
	for _,v in ipairs(self.childs) do
		v:BroadCastEvent(event, ...)
	end
end

function BaseFrame:BroadcastLayoutEvent(event, ...)
	if self[event] then
		self[event](self, ...)
	end
	for _,v in ipairs(self.layout_childs) do
		v:BroadcastLayoutEvent(event, ...)
	end
end

-- the root
root = BaseFrame:Create()
--[[
-- a simplified popup menu class
-- for static menu, use BaseFrame directly

menu_item = 
{
	string = ""
	checked = false
	grayed = false
	disabled = false
	menubarbreak = false
	seperator = false
	sub_items = nil or a table of menu_items
	on_command = function(menu_item) or nil
	
	fill ALL these 3 field to enable MF_OWNERDRAW
	width = nil
	height = nil
	ondraw = function(menu_item) or nil	
}

A sample :

function shit(tbl)
	for i=1, 50 do
		print(tbl, tbl.string)
	end
end

local menu_sample = 
{
	{
		string = "Item1",
		checked = true,
		on_command = shit,
	},
	{
		string = "Item2\tHelloWorld",
		grayed = true,
		on_command = shit,
	},
	{
		string = "Item3",
		disabled = true,
		on_command = shit,
	},
	{
		string = "sub",
		sub_items = 
		{
			{
				string = "SubItem1",
				menubarbreak=true,
				on_command = shit,
			},
			{
				string = "seperator",
				seperator = true,
			},
			{
				string = "SubItem2",
				on_command = shit,
			},
			{
				string = "SubItem3",
				on_command = shit,
			},
		}
	}
}


]]--

menu_builder = {}

local MF_CHECKED = 8
local MF_GRAYED = 1
local MF_DISABLED = 2
local MF_MENUBARBREAK = 32
local MF_SEPARATOR = 0x800
local MF_OWNERDRAW = 0x100

function menu_builder:Create(template, parent)
	local o = {}
	setmetatable(o, self)
	self.__index = self
	
	o.handle = ui.CreateMenu(parent and parent.handle)
	for _,v in ipairs(template) do
		local flags = 0
		flags = flags + (v.checked and MF_CHECKED or 0)
		flags = flags + (v.grayed and MF_GRAYED or 0)
		flags = flags + (v.disabled and MF_DISABLED or 0)
		flags = flags + (v.menubarbreak and MF_MENUBARBREAK or 0)
		flags = flags + (v.seperator and MF_SEPARATOR or 0)
		
		if v.sub_items then
			local sub = menu_builder:Create(v.sub_items, o)
			ui.AppendSubmenu(o.handle, v.string, flags, sub.handle)
		else
			ui.AppendMenu(o.handle, v.string, flags, v)
		end
	end	
	
	return o
end

function menu_builder:popup(x, y)
	local px,py = ui.get_mouse_pos()
	x, y = x or px, y or py	
	return ui.PopupMenu(self.handle, x-px, y-py)
end

function menu_builder:destroy()
	return ui.DestroyMenu(self.handle)
end


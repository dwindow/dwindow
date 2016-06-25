function popup3dvstar()
	local m = 
	{
		{
			string = L("Open File..."),
			on_command = function(v)
				local url = ui.OpenFile()
				if url then
					playlist:play(url)
				end
			end
		},
		{
			string = L("Open Folder..."),
			on_command = function(v)
				local folder = ui.OpenFolder()
				if folder then
					local tbl = player.enum_folder(folder)
					local to_play
					for _,v in pairs(tbl) do
						if v:sub(v:len()) ~= "\\" then
							to_play = folder .. v
							playlist:add(folder .. v)
						else
						end
					end						
							
					if to_play then
						playlist:play(to_play)
					end
				end
			end
		},
		{
			string = L("Close File"),
			grayed = not player.movie_loaded,
			on_command = function(v)
				player.reset()
			end
		},
		{
			seperator = true,
		},
		{
			string = L("Play\t(Space)"),
			grayed = player.is_playing(),
			on_command = function(v) player.pause() end
		},
		{
			string = L("Pause\t(Space)"),
			grayed = not player.is_playing(),
			on_command = function(v) player.pause() end
		},
		{
			string = L("Stop"),
			on_command = function(v) player.stop() end
		},
		{
			string = L("Previous\t(PageUp)"),
			grayed = playlist:current_pos() <= 1,
			on_command = function(v) playlist:previous() end
		},
		{
			string = L("Next\t(PageDown)"),
			grayed = playlist:current_pos() >= playlist:count(),
			on_command = function(v) playlist:next() end
		},
		{
			string = L("Playlist"),
			sub_items =
			{
				{
					string = "Item1",
					checked = true
				},
				{
					string = "Item2",
				},
				{
					seperator = true,
				},
				{
					string = "Clear",
				},
			},
		},
		{
			string = "Intel® WiDi",
			sub_items = 
			{
				{
					string = L("No Adaptors Found"),
				},
				{
					seperator = true,
				},
				{
					string = L("Scan For Available Adapters"),
					on_command = function() player.widi_start_scan() end
				},
				{
					string = L("Disconnect"),
					on_command = function() player.widi_disconnect() end
				},
				{
					seperator = true,
				},
				{
					string = L("Screen Mode"),
					grayed = true,
				},
				{
					string = L("Clone"),
					on_command = function() player.widi_set_screen_mode(2) end
				},
				{
					string = L("Extended"),
					on_command = function() player.widi_set_screen_mode(3) end
				},
				{
					string = L("External Only"),
					on_command = function() player.widi_set_screen_mode(4) end
				},
			},
		},
		{
			seperator = true,
		},
		{
			string = L("Switch To 2D Mode"),
			on_command = function() set_setting("Force2D", true) end
		},
		{
			string = L("Switch To 3D Mode"),
			on_command = function() set_setting("Force2D", false) end
		},
		{
			string = L("Swap Eyes"),
			on_command = function() set_setting("SwapEyes", not setting.SwapEyes) end
		},
		{
			seperator = true,
		},
		{
			string = L("Settings..."),
			on_command = function() player.show_setting() end
		},
		{
			string = L("About..."),
			on_command = function() player.show_about() end
		},
		{
			string = L("Exit"),
			on_command = function() player.exit() end
		},
	}
	
	-- playlist
	local t = {}
	for i=1,playlist:count() do
		local item = playlist:item(i)
		local text = GetName(item.L) or " "
		if item.R then text = text .. " + " .. GetName(item.R) end
				
		table.insert(t, {string=text, checked = (playlist:current_pos() == i), id = i, on_command = function(v) playlist:play_item(v.id) end})
	end
	
	table.insert(t, {seperator = true})
	table.insert(t, {string = "清空", on_command = function(v) playlist:clear() end})	
	m[10].sub_items = t
	-- widi
	if not player.widi_has_support() then
		table.remove(m, 11)
	else
		local p = m[11].sub_items
		
		local found = false
		for i,v in ipairs(table.pack(player.widi_get_adapters())) do
			found = true
			local desc = player.widi_get_adapter_information(i-1) .. "(" .. L(player.widi_get_adapter_information(i-1, "State")) .. ")"
			table.insert(p, 2, {string=desc, id = i, on_command = function(v) player.widi_connect(v.id-1)end})
		end
		
		if found then
			table.remove(p, 1)
		end	
		
	end
	
	m = menu_builder:Create(m)
	m:popup()
	m:destroy()	
	
end
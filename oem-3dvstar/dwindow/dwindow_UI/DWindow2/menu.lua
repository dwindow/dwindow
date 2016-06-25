local MB_YESNO = 4
local IDYES = 6
function popup_dwindow2()
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
			string = L("Open Left And Right File..."),
			on_command = function(v)
				local left, right = ui.OpenDoubleFile()
				if left and right then
					playlist:play(left, right)
				end
			end
		},
		{
			string = L("Open BluRay3D"),
			on_command = function(v)
				playlist:clear()
			end
		},
		{
			string = L("Close"),
			grayed = not player.movie_loaded,
			on_command = function() player.reset() end
		},
		{
			seperator = true,
		},
		{
			string = L("Video"),
			sub_items =
			{
				{
					string = L("Adjust Color..."),
					on_command = function()player.show_color_adjust_dialog() end
				},
				{
					seperator = true,
				},
				{
					string = L("Use CUDA Accelaration"),
					checked = setting.CUDA,
					on_command = function()
						set_setting("CUDA", not setting.CUDA)
						player.message_box(L("CUDA setting will apply on next file play."), "...", MB_OK)
					end
				},
				{
					string = L("Use DXVA2 Accelaration (Experimental)"),
					checked = setting.EVR,
					on_command = function()
						if not setting.EVR then
							if IDYES == player.message_box(L("This is a experimental feature.\r\nIt ONLY works for most H264 video.\r\nIt can cause a lot freeze or crash.\r\nIt will freeze with BD3D/MPO/3DP file or left+right file, and disable input layout detecting\r\nContinue ?"), "Warning", MB_YESNO) then
								set_setting("EVR", true)							
							end
						else
							set_setting("EVR", false)
						end
					end
				},
				{
					seperator = true,
				},
				{
					string = L("Aspect Ratio"),
					grayed = true
				},
				{
					string = L("Source Aspect"),
					checked = setting.Aspect < 0,
					on_command = function()
						set_setting("Aspect", -1)
					end
				},
				{
					string = L("16:9"),
					checked = math.abs(setting.Aspect - 16/9) < 0.001,
					on_command = function()
						set_setting("Aspect", 16/9)
					end
				},
				{
					string = L("4:3"),
					checked = math.abs(setting.Aspect - 4/3) < 0.001,
					on_command = function()
						set_setting("Aspect", 4/3)
					end
				},
				{
					string = L("2.35:1"),
					checked = math.abs(setting.Aspect - 2.35) < 0.001,
					on_command = function()
						set_setting("Aspect", 2.35)
					end
				},
				{
					seperator = true,
				},
				{
					string = L("Fill Mode"),
					grayed = true
				},
				{
					string = L("Default(Letterbox)"),
					checked = setting.AspectRatioMode == 0,
					on_command = function() set_setting("AspectRatioMode", 0) end
				},
				{
					string = L("Horizontal Fill"),
					checked = setting.AspectRatioMode == 2,
					on_command = function() set_setting("AspectRatioMode", 2) end
				},
				{
					string = L("Vertical Fill"),
					checked = setting.AspectRatioMode == 3,
					on_command = function() set_setting("AspectRatioMode", 3) end
				},
				{
					string = L("Stretch"),
					checked = setting.AspectRatioMode == 1,
					on_command = function() set_setting("AspectRatioMode", 1) end
				},
				{
					seperator = true,
				},
				{
					string = L("Quality"),
					grayed = true
				},
				{
					string = L("Best Quality (Low Speed)"),
					checked = setting.MovieResampling == 1,
					on_command = function() set_setting("MovieResampling", 1) set_setting("SubtitleResampling", 1) end
				},
				{
					string = L("Better Quality"),
					checked = setting.MovieResampling == 0,
					on_command = function() set_setting("MovieResampling", 0) set_setting("SubtitleResampling", 0) end
				},
				{
					string = L("Normal Quality (Fast)"),
					checked = setting.MovieResampling == 2,
					on_command = function() set_setting("MovieResampling", 2) set_setting("SubtitleResampling", 2) end
				},
				{
					seperator = true,
				},
				{
					string = L("Move Up\t(W)"),
					on_command = function() set_setting("MoviePosY", setting.MoviePosY - 0.005 ) end
				},
				{
					string = L("Move Down\t(S)"),
					on_command = function() set_setting("MoviePosY", setting.MoviePosY + 0.005 ) end
				},
				{
					string = L("Move Left\t(A)"),
					on_command = function() set_setting("MoviePosX", setting.MoviePosX - 0.005 ) end
				},
				{
					string = L("Move Right\t(D)"),
					on_command = function() set_setting("MoviePosX", setting.MoviePosX + 0.005 ) end
				},
				{
					string = L("Zoom In\t(NumPad 7)"),
					on_command = function() player.set_zoom_factor(player.get_zoom_factor() * 1.05) end
				},
				{
					string = L("Zoom Out\t(NumPad 1)"),
					on_command = function() player.set_zoom_factor(player.get_zoom_factor() / 1.05) end
				},
				{
					string = L("Increase Parallax\t(NumPad *)"),
					on_command = function() player.set_parallax(player.get_parallax() + 0.001) end
				},
				{
					string = L("Decrease Parallax\t(NumPad /)"),
					on_command = function() player.set_parallax(player.get_parallax() - 0.001) end
				},
				{
					string = L("Reset\t(NumPad 5)"),
					on_command = function()
						player.set_subtitle_pos(0.5, 0.95)
						player.set_parallax(0)
						player.set_subtitle_parallax(0)
						set_setting("MoviePosX", 0)
						set_setting("MoviePosY", 0)
						player.set_zoom_factor(1)
					end
				},
				
			}
		},
		{
			string = L("Audio"),
			sub_items =
			{
				{
					string = L("Load Audio Track..."),
					on_command = function()
						local file = ui.OpenFile(L("Audio Files").."(*.mp3;*.mxf;*.dts;*.ac3;*.aac;*.m4a;*.mka)", "*.mp3;*.mxf;*.dts;*.ac3;*.aac;*.m4a;*.mka", L("All Files"), "*.*")
						player.load_audio_track(file)
					end
				},
				{
					seperator = true,
				},
				{
					string = L("Use ffdshow Audio Decoder"),
					checked = setting.InternalAudioDecoder,
					on_command = function()
						set_setting("InternalAudioDecoder", not setting.InternalAudioDecoder)
						if player.movie_loaded then
							player.message_box(L("Audio Decoder setting may not apply until next file play or audio swtiching."), L("..."), MB_OK)
						end
					end
				},
				{
					string = L("Normalize Audio"),
					checked = player.get_normalize_audio(),
					on_command = function() player.set_normalize_audio(not player.get_normalize_audio()) end
				},
				{
					string = L("Latency && Stretch..."),
					on_command = function()
						local latency = player.show_latency_ratio_dialog(true, setting.AudioLatency, 1)
						set_setting("AudioLatency", latency)
					end
				},
				{
					seperator = true,
				},
				{
					string = L("Channel Setting"),
					grayed = true,
				},
				{
					string = L("Source"),
					checked = setting.AudioChannel == 0,
					on_command = function() player.set_output_channel(0) end
				},
				{
					string = L("Stereo"),
					checked = setting.AudioChannel == 2,
					on_command = function() player.set_output_channel(2) end
				},
				{
					string = L("5.1 Channel"),
					checked = setting.AudioChannel == 6,
					on_command = function() player.set_output_channel(6) end
				},
				{
					string = L("7.1 Channel"),
					checked = setting.AudioChannel == 8,
					on_command = function() player.set_output_channel(8) end
				},
				{
					string = L("Dolby Headphone"),
					checked = setting.AudioChannel == 1,
					on_command = function() player.set_output_channel(1) end
				},
				{
					string = L("Bitstreaming"),
					checked = setting.AudioChannel == -1,
					on_command = function() player.set_output_channel(-1) end
				},
			}
		},
		{
			string = L("Subtitle"),
			sub_items =
			{
				{
					string = L("Load Subtitle File..."),
					on_command = function()
						local file = ui.OpenFile(L("Subtitle Files").."(*.srt;*.sup;*.ssa;*.ass;*.sub;*.idx)", "*.srt;*.sup;*.ssa;*.ass", L("All Files"), "*.*")
						player.load_subtitle(file)
					end
				},
				{
					seperator = true,
				},
				{
					string = L("Latency && Stretch..."),
					on_command = function()
						local latency, ratio = player.show_latency_ratio_dialog(false, setting.SubtitleLatency, setting.SubtitleRatio)
						set_setting("SubtitleLatency", latency)
						set_setting("SubtitleRatio", ratio)
					end
				},
				{
					string = L("Display Subtitle"),
					checked = setting.DisplaySubtitle,
					on_command = function() set_setting("DisplaySubtitle", not setting.DisplaySubtitle) end,
				},
				{
					seperator = true,
				},
				{
					string = L("Increase Parallax\t(NumPad 9)"),
					on_command = function() player.set_subtitle_parallax(player.get_subtitle_parallax()+1) end
				},
				{
					string = L("Decrease Parallax\t(NumPad 3)"),
					on_command = function() player.set_subtitle_parallax(player.get_subtitle_parallax()-1) end
				},
				{
					string = L("Move Up\t(NumPad 8)"),
					on_command = function()
						local _,y = player.get_subtitle_pos()
						player.set_subtitle_pos(nil, y-0.0025)
					end
				},
				{
					string = L("Move Down\t(NumPad 2)"),
					on_command = function()
						local _,y = player.get_subtitle_pos()
						player.set_subtitle_pos(nil, y+0.0025)
					end
				},
				{
					string = L("Move Left\t(NumPad 4)"),
					on_command = function()
						local x = player.get_subtitle_pos()
						player.set_subtitle_pos(x-0.0025)
					end
				},
				{
					string = L("Move Right\t(NumPad 6)"),
					on_command = function()
						local x = player.get_subtitle_pos()
						player.set_subtitle_pos(x+0.0025)
					end
				},
				{
					string = L("Reset\t(NumPad 5)"),
					on_command = function()
						player.set_subtitle_pos(0.5, 0.95)
						player.set_parallax(0)
						player.set_subtitle_parallax(0)
						set_setting("MoviePosX", 0)
						set_setting("MoviePosY", 0)
						player.set_zoom_factor(1)
					end
				},
			},
		},
		{
			string = L("Movie Layout"),
			sub_items = 
			{
				{
					string = L("Auto"),
					checked = setting.InputLayout == 0x10000,
					on_command = function() player.set_input_layout(0x10000) end
				},
				{
					string = L("Side By Side"),
					checked = setting.InputLayout == 0,
					on_command = function() player.set_input_layout(0) end
				},
				{
					string = L("Top Bottom"),
					checked = setting.InputLayout == 1,
					on_command = function() player.set_input_layout(1) end
				},
				{
					string = L("Frame Sequential"),
					checked = setting.InputLayout == 3,
					on_command = function() player.set_input_layout(3) end
				},
				{
					string = L("Monoscopic"),
					checked = setting.InputLayout == 2,
					on_command = function() player.set_input_layout(2) end
				},
			}
		},
		{
			string = L("Viewing Device"),
			sub_items = 
			{
				{
					string = L("Nvidia 3D Vision"),
					checked = setting.OutputMode == 0,
					on_command = function() player.set_output_mode(0) end,
				},
				{
					string = L("AMD HD3D"),
					checked = setting.OutputMode == 11,
					on_command = function() player.set_output_mode(11) end,
				},
				{
					string = L("Intel Stereoscopic"),
					checked = setting.OutputMode == 12,
					on_command = function() player.set_output_mode(12) end,
				},
				{
					string = L("IZ3D Displayer"),
					checked = setting.OutputMode == 5,
					on_command = function() player.set_output_mode(5) end,
				},
				{
					string = L("Gerneral 120Hz Glasses"),
					checked = setting.OutputMode == 4,
					on_command = function() player.set_output_mode(4) end,
				},
				{
					string = L("Anaglyph"),
					checked = setting.OutputMode == 2,
					on_command = function() player.set_output_mode(2) end,
				},
				{
					string = L("Monoscopic 2D"),
					checked = setting.OutputMode == 3,
					on_command = function() player.set_output_mode(3) end,
				},
				{
					string = L("Subpixel Interlace"),
					checked = setting.OutputMode == 1 and setting.MaskMode == 3,
					on_command = function() player.set_output_mode(1) player.set_mask_mode(3) end,
				},
				{
					seperator = true,
				},
				{
					string = L("3DTV"),
					grayed = true,
				},
				{
					string = L("3DTV - Half Side By Side"),
					checked = setting.OutputMode == 9,
					on_command = function() player.set_output_mode(9) end,
				},
				{
					string = L("3DTV - Half Top Bottom"),
					checked = setting.OutputMode == 10,
					on_command = function() player.set_output_mode(10) end,
				},
				{
					string = L("3DTV - Line Interlace"),
					checked = setting.OutputMode == 1 and setting.MaskMode == 1,
					on_command = function() player.set_output_mode(1) player.set_mask_mode(1) end,
				},
				{
					string = L("3DTV - Checkboard Interlace"),
					checked = setting.OutputMode == 1 and setting.MaskMode == 2,
					on_command = function() player.set_output_mode(1) player.set_mask_mode(2) end,
				},
				{
					string = L("3DTV - Row Interlace"),
					checked = setting.OutputMode == 1 and setting.MaskMode == 0,
					on_command = function() player.set_output_mode(1) player.set_mask_mode(0) end,
				},
				{
					seperator = true,
				},
				{
					string = L("Dual Projector"),
					grayed = true,
				},
				{
					string = L("Dual Projector - Vertical Span Mode"),
					checked = setting.OutputMode == 8,
					on_command = function() player.set_output_mode(8) end,
				},
				{
					string = L("Dual Projector - Horizontal Span Mode"),
					checked = setting.OutputMode == 7,
					on_command = function() player.set_output_mode(7) end,
				},
				{
					string = L("Dual Projector - Independent Mode"),
					checked = setting.OutputMode == 6,
					on_command = function() player.set_output_mode(6) end,
				},
				{
					seperator = true,
				},
				{
					string = L("Fullscreen Output 1"),
				},
				{
					string = L("Fullscreen Output 2"),
				},
				{
					string = L("HD3D Fullscreen Mode Setting..."),
					on_command = function() player.show_hd3d_fullscreen_mode_dialog() end
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
			string = player.is_playing() and L("Pause\t(Space)") or L("Play\t(Space)"),
			grayed = not player.movie_loaded,
			on_command = function() player.pause() end
		},
		{
			string = L("Fullscreen\t(Alt+Enter)"),
			on_command = function() player.toggle_fullscreen() end
		},
		{
			string = L("Swap Left/Right\t(Tab)"),
			checked = setting.SwapEyes,
			on_command = function() player.set_swapeyes(not player.get_swapeyes()) end
		},
		{
			seperator = true,
		},
		
		
		{
			string = L("Media Infomation..."),
			grayed = not player.movie_loaded,
			on_command = function() player.show_media_info(playlist:current_item()) end
		},
		{
			string = L("Language"),
		},
		{
			string = L("About..."),
			on_command = function() player.show_about() end
		},
		{
			string = L("Logout..."),
			on_command = function()
				if player.message_box(L("Are you sure want to logout?"), L("Are you sure?"), MB_YESNO) == IDYES then
					player.logout()
					core.restart_this_program()
				end
			end
		},
		{
			string = L("Exit"),
			on_command = function() player.exit() end
		},
	}
	
	-- BluRay 3D
	local bd3d = {}
	m[3].sub_items = bd3d
	local bds = table.pack(player.enum_bd())
	for i=1,#bds/3 do
		local path = bds[i*3-2]
		local volume_name = bds[i*3-1]
		local is_bluray = bds[i*3-0]
		local desc = path ..  (is_bluray and (" (" ..volume_name .. ")") or (volume_name and L(" (No Movie Disc)") or L(" (No Disc)")))
		table.insert(bd3d, {grayed = not(path and volume_name and is_bluray), string= desc, path=path, on_command = function(t) playlist:play(t.path) end})
	end
	table.insert(bd3d, {seperator = true})
	table.insert(bd3d, {string = L("Folder..."), on_command = function()
				local folder = ui.OpenFolder()
				if folder then
					playlist:play(folder)
				end
			end})

	-- language
	m[18].sub_items = {}
	for _,v in pairs(core.get_language_list()) do
		table.insert(m[18].sub_items, {checked = v == setting.LCID, string=v, on_command = function(t) core.set_language(t.string) end})
	end
	
	-- monitors
	local f1 = {}
	local f2 = {}
	local monitor = table.pack(player.enum_monitors())
	for i=1,#monitor/5 do
		local check1 = monitor[i*5-4] == setting.Screen1.left and monitor[i*5-3] == setting.Screen1.top and monitor[i*5-2] == setting.Screen1.right and monitor[i*5-1] == setting.Screen1.bottom
		local check2 = monitor[i*5-4] == setting.Screen2.left and monitor[i*5-3] == setting.Screen2.top and monitor[i*5-2] == setting.Screen2.right and monitor[i*5-1] == setting.Screen2.bottom
		table.insert(f1, {checked = check1, string = monitor[i*5], id = i - 1, on_command = function(t) player.set_monitor(0, t.id) end})
		table.insert(f2, {checked = check2, string = monitor[i*5], id = i - 1, on_command = function(t) player.set_monitor(1, t.id) end})
	end
	
	m[10].sub_items[22].sub_items = f1
	m[10].sub_items[23].sub_items = f2
	
	-- subtitle
	local sub = m[8].sub_items
	local subs = table.pack(player.list_subtitle_track())
	if #subs == 0 then
		table.insert(sub, 1, {grayed = true, string = L("No Subtitle")})
	else
		for i=1, #subs/2 do
			table.insert(sub, 1, {string = subs[i*2-1], checked = subs[i*2], id = i-1, on_command = function(t) player.enable_subtitle_track(t.id) end})
		end
	end
	
	
	-- audio
	local aud = m[7].sub_items
	local auds = table.pack(player.list_audio_track())
	if #auds == 0 then
		table.insert(aud, 1, {grayed = true, string = L("No Audio Track")})
	else
		for i=1, #auds/2 do
			table.insert(aud, 1, {string = auds[i*2-1], checked = auds[i*2], id = i-1, on_command = function(t) player.enable_audio_track(t.id) end})
		end
	end
	
	
	-- widi
	if not player.widi_has_support() then
		table.remove(m, 11)
	else
		local p = m[11].sub_items
		
		local found = false
		for i,v in ipairs(table.pack(player.widi_get_adapters())) do
			found = true
			local desc = player.widi_get_adapter_information(v) .. "(" .. L(player.widi_get_adapter_information(v, "State")) .. ")"
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
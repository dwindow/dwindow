playlist = {}
setting.playlist = {}
setting.playlist.playing = 0
setting.playlist.list = {}

local get_pos

function playlist:add(L, R, pos)
	local pos = get_pos(L, R)
	if pos then return pos end
	
	table.insert(setting.playlist.list, pos or (#setting.playlist.list+1), {L=L,R=R})
	root:BroadCastEvent("OnPlaylistChange")
	return pos or #setting.playlist.list
end

function playlist:play(L, R)
	local pos = get_pos(L, R)
	if (pos == setting.playlist.playing) and setting.playlist.playing then
		if not player.movie_loaded then
			return playlist:play_item(setting.playlist.playing)
		end
	end
	return playlist:play_item(playlist:add(L, R))
end

function playlist:remove(L, R)
	local pos = get_pos(L, R)
	return self:remove_item(pos)
end

function playlist:remove_item(pos)
	if pos then
		table.remove(setting.playlist.list, pos)
		root:BroadCastEvent("OnPlaylistChange")
	end
end

function playlist:clear()
	setting.playlist.list = {}
	setting.playlist.playing = 0
	root:BroadCastEvent("OnPlaylistChange")
end

function playlist:next()
	if setting.playlist.playing< #setting.playlist.list then
		return playlist:play_item(setting.playlist.playing+1) or playlist:next()
	end
end

function playlist:previous()
	if setting.playlist.playing > 1 then
		return playlist:play_item(setting.playlist.playing-1)
	end
end

function playlist:play_item(n)
	if not n or n < 1 or n > #setting.playlist.list then
		return false
	end
	
	setting.playlist.playing = n
		
	return player.reset_and_loadfile(setting.playlist.list[n].L, setting.playlist.list[n].R)
end

function playlist:item(n)
	return setting.playlist.list[n]
end

function playlist:count()
	return #setting.playlist.list
end

function playlist:current_pos()
	return setting.playlist.playing
end

function playlist:current_item()
	return playlist:item(setting.playlist.playing) and playlist:item(setting.playlist.playing).L, playlist:item(setting.playlist.playing) and playlist:item(setting.playlist.playing).R
end

get_pos = function(L, R)
	for i=1,#setting.playlist.list do
		if setting.playlist.list[i].L == L and setting.playlist.list[i].R == R then
			return i
		end
	end
end
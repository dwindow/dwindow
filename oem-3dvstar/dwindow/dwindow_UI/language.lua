local lua_file = core.loading_file
local lua_path = GetPath(lua_file)
language_list = {}
local LL = {}

function L(english)
	return LL[english] or english
end

function core.add_language(t)
	if type(t) ~= "table" or type(t.LCID) ~= "table" or type(t.LCID[1]) ~= "string" then
		return false	-- invalid language table
	end
	
	language_list[t.LCID[1]] = t
end

function core.set_language(new_language)
	if new_language then
		if language_list[new_language] then
			LL = language_list[new_language]
			setting.LCID = new_language
		else
			LL = {}						-- if language missing, use enUS
			setting.LCID = "English"
		end
	else
		-- search for proper LCID
		local id = core.GetSystemDefaultLCID()
		for _,l in pairs(language_list) do
			for i=2,#l.LCID do
				if l.LCID[i] == id then
					LL = l
					setting.LCID = l.LCID[1]
				end
			end
		end
		
		-- use English if no proper LCID found
		if not setting.LCID then
			LL = {}
			setting.LCID = "English"
		end
	end
	
	core.ApplySetting("LCID")
	root:BroadCastEvent("OnLanguageChange")
end

function core.get_language_list()
	local o = {}
	for k,v in pairs(language_list) do
		print(k)
		table.insert(o, k)
	end
	return o
end

-- load all language file
for _,v in pairs(player.enum_folder(lua_path .. "Language\\")) do
	core.execute_luafile(lua_path .. "Language\\" .. v)
end

-- load setting's LCID
core.set_language(setting.LCID)
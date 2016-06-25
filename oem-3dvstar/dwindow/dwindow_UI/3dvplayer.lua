local http = {request = core.http_request}
local lua_file = core.loading_file
local lua_path = GetPath(lua_file)
local function GetCurrentLuaPath(offset)
	return lua_path
end

PageMerger ={}

function PageMerger:Create(o)
	o = o or {}
	o.childs = {}
	setmetatable(o, self)
	self.__index = self

	return o
end

function PageMerger:openURL(url)
	self.cache = {}
	self.total = nil
	self.pagesize = nil
	self.url = url

	self:GetPage(1)
end

function PageMerger:GetCount()
	return self.total
end

function PageMerger:GetPageSize()
	return self.pagesize
end

function PageMerger:GetItem(n)
	local page = math.floor((n-1) / self.pagesize) +1
	local item = (n-1) % self.pagesize + 1

	return self:GetPage(page)[item]
end

function PageMerger:GetPage(n)
	n = n or 1

	self.cache[n] = self.cache[n] or json_url2table(self.url .. "&page="..n)

	if not self.cache[n] then return nil end

	self.total = self.cache[n].splitpage.total
	self.pagesize = self.cache[n].splitpage.pagesize

	return self.cache[n].pagecontent
end

function printtable(table, level)
	level = level or 0
	local prefix = ""
	for i=1,level do
		prefix = prefix .. "  "
	end

	for k,v in pairs(table) do
		if type(v) == "table" then
			print(prefix..k..":")
			printtable(v, level+1)
		else
			print(prefix..k .. ":\t" .. tostring(v))
		end
	end
end

function istable(t)
	return type(t) == "table"
end

function json_url2table(url)
	local bytes, response_code, headers = http.request(url)
	
	--print(bytes, response_code, headers)

	bytes = string.gsub( bytes, "\r\n", "\\n")
	
	local json = core.cjson()

	local suc,result = pcall(json.decode, bytes)

	--print(suc, result)
	if not istable(result) then return nil, result end

	return result
end

function string_split(pString, pPattern)
   local Table = {}  -- NOTE: use {n = 0} in Lua-5.0
   local fpat = "(.-)" .. pPattern
   local last_end = 1
   local s, e, cap = pString:find(fpat, 1)
   while s do
      if s ~= 1 or cap ~= "" then
     table.insert(Table,cap)
      end
      last_end = e+1
      s, e, cap = pString:find(fpat, last_end)
   end
   if last_end <= #pString then
      cap = pString:sub(last_end)
      table.insert(Table, cap)
   end
   return Table
end

function v3d_init()
	if v3d_inited then return end
	
	v3dplayer = {}
	v3dplayer.API_ROOT_PATH = "http://api.3dv.cn/p/ver=33&mac=00-00-00-00-00-00&model=dwindow&uid=0&"
	v3dplayer.feedback_url = v3dplayer.API_ROOT_PATH .. "mod=feedback"
	v3dplayer.rating_url = v3dplayer.API_ROOT_PATH .. "mod=mark"
	v3dplayer.comment_url = v3dplayer.API_ROOT_PATH .. "mod=comment_url"
	v3dplayer.details_web_url = v3dplayer.API_ROOT_PATH .. "mod=movie&product=player&mid="
	v3dplayer.subtitle_url = v3dplayer.API_ROOT_PATH .. "mod=subtitle&json=1"

	local configuration_base = json_url2table(v3dplayer.API_ROOT_PATH)

	if not istable(configuration_base) then
		error("configuration_base")
	end

	printtable(configuration_base)

	v3dplayer.video_server = configuration_base.server or "http://v.cnliti.com/"
	local configuration = json_url2table(v3dplayer.API_ROOT_PATH .. "sign=" .. (configuration_base.version or 0) )

	--printtable(configuration)

	if not istable(configuration) then
		error("configuration")
	end

	v3dplayer.maintenance = configuration.maintain or ""
	v3dplayer.pic_url = configuration.picPath or "http://www.cnliti.com/uploadfile/"
	v3dplayer.thumb_url = configuration.thumbPath or "http://www.cnliti.com/uploadfile/"
	v3dplayer.subtitle_url = configuration.subtitlePath or "http://www.cnliti.com/uploadfile/subtitle/"
	v3dplayer.speed_limit = tonumber(configuration.limitspeed or 300)
	v3dplayer.min_3dvplayer_version = tonumber(configuration.forceupdate or 17)

	--[[
	--printtable(v3dplayer)
	local marks = {"video", "trailers", "short", "demo"}
	for _,mark in pairs(marks) do
		local video = PageMerger:Create()
		video:openURL(v3dplayer.API_ROOT_PATH .. "mod=movielist&mark="..mark)
		for i=1,video:GetCount() do
			local movie = video:GetItem(i)

			local addresses = string_split(movie.address or "||", "|")
			local highest = addresses[1] or addresses[2] or addresses[3]
			print(movie.name or "", movie.id or 0, v3dplayer.video_server .. highest,
				v3dplayer.subtitle_url .. movie.caption)

			local detail_url = v3dplayer.API_ROOT_PATH .. "mod=info&mid=" .. movie.id .. "&mark=" .. mark;
			local detail, error_desc = json_url2table(detail_url)

			if istable(detail) then
				printtable(detail)
			else
				print(detail_url, error_desc)
			end
		end
	end
	]]--

	video_list = PageMerger:Create()
	video_list:openURL(v3dplayer.API_ROOT_PATH .. "mod=movielist&mark=".."video")

	v3d_inited = true
end

function load_another(n)

	Thread:Create(function()
	v3d_init()

	math.randomseed(core.GetTickCount())
	local movie = video_list:GetItem(n or math.random(video_list:GetCount()))
	local addresses = string_split(movie.address or "||", "|")
	local highest = (addresses[1] ~= "NULL" and addresses[1]) or (addresses[2] ~= "NULL" and addresses[2]) or (addresses[3] ~= "NULL" and addresses[3])

	playlist:play(v3dplayer.video_server .. highest)
	
	if movie.caption and movie.caption ~= "" then
		player.load_subtitle(v3dplayer.subtitle_url .. movie.caption)
	end
	
	--printtable(movie)
	end, 0)
end

function v3dplayer_getitem(id)
	v3d_init()
	return video_list:GetItem(id)
end


function v3dplayer_add_button()
	local test = BaseFrame:Create()
	root:AddChild(test)
	test:SetPoint(TOPLEFT)

	function test:RenderThis()
		paint(0,0,self.width,self.height, self.res, 1, bilinear_mipmap_minus_one)
	end

	function test:OnInitCPU()
		self.res = self.res or test_get_text_bitmap("HelloWorld你好")
		self:SetSize(self.res.width, self.res.height)
	end

	function test:OnMouseDown()
		if load_another then load_another() end
	end

	function test:OnReleaseCPU()
		if self.res and self.res.res then
			core.release_resource_core(self.res.res)
		end
		self.res = nil
	end
end
local socket = require "socket"
local http = require "socket.http"
local ltn12 = require("ltn12")
--require "utils"

local url = "http://wifisd/sd/DCIM/107CANON/"

function get_url(url,file)
	local body = {}
	local req_tbl = {url=url, sink=ltn12.sink.table(body), create=function()
	  local req_sock = socket.tcp()
	  req_sock:settimeout(5)
	  return req_sock
	end}
	if file then
		req_tbl.sink = ltn12.sink.file(io.open(file,"w"))
	end
	local res, code, headers, status = http.request(req_tbl)
	--dump_table(body)
	--return res, code, headers, status
	return table.concat(body)
end

function parse_list(body)
	local files = {}
	local res = {}
	--for fn, size in string.gmatch(body,">(%g+%.JPG).+(%d+)%sbytes.+") do
	for fn in string.gmatch(body,">(%g+%.JPG)") do
		--print(fn,size)
		table.insert(files,fn)
	end
	local i=1
	for sz in string.gmatch(body,"(%d+) bytes") do
		--print(sz)
		--table.insert(sizes,sz)
		res[files[i]]=sz+0
		i = i + 1
	end
	return res
end

function fsize(fn)
	local fh = io.open(fn,"r")
	if not fh then return nil end
	local size = fh:seek("end")
	fh:close()
	return size
end

local body

print("waiting for SD...")
repeat
	body = get_url(url)
until body

--print(body)

local list = parse_list(body)
--dump_table(parse_list(txt))

for fn,sz in pairs(list) do
	--print(fn,sz,fsize(fn))
	if fsize(fn) ~= sz then
		print("get "..fn)
		get_url(url..fn,fn)
	end
end

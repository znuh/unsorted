#!/usr/bin/lua
local inotify = require 'inotify'
local notify = inotify.init()

-- Watch for new files and renames
local wd = notify:addwatch('.', inotify.IN_CLOSE_WRITE)

local last_md5

for ev in notify:events() do
	if ev.name:match("_bitgen.xwbt") then
		local ls = io.popen("ls -t *.bit","r")
		local bs = ls:read("*line")
		ls:close()
		local md5 = io.popen("md5sum "..bs,"r")
		local sum = md5:read("*line")
		md5:close()
		if sum ~= last_md5 then
			print(bs.." modified")
			local cmd = arg[1]:gsub("%%f",bs)
			print(cmd)
			os.execute(cmd)
			print("done")
		end
		last_md5 = sum
	end
end

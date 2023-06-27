#!/usr/bin/lua

-- output lines from stdin to stdout with precise (1us) timestamps
-- arg[1]:
--   --rel-start: print timestamps relative to start of script
--   --rel-first: print timestamps relative to first line
--   --rel-prev : print timestamps relative to previous line

local sock = require "socket"

local ref  = nil
local mode = arg[1]

if mode == "--rel-start" then ref = sock.gettime() end

for line in io.lines() do
	local now = sock.gettime()
	line = line:gsub("\r","")
	if ref == nil then
		if mode == "--rel-first" or mode == "--rel-prev" then
			ref = now
		else
			ref = 0
		end
	end
	print(string.format("%.6f %s",now - ref, line))
	if mode == "--rel-prev" then
		ref = now
	end
end

#!/usr/bin/lua

-- output lines from stdin to stdout with precise timestamps

local sock = require "socket"

local last = nil

-- cmdline argument -rel --relative etc. -> output relative timestamps
if string.find(arg[1] or "","rel") then
	last = sock.gettime()
end

for line in io.lines() do
	local now = sock.gettime()
	print(string.format("%.6f %s",now - (last or 0), line))
	if last ~= nil then last = now end
end

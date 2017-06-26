#!/usr/bin/lua

-- simple traffic monitor

socket = require("socket")

local function find_route()
	local p = io.popen("/sbin/route -n","r")
	local iface
	for line in p:lines() do
		iface = iface or line:match("^0%.0%.0%.0.+%s+(%g+)$")
	end
	p:close()
	return iface
end

local function fmt(val)
	local prefixes = { "Ki", "Mi" }
	local prefix = "  "
	for i=1,2 do
		if val < 1000 then break end
		val = val / 1024
		prefix = prefixes[i]
	end
	return string.format("%5.1f "..prefix.."B/s",val)
end

local last_rx, last_tx

local iface = arg[1] or find_route() -- use if w/ default route if none given

while true do
	local p = io.popen("/sbin/ifconfig "..iface,"r")
	local rx, tx = nil, nil
	for line in p:lines() do
		rx = rx or line:match("RX%s+packets.*%s+bytes%s+(%d+)")
		tx = tx or line:match("TX%s+packets.*%s+bytes%s+(%d+)")
	end
	p:close()
	if last_rx then
		local r, t = rx - last_rx, tx - last_tx
		io.write("\r"..iface.."   RX: "..fmt(r).."   TX: "..fmt(t))
		io.stdout:flush()
	end
	last_rx, last_tx = rx, tx
	socket.sleep(1)
end

#!/usr/bin/lua

-- simple traffic monitor

socket = require("socket")

local function interruptible_sleep(n)
	while n > 0 do
		local nap = math.min(n,0.05) -- keep things responsive (e.g. for SIGINT)
		local res = pcall(socket.sleep, nap) -- prevent stack trace output upon SIGINT
		if not res then
			io.write("\r")
			os.exit(0) 
		end
		n = n - nap
	end
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

local function read_stats(iflist)
	local devs, rx, tx = {}, {}, {}
	local fh = io.open("/proc/net/dev")
	for line in fh:lines() do
		local t = {}
		for w in line:gmatch("(%g+)%s+") do
			table.insert(t, w)
		end
		local dev = t[1]:match("(%g+):")
		if iflist and not iflist[dev] then dev = nil end
		if dev then	
			table.insert(devs, dev)
			rx[dev], tx[dev] = t[2], t[10] 
		end
	end
	fh:close()
	return devs, rx, tx
end

local function get_active_ifs()
	local devs = {}
	local ph = io.popen("/usr/bin/ip link show up","r")
	for line in ph:lines() do
		local name = line:match("^%d+:%s+(%g+):")
		if name then devs[name] = true end
	end
	ph:close()
	return devs
end

local last_rx, last_tx = {}, {}
local n_devs = 0
local rate = 1
local last_time

print("              RX            TX")

local iflist = {}
if arg[1] then
	iflist[arg[1]] = true
else 
	iflist = get_active_ifs()
end

while true do
	
	-- move cursor n_devs lines up
	if n_devs > 0 then
		io.write("\27["..n_devs.."A")
	end
	
	n_devs = 0
	local now = socket.gettime()
	local devs, rx, tx = read_stats(iflist)
	for _, dev in ipairs(devs) do
		local rx, tx = rx[dev], tx[dev]
		local rxdiff, txdiff = 0, 0
		-- ignore 1st run w/o previous values
		if last_rx[dev] ~= nil then
			local dt = now - last_time
			rxdiff = (rx - last_rx[dev]) / dt
			txdiff = (tx - last_tx[dev]) / dt
		end
		-- format stuff
		dev = string.format("%10s", dev)
		local r, t = fmt(rxdiff), fmt(txdiff)
		-- bold output for values > 0
		if rxdiff > 0 then r = "\27[1m" .. r .. "\27[0m" end
		if txdiff > 0 then t = "\27[1m" .. t .. "\27[0m" end
		print(dev.."  "..r.."   "..t)
		n_devs = n_devs + 1
	end
	last_rx, last_tx = rx, tx
	last_time = now
	interruptible_sleep(1/rate)
end

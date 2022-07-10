#!/usr/bin/lua

-- this script fetches the same data as the LHC announcer (https://announcer.web.cern.ch/announcer/)
-- messages for the enabled categories are displayed as desktop notification

local lgi    = require 'lgi'
local Notify = lgi.require('Notify')

local socket = require("socket")

local https  = require "ssl.https"
local ltn12  = require("ltn12")

-- enable or disable categories here
local categories = {
	beam            = true,
	collimation     = true,
	experiments     = true,
	feedback        = true,
	instrumentation = true,
	mode            = true,
	["post-mortem"] = true,
	power           = false, -- disabled by default
	rf              = true,
	surveillance    = true
}

-- message timeout in milliseconds
local min_message_timeout      = 2000
local max_message_timeout      = 5000
local message_timeout_per_line = 1000

local last_ts = 0

function sleep(n)
	-- keep the LUA interpreter responsive in case ctrl+c is pressed
	local maxwait = 0.1
	while n > 0 do
		local wait = math.min(maxwait, n)
		socket.select(nil, nil, wait)
		n = n - wait
	end
end

function send_notify(body, n_lines)
	local msg=Notify.Notification.new("LHC",body)
	local timeout = n_lines * message_timeout_per_line
	timeout = math.min(timeout, max_message_timeout)
	timeout = math.max(timeout, min_message_timeout)
	msg:set_timeout(timeout)
	msg:show()
end

function fetch_update()
	local body, res = {}, {}
	https.request{url="https://announcer.web.cern.ch/announcer/get_events.pl?"..last_ts, sink = ltn12.sink.table(body)}
	body = table.concat(body,"")
	-- redneck approach to parsing the XML data:
	for event in string.gmatch(body, '<event ([%g ]+) />') do
		local evt = {}
		--print(event)
		-- collect all keys/values
		for k,v in string.gmatch(event,'(%l+)="([%g ]-)"') do
			evt[k]=v
		end
		-- update last ts
		last_ts = math.max(last_ts, evt.time+0)
		-- only add enabled categories
		if categories[evt.category] then table.insert(res, evt) end
	end
	last_ts = string.format("%d",last_ts) -- fix large number formatting
	return res
end

Notify.init("LHC.notify")

-- main loop
while true do
	local evts = fetch_update()
	local msg = ""
	local console_buf = ""
	for i=#evts,1,-1 do
		if #msg>0 then msg = msg .. "\n" end
		local line = "[" .. evts[i].category .. "] " .. evts[i].message
		local datestr = os.date("%x %X ",evts[i].time/1000000)
		msg = msg .. line
		console_buf = datestr .. line .. "\n" .. console_buf
	end
	if #msg > 0 then
		send_notify(msg, #evts)
		io.write(console_buf)
	end
	sleep(1)
end

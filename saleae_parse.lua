#!/usr/bin/lua

require "utils"

function load_file(fn,ch)
	local fh = io.open(fn)
	local line = fh:read("*l")
	--print(line)
	local res = {}
	for line in fh:lines() do
		--print(line)
		if string.find(line,",Error") then
		--
		else
			local t, v = string.match(line,"(%d+.%d+),0x(%x%x)")
			t, v = t+0, tonumber(v,16)
			--print(t,string.char(v))
			table.insert(res,{t=t,d=v,ch=ch})
		end
	end
	return res
end

function merge_logs(t1, t2)
	local idx1, idx2 = 1, 1
	local res = {}
	
	-- handle all entries
	while idx1 <= #t1 or idx2 <= #t2 do
		local e1, e2 = t1[idx1], t2[idx2]
		local cmp
		-- compare timestamps
		if e1 == nil then
			cmp = false
		elseif e2 == nil then
			cmp = true
		else
			cmp = e1.t <= e2.t
		end
		-- select next entry
		local ne, inc1, inc2 = e1, 1, 0
		if not cmp then
			ne, inc1, inc2 = e2, 0, 1
		end
		table.insert(res, ne)
		--print(ne.t,ne.ch)
		idx1, idx2 = idx1 + inc1, idx2 + inc2
	end
	return res
end

function parse_log(t,delta)
	local curr_ch, buf, st, lt = nil, "", nil, 0
	local res = {}
	for i=1,#t do
		local e = t[i]
		if e.ch ~= curr_ch or e.t > (lt + delta) then
			if #buf > 0 then
				table.insert(res,{ch=curr_ch, st=st, data=buf})
			end
			curr_ch, st, buf = e.ch, e.t, ""
		end
		buf = buf .. string.char(e.d)
		lt = e.t
	end
	return res
end

function asciiguard(str)
	local res = ""
	for i=1,#str do
		local c = string.byte(str,i)
		if c < 0x20 or c > 0x7e then
			c = string.byte('.')
		end
		res = res .. string.char(c)
	end
	return res
end

function xpand(s,n)
	local res = s
	for i=#res,n do
		res = res .. " "
	end
	return res
end

function dump_msgs(m)
	for i=1,#m do
		local e = m[i]
		print(e.ch,string.format("%5.3f",e.st),xpand(tohex(e.data),110),asciiguard(e.data))
	end
end

local f1, f2 = load_file(arg[1],"<"), load_file(arg[2],">")
local mt = merge_logs(f1, f2)
local msgs = parse_log(mt,0.01) -- 10ms end-of-message timeout
dump_msgs(msgs)

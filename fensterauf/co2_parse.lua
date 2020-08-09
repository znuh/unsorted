local last_ts = 0
local buf = ""

function convert(buf)
	local res = ""
	local nibble = 0
	local n_cnt = 0
	for i=1,#buf do
		local val = string.byte(buf,i) - 0x30
		nibble = nibble * 2
		nibble = nibble + val
		n_cnt = n_cnt + 1
		if n_cnt == 4 then
			res = res .. string.format("%x", nibble)
			nibble = 0
			n_cnt = 0
		end
	end
	return res
end

for line in io.lines() do
	local ts, desc, txt = line:match("(%g+),(.*),'(%d)'")
	if txt then
		ts = ts + 0
		if ts - last_ts > 0.04 then
			buf = ""
			if ts - last_ts > 1 then 
				 print() 
			end
		end
		buf = buf .. txt
		-- also see https://hackaday.io/project/5301/logs
		if #buf == 40 then
			local bin = buf
			local hex = convert(buf)
			local val = tonumber(string.sub(hex,3,6),16)
			io.write(hex.." ")
			if hex:find("^50") then
				io.write("(CO2: ",val,"ppm) ")
			elseif hex:find("^41") then
				io.write("(RH: ",math.floor(val/100),"%) ")
			elseif hex:find("^42") then
				io.write("(T: ",string.format("%.1f",val/16.0-273.15),"°C) ")
			end
			buf = ""
		end
		last_ts = ts
	end
end

print()

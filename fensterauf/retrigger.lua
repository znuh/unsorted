
function scale(i)
	local factor = 1
	local maxval, mult = math.floor(0x1f/factor), 8*factor
	i = i - 4
	i = math.min(i, maxval)
	i = maxval - i
	i = i * mult
	return i
end

function retrigger(i)
	i = math.floor(i/256)
	-- apply scaling formula
	i = scale(i)
	i = i * 8 -- mult by 8 seconds (watchdog timer)
	local m,s = math.floor(i/60), i%60
	return string.format("%02d:%02d (%3d)",m,s,i/8), i/8 == 0
end

for i=1024,8960,256 do
	local res, zero = retrigger(i)
	print(i,res)
	if zero then return end
end

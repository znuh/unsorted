require "spacenav"
dofile("../utils.lua")
sn = spacenav()
while true do
	local t = sn:get_vals()
	print(t.x,t.y,t.z,t.rx,t.ry,t.rz,t.btn_l,t.btn_r)
	if t.btn_l == true and t.btn_r == true then break end
	sleep(0.03)
end
sn = nil

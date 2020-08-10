#!/usr/bin/lua
--require 'utils'

if socket == nil then socket = require("socket") end

function sleep(n)
	-- keep the LUA interpreter responsive in case ctrl+c is pressed
	while n >= 1 do
		socket.select(nil, nil, 1)
		n = n - 1
	end
	if n>0 then
		socket.select(nil, nil, n)
	end
end

local I2C = require('periphery').I2C
local i2c = I2C("/dev/i2c-10")

local t67_addr = 0x15

local T67 = {
	FW_REV                 = 5001,
	STATUS                 = 5002,
	GAS_PPM	               = 5003,
	RESET_DEVICE           = 1000,
	START_SINGLE_POINT_CAL = 1004,
	SLAVE_ADDRESS          = 4005,
	ABC_LOGIC_EN_DISABLE   = 1006
}

function t67_read(reg,delay,n_)
	local reg_msb, reg_lsb = math.floor(reg/256), reg%256
	local n = n_ or 1
	local n_msb, n_lsb = math.floor(n/256), n%256
	local msgs = { {4, reg_msb, reg_lsb, n_msb, n_lsb}, {0, 0, flags = I2C.I2C_M_RD} }
	for i=3, 2+n*2 do
		msgs[2][i]=0
	end
	local res1 = i2c:transfer(t67_addr, {msgs[1]})
	sleep(delay or 0.01)
	local res = i2c:transfer(t67_addr, {msgs[2]})
	--dump_table(res)
	assert(msgs[2][1] == 4)
	local ret = {}
	for i=1,n do
		ret[i] = msgs[2][1+i*2]*256 + msgs[2][1+i*2+1]
	end
	return unpack(ret)
end

local fw, status, co2_ppm = t67_read(T67.FW_REV), t67_read(T67.STATUS), t67_read(T67.GAS_PPM)

print(string.format("FW REV : 0x%04x",fw))
print(string.format("STATUS : 0x%04x",status))
print(string.format("CO2_PPM: %d",co2_ppm))

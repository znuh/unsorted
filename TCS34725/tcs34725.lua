#!/usr/bin/lua

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

function gettime()
	return socket.gettime()
end

local I2C = require('periphery').I2C
local i2c = I2C("/dev/i2c-10")

local tcs_addr = 0x29

local TCS = {
	CMD 			= 0x80,
	-- registers
	ENABLE			= 0,
	ENABLE_AIEN		= 0x10,
	ENABLE_WEN		= 0x08,
	ENABLE_AEN		= 0x02,
	ENABLE_PON		= 0x01,
	ATIME			= 0x01,
	WTIME			= 0x03,
	WTIME_2_4MS		= 0xff,
	WTIME_204MS		= 0xab,
	WTIME_614MS		= 0x00,
	AILTL			= 0x04,
	AILTH			= 0x05,
	AIHTL			= 0x06,
	AIHTH			= 0x07,
	PERS			= 0x0c,
	PERS_NONE		= 0x00,
	PERS_1_CYCLE    = 0x01,
	PERS_2_CYCLE	= 0x02,
	PERS_3_CYCLE	= 0x03,
	PERS_5_CYCLE	= 0x04,
	PERS_10_CYCLE	= 0x05,
	PERS_15_CYCLE	= 0x06,
	PERS_20_CYCLE	= 0x07,
	PERS_25_CYCLE	= 0x08,
	PERS_30_CYCLE	= 0x09,
	PERS_35_CYCLE	= 0x0a,
	PERS_40_CYCLE	= 0x0b,
	PERS_45_CYCLE	= 0x0c,
	PERS_50_CYCLE	= 0x0d,
	PERS_55_CYCLE	= 0x0e,
	PERS_60_CYCLE	= 0x0f,
	CONFIG			= 0x0d,
	CONFIG_WLONG	= 0x02,
	CONTROL			= 0x0f,
	ID				= 0x12,
	STATUS			= 0x13,
	STATUS_AINT		= 0x10,
	STATUS_AVALID	= 0x01,
	CDATAL			= 0x14,
	CDATAH			= 0x15,
	RDATAL			= 0x16,
	RDATAH			= 0x17,
	GDATAL			= 0x18,
	GDATAH			= 0x19,
	BDATAL			= 0x1a,
	BDATAH			= 0x1b,
	
	INTEG_2_4MS		= 0xff,
	INTEG_24MS		= 0xf6,
	INTEG_50MS		= 0xeb,
	INTEG_101MS		= 0xd5,
	INTEG_154MS		= 0xc0,
	INTEG_700MS		= 0x00,
	
	GAIN_1X			= 0x00,
	GAIN_4X			= 0x01,
	GAIN_16X		= 0x02,
	GAIN_60X		= 0x03
}

function tcs_write8(reg, val)
	local msgs = { { reg + TCS.CMD, val} }
	return i2c:transfer(tcs_addr, msgs)
end

function tcs_read8(reg)
	local msgs = { { reg + TCS.CMD}, {0x00, flags = I2C.I2C_M_RD} }
	local res = i2c:transfer(tcs_addr, msgs)
	return msgs[2][1]
end

function tcs_read16(reg)
	local msgs = { { reg + TCS.CMD}, {0, 0, flags = I2C.I2C_M_RD} }
	local res = i2c:transfer(tcs_addr, msgs)
	local val = msgs[2][1] + msgs[2][2]*256
	return val
end

local integration_time = TCS.INTEG_700MS --2_4MS

function tcs_init()
	tcs_write8(TCS.ATIME, integration_time)
	tcs_write8(TCS.CONTROL, TCS.GAIN_1X)
	tcs_write8(TCS.ENABLE, TCS.ENABLE_PON + TCS.ENABLE_AIEN) -- ENABLE_AIEN disables LED on Adafruit boards
	sleep(0.003)
	tcs_write8(TCS.ENABLE, TCS.ENABLE_PON + TCS.ENABLE_AEN + TCS.ENABLE_AIEN)
	sleep(0.7)
	
end

function tcs_getCRGB()
	local msgs = { { TCS.CDATAL + TCS.CMD}, {0, 0, 0, 0, 0, 0, 0, 0, flags = I2C.I2C_M_RD} }
	local res = i2c:transfer(tcs_addr, msgs)
	local c = msgs[2][1] + msgs[2][2]*256
	local r = msgs[2][3] + msgs[2][4]*256
	local g = msgs[2][5] + msgs[2][6]*256
	local b = msgs[2][7] + msgs[2][8]*256
	print('CRGB:',c,r,g,b)
	return c, r, g, b
end

function tcs_getCCT_mccamy()
	local c,r,g,b = tcs_getCRGB()
	--print(c,r,g,b)
	local X = (-0.14282 * r) + (1.54924 * g) + (-0.95641 * b)
	local Y = (-0.32466 * r) + (1.57837 * g) + (-0.73191 * b)
	local Z = (-0.68202 * r) + (0.77073 * g) + (0.56332 * b)
	local xc = X / (X + Y + Z)
	local yc = Y / (X + Y + Z)
	local n = (xc - 0.3320) / (0.1858 - yc)
	local cct = (449.0 * n^3) + (3525.0 * n^2) + (6823.3 * n) + 5520.33
	return cct
end

function tcs_getCCT()
	local c,r,g,b = tcs_getCRGB()
	--print(c,r,g,b)
	if c == 0 then return 0 end
	local sat = 65535
	if 256 - integration_time <= 63 then 
		sat = 1024 * (256 - integration_time)
		sat = sat - (sat / 4)
	end
	
	if c >= sat then return 0 end
	
	local ir = 0
	if r + g + b > c then ir = (r + g + b - c)/2 end
	
	local r2 = r - ir
	local b2 = b - ir
	
	if r2 == 0 then return 0 end
	
	local cct = (3810 * b2) / r2 + 1391;
	return math.floor(cct+0.5)
end

tcs_init()
print(tcs_getCCT()..' K')

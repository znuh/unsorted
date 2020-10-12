#!/usr/bin/lua
s = require 'socket'
s.sleep(arg[1]*60)
while true do os.execute('play -n -c1 synth 0.1 sine 800'); s.sleep(1) end

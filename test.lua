local xe = require "xemsg"

-- socket/bind/connect test
local server = assert( xe.socket(xe.AF_SP, xe.NN_PULL) )
local e1 = assert( server:bind("tcp://*:4321") )

local client = assert( xe.socket(xe.AF_SP, xe.NN_PUSH) )
local e2 = assert( client:connect("tcp://localhost:4321") )

-- send/recv test
assert( client:send("Hello World!") )
assert( assert( server:recv() ) == "Hello World!" )

-- poll test
assert( client:send("Another test") )
assert( client:send("Another test 2") )
local fds = { {fd=server, events=xe.NN_POLLIN } }
assert( assert( xe.poll(fds, 1000) ) == 1 ) -- timeout=1000 ms
assert( fds[1].revents == fds[1].events )
assert( assert( server:recv() ) == "Another test" )
assert( assert( server:recv() ) == "Another test 2" )
assert( server:shutdown(e1) )
assert( client:shutdown(e2) )

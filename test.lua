local xe = require "xemsg"

-- socket/bind/connect test
local server = assert( xe.socket(xe.AF_SP, xe.NN_PULL) )
assert( xe.bind(server, "tcp://*:4321") )

local client = assert( xe.socket(xe.AF_SP, xe.NN_PUSH) )
assert( xe.connect(client, "tcp://localhost:4321") )

-- send/recv test
assert( xe.send(client, "Hello World!", 0) )
assert( assert( xe.recv(server,0) ) == "Hello World!" )

-- poll test
assert( xe.send(client, "Another test", 0) )
assert( xe.send(client, "Another test 2", 0) )
local fds = { {fd=server, events=xe.NN_POLLIN } }
assert( assert( xe.poll(fds, 0.01) ) == 1 )
assert( fds[1].revents == fds[1].events )
assert( assert( xe.recv(server, 0) ) == "Another test" )
assert( assert( xe.recv(server, 0) ) == "Another test 2" )
xe.term()

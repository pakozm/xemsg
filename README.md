# Xemsg!

Plain binding of [nanomsg](https://github.com/nanomsg/nanomsg) to Lua (no FFI),
working for Lua 5.1 and Lua 5.2. The name Xemsg! is a valencian joke: we
normally use the word *nano* to refer to our budies, and in some cases it is
preceded by *xe* word to form the following interjection: **Xe nano!**

This software has been tested with **nanomsg v1.0.0**.

## Why another nanomsg binding?

All the requirements we need are not fulfilled in current bindings. The
requirements here are: a simple library for gluing nanomsg and Lua without any
other dependency (no LuaFFI), besides Lua 5.1/5.2 and Unix like OS.

Currently exists three other well known bindings:

- Official one, for LuaJIT, mainly developed by Pierre Chapuis:
  [luajit-nanomsg](https://github.com/nanomsg/luajit-nanomsg)

- For Lua 5.1 using LuaFFI, developed by Neopallium:
  [lua-nanomsg](https://github.com/Neopallium/lua-nanomsg)

- For Lua 5.2 in Windows, developed by Andrew Starks and Christian Bechette:
  [nml](https://github.com/trms/nml)

## Installation

From a terminal:

```
$ git clone https://github.com/pakozm/xemsg.git
$ cd xemsg
$ make
$ sudo make install
```

### Compilation details

The makefile automatically detects which platform and Lua version are you
using, so for compilation you just need to do:

```
$ make
```

You can force the platform compilation by using `$ make Linux` or `$ make Darwin`.
Additionally, you can force the Lua version by doing:

```
$ make LUAPKG=lua5.2
$ sudo make LUAPKG=lua5.2 install
```

where `lua5.2` can be replaced by `lua5.1` and `luajit`.

## Use

Xemsg! has been developed to be a plain binding of nanomsg library, but
following some common patterns in Lua. For instance, all functions return one or
two values. In case of success, one value is returned with the expected content
by the user. In case of failure, a `nil` is returned as first value and an error
message as second value, allowing to wrap almost all function calls around an
assert. A third argument with the error number is also returned, allowing to
perform different actions depending in the error type.

```Lua
> xe = require "xemsg"
> s = assert( xe.socket(xe.NN_PULL) )
```

Xemsg! is at least as easier as nanomsg. The following is a simple server
example which opens a TCP port, waits for messages and prints them into screen:

```Lua
> s = assert( xe.socket(xe.NN_PULL) )
> x = assert( s:bind("tcp://*:4321") )
> while true do print( s:recv() ) end
```

And its corresponding client which sends a sequence of messages:

```Lua
> s = assert( xe.socket(xe.NN_PUSH) )
> x = assert( s:connect("tcp://localhost:4321") )
> while true do assert( s:send("message") ) end
```

Notice that SP sockets are Lua objects, being full userdatas with a common
metatable. Similarly as Lua does with `string` and their metatable, all SP
objects has as a reference to `xe` table, so, as long as a function receives a
SP socket object in its first argument, the function can be called as a method
using `:` operator for simplicity.

### Reference

This documentation is based on the
[official nanomsg reference](http://nanomsg.org/documentation.html).

**socket([domain,] protocol)**

Opens an SP socket.

`s [, e_msg, e_num] = xe.socket([domain,] protocol)`

Inputs:

- domain: an optional number, `xe.AF_SP` for standard SP socket, `xe.AF_SP_RAW`
  for a raw one. If not given, it is `xe.AF_SP` by default.
- protocol: a number, more information in nanomsg documentation:
  [pubsub](http://nanomsg.org/v0.6/nn_pubsub.7.html),
  [reqprep](http://nanomsg.org/v0.6/nn_reqrep.7.html),
  [pipeline](http://nanomsg.org/v0.6/nn_pipeline.7.html),
  [survey](http://nanomsg.org/v0.6/nn_survey.7.html),
  [bus](http://nanomsg.org/v0.6/nn_bus.7.html).

Outputs:

- s: a SP socket object, `nil` in case of failure.
- e_msg: string with error message only in case of failure.
- e_num: error number, only in case of failure.

Notice that SP sockets are Lua objects, being full userdatas with a common
metatable. Similarly as Lua does with `string` and their metatable, all SP
objects has as a reference to `xe` table, so, as long as a function receives a
SP socket object in its first argument, the function can be called as a method
using `:` operator for simplicity.

**close(s)**

Closes an SP socket.

`ok [, e_msg, e_num] = xe.close(s)`

Inputs:

- s: a SP socket object, as returned by `xe.socket`.

Outputs:

- ok: `true` or `nil` value indicating success or failure.
- e_msg: string with error message, only in case of failure.
- e_num: error number, only in case of failure.

**setsockopt(s, level, option, optval)**

Sets a socket option.

`ok [, e_msg, e_num] = xe.setsockopt(s, level, option, optval)`

Inputs:

- s: SP socket object.
- level: a number indicating the protocol level related with the option.
  Use `xe.NN_SOL_SOCKET` for generic socket-level options;
  socket type for socket-type-specific options (e.g. `xe.NN_SUB`);
  ID of the transport for transport-specific options (e.g. `xe.NN_TCP`).
- option: number indicating which option. See
  [nn_setsockopt](http://nanomsg.org/v0.6/nn_setsockopt.3.html) for available
  options values.
- optval: number or string depending on the option field. See
  [nn_setsockopt](http://nanomsg.org/v0.6/nn_setsockopt.3.html) for more
  details.

Outputs:

- ok: `true` or `nil` value indicating success or failure.
- e_msg: string with error message, only in case of failure.
- e_num: error number, only in case of failure.

**getsockopt(s, level, option)**

Retrieves a socket option.

`optval [, e_msg, e_num] = xe.getsockopt(s, level, option)`

Inputs:

- s: SP socket object.
- level: a number indicating the protocol level related with the option.
  Use `xe.NN_SOL_SOCKET` for generic socket-level options;
  socket type for socket-type-specific options (e.g. `xe.NN_SUB`);
  ID of the transport for transport-specific options (e.g. `xe.NN_TCP`).
- option: number indicating which option. See
  [nn_setsockopt](http://nanomsg.org/v0.6/nn_setsockopt.3.html) for available
  options values.

Outputs:

- optval: number or string depending on the option field. See
  [nn_setsockopt](http://nanomsg.org/v0.6/nn_setsockopt.3.html) for more
  details. This value is `nil` in case of failure.
- e_msg: string with error message, only in case of failure.
- e_num: error number, only in case of failure.

**bind(s, addr)**

Adds a local endpoint to the socket.

`id [, e_msg, e_num] = xe.bind(s, addr)`

Inputs:

- s: SP socket object.
- addr: a string formed by two parts: transport://address. The transport
  indicates which protocol to use. Address meaning depends in transport
  protocol.

Outputs:

- id: number with the endpoint identifier. It can be used with `xe.shutdown()`
  to remove the endpoint. This value is `nil` in case of failure.
- e_msg: string with error message, only in case of failure.
- e_num: error number, only in case of failure.

The following transports are known for v0.6 of nanomsg: [inproc](http://nanomsg.org/v0.6/nn_inproc.7.html),
[ipc](http://nanomsg.org/v0.6/nn_ipc.7.html), [tcp](http://nanomsg.org/v0.6/nn_tcp.7.html),
[tcpmux](http://nanomsg.org/v0.6/nn_tcpmux.7.html), [WebSocket](http://nanomsg.org/v0.6/nn_ws.7.html).

The addr parameter has a maximum length specified by `xe.NN_SOCKADDR_MAX`.

`xe.bind()` and `xe.connect()` may be called multiple times with the same socket,
allowing to communicate via multiple heterogeneous endpoints.

**connect(s, addr)**

Adds a remote endpoint to the socket.

`id [, e_msg, e_num] = xe.connect(s, addr)`

Inputs:

- s: SP socket object.
- addr: a string formed by two parts: transport://address. The transport
  indicates which protocol to use. Address meaning depends in transport
  protocol.

Outputs:

- id: number with the endpoint identifier. It can be used with `xe.shutdown()`
  to remove the endpoint. This value is `nil` in case of failure.
- e_msg: string with error message, only in case of failure.
- e_num: error number, only in case of failure.

See `xe.bind()` for more details about addr parameter.

**shutdown(s, endpoint)**

Removes an endpoint from the socket.

`ok [, e_msg, e_num] = xe.shutdown(s, endpoint)`

Inputs:

- s: SP socket object.
- endpoint: a number with the endpoint identifier as returned by `xe.bind()` or
  `xe.connect()`.

Outputs:

- ok: `true` or `nil` value indicating success or failure.
- e_msg: string with error message, only in case of failure.
- e_num: error number, only in case of failure.

**send(s, buf [, flags])**

Sends a message.

`size [, e_msg, e_num] = xe.send(s, buf [, flags])`

Inputs:

- s: SP socket object.
- buf: a string with the message data.
- flags: an optional number with a bit set of flags. Normally it is `0` or
  `xe.NN_DONTWAIT`. By default it is set to 0.

Outputs:

- size: the number of bytes in the message, or `nil` in case of failure.
- e_msg: string with error message, only in case of failure.
- e_num: error number, only in case of failure.

**recv(s [, n] [, flags])**

Receives a message.

`msg [, e_msg, e_num] = xe.recv(s)`

`msg [, e_msg, e_num] = xe.recv(s, flags)`

`msg [, e_msg, e_num] = xe.recv(s, n, flags)`

Inputs:

- s: SP socket object.
- n: an optional number indicating the expected size of the message. When this
  parameter is present, a lua_Buffer is given to nanomsg for reducing memory
  overhead. If not given, the string returned by nanomsg will be duplicated into
  Lua stack.
- flags: an optional number with a bit set of flags. Normally it is `0` or
  `xe.NN_DONTWAIT`. By default it is set to 0.

Outputs:

- msg: a string with the received message or `nil` in case of failure.
- e_msg: string with error message, only in case of failure.
- e_num: error number, only in case of failure.


**poll(fds [, timeout])**

Multiplexing several sockets for events.

`n [, e_msg, e_num] = xe.poll(fds [, timeout])`

Inputs:

- fds: a table used as input/output parameter.
- timeout: optional number of mili-seconds before timeout, by default it is 0.

Outputs:

- n: a number indicating how many SP sockets are ready for the indicated events.
  It is 0 in case of timeout, and `nil` in case of failure.
- e_msg: string with error message, only in case of failure.
- e_num: error number, only in case of failure.

The table `fds` is used as input and output parameter. It contains the events
you want to be forewarned of. The table is an array of tables with the following
structure:

```
fds = {
  {
    fd=SP socket object,
    events=bit set number,
  },
  {
    fd=another SP socket object,
    events=another bit set number,
  },
  ...
}
```

The field `events` contains a binary combination of 0 and values `xe.NN_POLLIN`,
`xe.NN_POLLOUT`. When n>0, this table is updated with an additional field:

```
fds = {
  {
    fd=SP socket object,
    events=bit set number,
    revents=bit set number,
  },
  {
    fd=another SP socket object,
    events=another bit set number,
    revents=another bit set number,
  },
  ...
}
```

The field `revents` indicates which of the `events` are ready, so it will be
a binary combination of 0 and values `xe.NN_POLLIN`, `xe.NN_POLLOUT`.

**device(s1 [, s2])**

Starts a device.

`nil,e_msg,e_num = xe.device(s1 [, s2])`

Inputs:

- s1: SP socket object.
- s2: SP socket object, optional, if not given the device will be set in
  *loopback* mode.

Outputs:

- nil: indicating failure.
- e_msg: string with error message, returned always.
- e_num: error number, returned always.

This function loops and sends and messages received from s1 to s2 and
viceversa. If only one socket is given, it works in *loopback* mode: it loops
and sends any messages received from the socket back to itself.

**term()**

Notifies all sockets about process termination.

`xe.term()`

After calling this function all sockets and endpoints will be terminated.

**get()**

Pushes in Lua the integer value of the SP socket.

`n = xe.get(s)`

Inputs:

- s: the SP socket object.

Outputs:

- n: the number associated with SP socket s.

**release()**

Pushes in Lua the integer value of the SP socket and assigns not valid value to
the userdata. Therefore, the **userdata cannot be used after** calling this
function.

`n = xe.release(s)`

Inputs:

- s: the SP socket object.

Outputs:

- n: the number associated with SP socket s.

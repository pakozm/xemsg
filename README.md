# Xemsg!

Plain binding of [nanomsg](https://github.com/nanomsg/nanomsg) to Lua 5.2 (no
FFI). The name Xemsg! is a valencian joke: we normally use the word *nano* to
refer to our budies, and in some cases it is preceded by *xe* word to form the
following interjection: **Xe nano!**

## Installation

From a terminal:

```
$ git clone https://github.com/pakozm/xemsg.git
$ cd xemsg
$ make
$ sudo make install
```

## Use

Xemsg! has been developed to be a plain binding of nanomsg library, but
following some common patterns in Lua. For instance, all functions return one or
two values. In case of success, one value is returned with the expected content
by the user. In case of failure, a `nil` is returned as first value and an error
message as second value, allowing to wrap almost all function calls around an
assert:

```Lua
> xe = require "xemsg"
> s = assert( xe.socket(xe.AF_SP, xe.NN_PULL) )
```

Xemsg! is at least as easier as nanomsg. The following is a simple server
example which opens a TCP port, waits for messages and prints them into screen:

```Lua
> s = assert( xe.socket(xe.AF_SP, xe.NN_PULL) )
> x = assert( xe.bind(s, "tcp://*:4321") )
> while true do print(xe.recv(s, 0)) end
```

### Reference

**socket(domain, protocol)**

Opens an SP socket.

`id [, error_msg] = xe.socket(domain, protocol)`

- domain: a number, `xe.AF_SP` for standard SP socket, `xe.AF_SP_RAW` for a raw one.
- protocol: a number, more information in nanomsg documentation:
  [pubsub](http://nanomsg.org/v0.6/nn_pubsub.7.html),
  [reqprep](http://nanomsg.org/v0.6/nn_reqrep.7.html),
  [pipeline](http://nanomsg.org/v0.6/nn_pipeline.7.html),
  [survey](http://nanomsg.org/v0.6/nn_survey.7.html),
  [bus](http://nanomsg.org/v0.6/nn_bus.7.html).

It returns the socket id in case of success or `nil` followed by an error
message in case of error.

**close(s)**

Close an SP socket
nn_close(3)

**setsockopt(s, level, option, optval)**

Set a socket option
nn_setsockopt(3)

**getsockopt(s, level, option)**

Retrieve a socket option
nn_getsockopt(3)

**bind(s, addr)**

Add a local endpoint to the socket
nn_bind(3)

**connect(s, addr)**

Add a remote endpoint to the socket
nn_connect(3)

**shutdown(s, how)**

Remove an endpoint from the socket
nn_shutdown(3)

**send(s, buf, flags)**

Send a message
nn_send(3)

**recv(s, [n,] flags)**

Receive a message
nn_recv(3)

**poll(fds, timeout)**

Multiplexing
nn_poll(3)

**device(s1, s2)**

Start a device
nn_device(3)

**term()**

Notify all sockets about process termination
nn_term(3)

## Other known bindings

- Official for LuaJIT, mainly developed by Pierre Chapuis:
  [luajit-nanomsg](https://github.com/nanomsg/luajit-nanomsg)

- For Lua5.1 using LuaFFI, developed by Neopallium:
  [lua-nanomsg](https://github.com/Neopallium/lua-nanomsg)

- For Lua 5.2 in Windows, developed by Andrew Starks and Christian Bechette:
  [nml](https://github.com/trms/nml)

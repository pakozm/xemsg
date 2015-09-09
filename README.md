# Xemsg!

Plain binding of [nanomsg](https://github.com/nanomsg/nanomsg) to Lua 5.2 (no
FFI). The name Xemsg! is a valencian joke: we normally use the word *nano* to
refer to our budies, and in some cases it is preceded by *xe* word to form the
following interjection: **Xe nano!**

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

## Other known bindings

- Official for LuaJIT, mainly developed by Pierre Chapuis:
  [luajit-nanomsg](https://github.com/nanomsg/luajit-nanomsg)

- For Lua5.1 using LuaFFI, developed by Neopallium:
  [lua-nanomsg](https://github.com/Neopallium/lua-nanomsg)

- For Lua 5.2 in Windows, developed by Andrew Starks and Christian Bechette:
  [nml](https://github.com/trms/nml)

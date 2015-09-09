package = "xemsg"
version = "scm-0"

source = {
  url = "https://github.com/pakozm/xemsg/archive/master.zip",
  dir = "xemsg-master",
}

description = {
  summary = "Lua client library for mongodb",
  detailed = [[
      xemsg: Lua binding for nanomsg library
   ]],
  homepage = "https://github.com/pakozm/xemsg",
  license = "MIT/X11",
}

dependencies = {
  "lua >= 5.2",
}

external_dependencies = {
  LIBNANOMSG = {
    header = "/usr/include/nanomsg/nn.h",
    library = "libnanomsg",
  },
}

build = {
  type = "make",
  copy_directories = {},
  install_pass = false,
  install = { lib = { "xemsg.so" } },
}

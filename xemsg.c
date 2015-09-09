/*
 * Copyright (c) 2015 Francisco Zamora-Martinez (pakozm@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <nanomsg/nn.h>
#include <nanomsg/bus.h>
#include <nanomsg/pair.h>
#include <nanomsg/pipeline.h>
#include <nanomsg/pubsub.h>
#include <nanomsg/reqrep.h>
#include <nanomsg/survey.h>

#define XEMSG_NAME_FIELD    "_NAME"
#define XEMSG_NAME          "xemsg"
#define XEMSG_VERSION_FIELD "_VERSION"
#define XEMSG_VERSION       "0.1"

/**
 * Returns 1 if the value 'n' at stack 'L' is an integer, and stores its value
 * at the given pointer 'r'. Otherwise, it returns 0 and 'r' is not used.
 */
int xelua_isinteger(lua_State *L, int n, int *r) {
  if (lua_isnumber(L,n)) {
    double aux = lua_tonumber(L, n);
    *r = aux;
    if ((double)*r == aux) return 1;
  }
  return 0;
}

/**
 * Returns 1 if the value 'n' at stack 'L' is a short integer, and stores its
 * value at the given pointer 'r'. Otherwise, it returns 0 and 'r' is not used.
 */
int xelua_isshort(lua_State *L, int n, short *r) {
  if (lua_isnumber(L,n)) {
    double aux = lua_tonumber(L, n);
    *r = aux;
    if ((double)*r == aux) return 1;
  }
  return 0;
}

int check_error(lua_State *L, int r) {
  if (r < 0) {
    int err = nn_errno();
    lua_pushnil(L);
    lua_pushstring(L, nn_strerror(err));
    return 2;
  }
  return 0;
}

/**
 * Checks a returned int and pushes it into Lua in case of success, otherwise
 * pushes a nil value followed by the error string.
 */
int check_int(lua_State *L, int r) {
  int nret = check_error(L, r);
  if (nret) return nret;
  lua_pushinteger(L, r);
  return 1;
}

/**
 * Checks a returned int and pushes true into Lua in case of success, otherwise
 * pushes a nil value followed by the error string.
 */
int check_succ(lua_State *L, int r) {
  int nret = check_error(L, r);
  if (nret) return nret;
  lua_pushboolean(L, 1);
  return 1; 
}

/***************************************************************************/

/* TODO:

   NN_EXPORT int nn_recvmsg (int s, struct nn_msghdr *msghdr, int flags);
   int nn_sendmsg (int s, const struct nn_msghdr *msghdr, int flags);
   void *nn_allocmsg (size_t size, int type);
   void *nn_reallocmsg (void *msg, size_t size);
   int nn_freemsg (void *msg);
   struct nn_cmsghdr *NN_CMSG_FIRSTHDR(struct nn_msghdr *hdr);
   struct nn_cmsghdr *NN_CMSG_NXTHDR(struct nn_msghdr *hdr, struct nn_cmsghdr *cmsg);
   unsigned char *NN_CMSG_DATA(struct nn_cmsghdr *cmsg);
   size_t NN_CMSG_SPACE(size_t len);
   size_t NN_CMSG_LEN(size_t len);
*/

/**
 * id,error_msg = xemsg.bind(s, addr) 
 */
int xemsg_bind(lua_State *L) {
  return check_int(L, nn_bind( luaL_checkinteger(L, 1),
                               luaL_checkstring(L,2) ));
}

/**
 * ok,error_msg = xemsg.close(s)
 */
int xemsg_close(lua_State *L) {
  return check_succ(L, nn_close( luaL_checkinteger(L, 1) ));
}

/**
 * id,error_msg = xemsg.connect(s, addr)
 */
int xemsg_connect(lua_State *L) {
  return check_int(L, nn_connect( luaL_checkinteger(L, 1),
                                  luaL_checkstring(L, 2) ));
}

/**
 * nil,error_msg = xemsg.device(s1[, s2])
 */
int xemsg_device(lua_State *L) {
  return check_succ(L, nn_device( luaL_checkinteger(L, 1),
                                  luaL_optinteger(L, 2, -1) ));
}

/**
 * optval,error_msg = xemsg.getsockopt(s, level, option)
 */
int xemsg_getsockopt(lua_State *L) {
  void *optval = NULL;
  size_t optvallen;
  int option = luaL_checkinteger(L, 3);
  int r = nn_getsockopt( luaL_checkinteger(L, 1),
                         luaL_checkinteger(L, 2),
                         option,
                         optval,
                         &optvallen );
  int nret = check_error(L, r);
  if (nret) return nret; /* error case */
  /* the value optval depends on option value, check it and push into Lua the
     corresponding result */
  switch(option) {
  case NN_SOCKET_NAME: lua_pushlstring(L, optval, optvallen); break;
  default: lua_pushinteger(L, *((int*)optval)); break;
  }
  return 1;
}

/**
 * n,error_msg = xemsg_poll({ {fd=fd, events=events}, {fd=fd, events=events}, ...}, timeout)
 */
int xemsg_poll(lua_State *L) {
  double seconds;
  struct nn_pollfd *fds;
  int n = luaL_len(L,1), i, milis, r, nret;
  /* alloc memory for pollfd array, initializing it with the data of Lua table
     at position 1 in the stack */
  fds = (struct nn_pollfd*)malloc(sizeof(struct nn_pollfd)*n);
  for (i=1; i<=n; ++i) {
    lua_rawgeti(L, 1, i);
    lua_getfield(L, -1, "fd");
    lua_getfield(L, -2, "events");
    if (!xelua_isshort(L, -1, &fds[i-1].events) ||
        !xelua_isinteger(L, -2, &fds[i-1].fd)) {
      free(fds);
      return luaL_error(L, "Expected integer values");
    }
    lua_pop(L, 3);
  }
  /* timeout is given as a real number of seconds, read it from Lua and convert
     it to miliseconds integer */
  seconds = luaL_checknumber(L, 2);
  milis = (int)(seconds * 1000.0);
  r = nn_poll( fds, n, milis ); /* LIBRARY CALL */
  nret = check_error(L, r);
  if (nret) { /* error case */
    free(fds);
    return nret;
  }
  if (r == 0) { /* timeout case */
    free(fds);
    lua_pushnumber(L, 0);
    lua_pushstring(L, "timeout");
    return 2;
  }
  /* general case: write 'revents' field in the table at stack's position 1 */
  for (i=1; i<=n; ++i) {
    lua_rawgeti(L, 1, i);
    lua_pushnumber(L, fds[i-1].revents);
    lua_setfield(L, -2, "revents");
    lua_pop(L, 1);
  }
  /* return number of fd's ready to be read of written */
  lua_pushnumber(L, r);
  free(fds);
  return 1;
}

/**
 * msg,error_msg = xemsg.recv(s, [n,] flags)
 */
int xemsg_recv(lua_State *L) {
  /* two cases depending in the number of values at the stack */
  int n = lua_gettop(L);
  if (n == 2) {
    /* first case: two values, the socket and the flags; call nn_recv to
       allocate a buffer with enough space for reading the whole message */
    void *buf;
    int r = nn_recv( luaL_checkinteger(L, 1), &buf, NN_MSG,
                     luaL_checkinteger(L, 2) );
    int nret = check_error(L, r);
    if (nret) return nret; /* error case */
    lua_pushlstring(L, buf, r);
    nn_freemsg(buf);
    return 1;
  } else if (n == 3) {
    /* second case: three values, the socket, the expected message size and the
       flags; call nn_recv with a Lua buffer allocated to store the expected
       length */
    int s     = luaL_checkinteger(L, 1);
    size_t sz = luaL_checkinteger(L, 2);
    int flags = luaL_checkinteger(L, 3);
    luaL_Buffer buf;
    char *buf_str = luaL_buffinitsize(L, &buf, sz);
    int r = nn_recv( s, buf_str, sz, flags );
    if (r < 0) {
      luaL_pushresultsize(&buf, sz); lua_pop(L, 1);
      lua_pushnil(L);
      lua_pushstring(L, nn_strerror(nn_errno()));
      return 2;
    }
    luaL_pushresultsize(&buf, sz);
    return 1;
  } else {
    return luaL_error(L, "Incorrect number of arguments, expected 2 or 3");
  }
}

/**
 * size,error_msg = xemsg.send(s, buf, flags)
 */
int xemsg_send(lua_State *L) {
  const void *buf;
  size_t len;
  int type = lua_type(L, 2);
  /* depending in the type of the data, buf will point to a userdata message or
     to a string */
  switch(type) {
  case LUA_TUSERDATA:
    buf = luaL_checkudata(L, 2, "nn_buffer");
    break;
  case LUA_TSTRING:
    buf =luaL_checkstring(L, 2);
    break;
  default:
    lua_pushnil(L);
    lua_pushstring(L, "Expected string or nn_buffer at 2nd argument");
    return 2;
  }
  len = lua_rawlen(L, 2);
  return check_int(L, nn_send( luaL_checkinteger(L, 1),
                               buf,
                               len,
                               luaL_checkinteger(L, 3) ));
}

/**
 * ok,error_msg = xemsg.setsockopt(s, level, option, optval)
 */
int xemsg_setsockopt(lua_State *L) {
  const void *optval;
  /* this union allow to point optval to an integer or a string value */
  union { int vint; const char *vstr; } optval_u;
  size_t optvallen;
  int type = lua_type(L, 4);
  /* depending in the type of the data, optval will point to a integer or to a
     string */
  switch(type) {
  case LUA_TNUMBER:
    optval_u.vint = luaL_checkinteger(L, 4);
    optval = &optval_u.vint;
    optvallen = sizeof(int);
    break;
  case LUA_TSTRING:
    optval_u.vstr = luaL_checkstring(L, 4);
    optval = optval_u.vstr;
    optvallen = sizeof(char) * luaL_len(L, 4);
    break;
  default:
    lua_pushnil(L);
    lua_pushstring(L, "Expected number or string at 4th argument");
    return 2;
  }
  return check_succ(L, nn_setsockopt( luaL_checkinteger(L, 1),
                                      luaL_checkinteger(L, 2),
                                      luaL_checkinteger(L, 3),
                                      optval,
                                      optvallen ));
}

/**
 * ok,error_msg = xemsg.shutdown(s, how)
 */
int xemsg_shutdown(lua_State *L) {
  return check_succ(L, nn_shutdown( luaL_checkinteger(L, 1),
                                    luaL_checkinteger(L, 2) ));
}

/**
 * id,error_msg = xemsg.socket(domain, protocol)
 */
int xemsg_socket(lua_State *L) {
  return check_int(L, nn_socket( luaL_checkinteger(L, 1),
                                 luaL_checkinteger(L, 2) ));
}

/**
 * xemsg.term()
 */
int xemsg_term(lua_State *L) {
  (void) L;
  nn_term();
  return 0;
}

/***************************************************************************/

int luaopen_xemsg(lua_State *L) {
  int i, symbol_value;
  const char *symbol_name;
  static const luaL_Reg funcs[] = {
    {"bind", xemsg_bind},
    {"close", xemsg_close},
    {"connect", xemsg_connect},
    {"device", xemsg_device},
    {"getsockopt", xemsg_getsockopt},
    {"poll", xemsg_poll},
    {"recv", xemsg_recv},
    {"send", xemsg_send},
    {"setsockopt", xemsg_setsockopt},
    {"shutdown", xemsg_shutdown},
    {"socket", xemsg_socket},
    {"term", xemsg_term},
    {NULL, NULL}
  };
  
  luaL_newlib(L, funcs);

  /* export all available symbols */
  for (i=0; (symbol_name = nn_symbol(i, &symbol_value)); ++i) {
    lua_pushinteger(L, symbol_value);
    lua_setfield(L, -2, symbol_name);
  }
  /* add NN_POLLINOUT to avoid using and/or operator in Lua */
  lua_pushinteger(L, NN_POLLIN & NN_POLLOUT); lua_setfield(L, -2, "NN_POLLINOUT");

  /* add version and name of this module */
  lua_pushstring(L, XEMSG_NAME);
  lua_setfield(L, -2, XEMSG_NAME_FIELD);
  lua_pushstring(L, XEMSG_VERSION);
  lua_setfield(L, -2, XEMSG_VERSION_FIELD);
  
  return 1;
}

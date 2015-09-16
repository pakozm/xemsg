/*
 * This file is part of Xemsg! a Lua binding for nanomsg library.

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

#define SYMBOL_NAMES_FIELD  "SYMBOL_NAMES"
#define XEMSG_NAME_FIELD    "_NAME"
#define XEMSG_NAME          "Xemsg!"
#define XEMSG_VERSION_FIELD "_VERSION"
#define XEMSG_VERSION       "0.1"

#define SOCKET_METATABLE "xemsg.socket"

#if LUA_VERSION_NUM < 502
#define luaL_len(L,n) (int)lua_objlen((L),(n))

void *luaL_testudata(lua_State *L, int n, const char *tname) {
  if (!lua_isuserdata(L,n) || !lua_getmetatable(L,n)) return NULL;
  luaL_getmetatable(L, tname);
  int eq = lua_rawequal(L,-1,-2);
  lua_pop(L,2); // remove metatables
  if (!eq) return NULL;
  return lua_touserdata(L,n);
}

void luaL_setmetatable(lua_State *L, const char *tname) {
  luaL_getmetatable(L,tname);
  lua_setmetatable(L,-2);
}
#endif

/**
 * Pushes a POSIX error into the stack and returns the number of pushed values.
 */
int xelua_pusherror(lua_State *L, const char *msg, int err) {
  lua_pushnil(L);
  lua_pushstring(L, msg);
  lua_pushnumber(L, err);
  return 3;
}

/**
 * Returns if the value is a socket or no.
 */
int xelua_issocket(lua_State *L, int n, int *s) {
  int *aux = luaL_testudata(L, n, SOCKET_METATABLE);
  if (aux == NULL) return 0;
  if (s != NULL) *s = *aux;
  return 1;
}

/**
 * Returns the socket pointer.
 */
int *xelua_checksocket(lua_State *L, int n) {
  int *s = (int*)luaL_testudata(L, n, SOCKET_METATABLE);
  if (s == NULL) {
    luaL_error(L, "Expected a socket as argument");
    return NULL;
  }
  return s;
}

/**
 * Returns the socket integer value or the given def in case of testudata
 * fails.
 */
int xelua_optsocket(lua_State *L, int n, int def) {
  int *s = (int*)luaL_testudata(L, n, SOCKET_METATABLE);
  if (s == NULL) return def;
  return *s;
}

/**
 * Allocates a new socket and pushes it into the Lua stack. Returns the number
 * of pushed values.
 */
int xelua_newsocket(lua_State *L, int value) {
  int *s = (int*)lua_newuserdata(L, sizeof(int));
  *s = value;
  luaL_setmetatable(L, SOCKET_METATABLE);
  return 1;
}

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

/**
 * Checks the value of 'r', in case of 'r<0', it pushes into stack 'L'
 * a nil value, an error message string and the error number.
 */
int xelua_error(lua_State *L, int r) {
  if (r < 0) {
    int err = nn_errno();
    return xelua_pusherror(L, nn_strerror(err), err);
  }
  return 0;
}

/**
 * Checks a returned int and pushes it into Lua in case of success, otherwise
 * pushes a nil value followed by the error string and error number.
 */
int xelua_errorint(lua_State *L, int r) {
  int nret = xelua_error(L, r);
  if (nret) return nret;
  lua_pushinteger(L, r);
  return 1;
}

/**
 * Checks a returned int and pushes true into Lua in case of success, otherwise
 * pushes a nil value followed by the error string and error number.
 */
int xelua_errorsucc(lua_State *L, int r) {
  int nret = xelua_error(L, r);
  if (nret) return nret;
  lua_pushboolean(L, 1);
  return 1; 
}

/***************************************************************************/

/**
 * Socket __gc metamethod, terminates the socket connection.
 */
int xemsg_socket_gc(lua_State *L) {
  int s = *xelua_checksocket(L, 1);
  if (s >= 0) nn_close(s);
  return 0;
}

/**
 * Socket __tostring metamethod returns a string with the socket number.
 */
int xemsg_socket_tostring(lua_State *L) {
  int s = *xelua_checksocket(L, 1);
  lua_pushfstring(L, "SP-socket: %d", s);
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
 * id,e_msg,e_num = xe.bind(s, addr) 
 */
int xemsg_bind(lua_State *L) {
  return xelua_errorint(L, nn_bind( *xelua_checksocket(L, 1),
                                    luaL_checkstring(L,2) ));
}

/**
 * ok,e_msg,e_num = xe.close(s)
 */
int xemsg_close(lua_State *L) {
  int *s = xelua_checksocket(L, 1);
  int r = xelua_errorsucc(L, nn_close(*s) );
  *s = -1; // invalid socket number
  return r;
}

/**
 * id,e_msg,e_num = xe.connect(s, addr)
 */
int xemsg_connect(lua_State *L) {
  return xelua_errorint(L, nn_connect( *xelua_checksocket(L, 1),
                                       luaL_checkstring(L, 2) ));
}

/**
 * nil,e_msg,e_num = xe.device(s1[, s2])
 */
int xemsg_device(lua_State *L) {
  return xelua_errorsucc(L, nn_device( *xelua_checksocket(L, 1),
                                       xelua_optsocket(L, 2, -1) ));
}

/**
 * optval,e_msg,e_num = xe.getsockopt(s, level, option)
 */
int xemsg_getsockopt(lua_State *L) {
  void *optval = NULL;
  size_t optvallen;
  int option = luaL_checkinteger(L, 3);
  int r = nn_getsockopt( *xelua_checksocket(L, 1),
                         luaL_checkinteger(L, 2),
                         option,
                         optval,
                         &optvallen );
  int nret = xelua_error(L, r);
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
 * n,e_msg,e_num = xemsg_poll({ {fd=fd, events=events}, {fd=fd, events=events}, ...} [, timeout])
 */
int xemsg_poll(lua_State *L) {
  if (xelua_issocket(L,1,NULL)) lua_remove(L,1); // to allow s:poll(...) call
  struct nn_pollfd *fds;
  int n = luaL_len(L,1), i, milis, r, nret;
  /* alloc memory for pollfd array, initializing it with the data of Lua table
     at position 1 in the stack */
  fds = (struct nn_pollfd*)malloc(sizeof(struct nn_pollfd)*n);
  for (i=1; i<=n; ++i) {
    lua_rawgeti(L, 1, i);
    lua_getfield(L, -1, "fd");
    lua_getfield(L, -2, "events");
    if (!xelua_isshort(L, -1, &fds[i-1].events)) {
      free(fds);
      return luaL_error(L, "Expected an integer at events field");
    }
    if (!xelua_issocket(L, -2, &fds[i-1].fd)) {
      free(fds);
      return luaL_error(L, "Expected a socket at fd field");
    }
    lua_pop(L, 3);
  }
  milis = luaL_optinteger(L, 2, 0);
  r = nn_poll( fds, n, milis ); /* LIBRARY CALL */
  nret = xelua_error(L, r);
  if (nret) { /* error case */
    free(fds);
    return nret;
  }
  if (r == 0) { /* timeout case */
    free(fds);
    lua_pushnumber(L, 0);
    return 1;
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
 * msg,e_msg,e_num = xe.recv(s [, n] [, flags])
 */
int xemsg_recv(lua_State *L) {
  /* three cases depending in the number of values at the stack */
  int n = lua_gettop(L);
  if (n >= 1 && n <= 2) {
    /* first and second case: call nn_recv to allocate a buffer with enough
       space for reading the whole message */
    int flags = 0;
    /* optional flags */
    if (n == 2) flags = luaL_checkinteger(L, 2);
    void *buf;
    int r = nn_recv( *xelua_checksocket(L, 1), &buf, NN_MSG, flags );
    int nret = xelua_error(L, r);
    if (nret) return nret; /* error case */
    lua_pushlstring(L, buf, r);
    nn_freemsg(buf);
    return 1;
  }
  else if (n == 3) {
    /* third case: three values, the socket, the expected message size and the
       flags; call nn_recv with a Lua buffer allocated to store the expected
       length */
    int s     = *xelua_checksocket(L, 1);
    int flags = luaL_checkinteger(L, 3);
#if LUA_VERSION_NUM < 502
    /* TODO: implement using luaL_Buffer in Lua 5.1 */
    void *buf;
    int r = nn_recv( s, &buf, NN_MSG, flags );
    int nret = xelua_error(L, r);
    if (nret) return nret; /* error case */
    lua_pushlstring(L, buf, r);
    nn_freemsg(buf);
    return 1;
#else
    size_t sz = luaL_checkinteger(L, 2);
    luaL_Buffer buf;
    char *buf_str = luaL_buffinitsize(L, &buf, sz);
    int r = nn_recv( s, buf_str, sz, flags );
    if (r < 0) {
      int err = nn_errno();
      luaL_pushresultsize(&buf, sz); lua_pop(L, 1);
      return xelua_pusherror(L, nn_strerror(err), err);
    }
    luaL_pushresultsize(&buf, sz);
    return 1;
#endif
  } else {
    return
      xelua_pusherror(L, "Incorrect number of arguments, expected two or three",
                      EINVAL);
  }
}

/**
 * size,e_msg,e_num = xe.send(s, buf [, flags])
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
    return
      xelua_pusherror(L, "Expected string or nn_buffer at 2nd argument",
                      EINVAL);
  }
  len = luaL_len(L, 2);
  return xelua_errorint(L, nn_send( *xelua_checksocket(L, 1),
                                    buf,
                                    len,
                                    luaL_optinteger(L, 3, 0) ));
}

/**
 * ok,e_msg,e_num = xe.setsockopt(s, level, option, optval)
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
    return
      xelua_pusherror(L, "Expected number or string at 4th argument",
                      EINVAL);
  }
  return xelua_errorsucc(L, nn_setsockopt( *xelua_checksocket(L, 1),
                                           luaL_checkinteger(L, 2),
                                           luaL_checkinteger(L, 3),
                                           optval,
                                           optvallen ));
}

/**
 * ok,e_msg,e_num = xe.shutdown(s, how)
 */
int xemsg_shutdown(lua_State *L) {
  return xelua_errorsucc(L, nn_shutdown( *xelua_checksocket(L, 1),
                                         luaL_checkinteger(L, 2) ));
}

/**
 * id,e_msg,e_num = xe.socket([domain,] protocol)
 */
int xemsg_socket(lua_State *L) {
  int n = lua_gettop(L), domain=AF_SP;
  if (n == 2) domain = luaL_checkinteger(L, 1);
  if (n < 1 || n > 2) {
    return
      xelua_pusherror(L, "Incorrect number of arguments, expected 1 or 2",
                      EINVAL);
  }
  int s = nn_socket( domain, luaL_checkinteger(L, n) );
  if (s < 0) return xelua_error(L, s);
  return xelua_newsocket(L, s);
}

/**
 * xe.term()
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
  
#if LUA_VERSION_NUM < 502
  lua_newtable(L);
  luaL_register(L, NULL, funcs);
#else
  luaL_newlib(L, funcs);
#endif
  
  lua_newtable(L);
  /* export all available symbols */
  for (i=0; (symbol_name = nn_symbol(i, &symbol_value)); ++i) {
    lua_pushinteger(L, symbol_value);
    lua_setfield(L, -3, symbol_name);
    lua_pushstring(L, symbol_name);
    lua_rawseti(L, -2, symbol_value);
  }
  
  lua_setfield(L, -2, SYMBOL_NAMES_FIELD);
  
  /* add version and name of this module */
  lua_pushstring(L, XEMSG_NAME);
  lua_setfield(L, -2, XEMSG_NAME_FIELD);
  lua_pushstring(L, XEMSG_VERSION);
  lua_setfield(L, -2, XEMSG_VERSION_FIELD);

  /* set socket metamethods */
  if (luaL_newmetatable(L, SOCKET_METATABLE)) {
    lua_pushvalue(L, -2);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, xemsg_socket_gc);
    lua_setfield(L, -2, "__gc");
    lua_pushcfunction(L, xemsg_socket_tostring);
    lua_setfield(L, -2, "__tostring");
  }
  lua_pop(L, 1);
  
  return 1;
}

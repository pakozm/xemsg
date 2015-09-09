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

int xelua_isinteger(lua_State *L, int n, int *r) {
  if (lua_isnumber(L,n)) {
    double aux = lua_tonumber(L, n);
    *r = aux;
    if ((double)*r == aux) return 1;
  }
  return 0;
}

int xelua_isshort(lua_State *L, int n, short *r) {
  if (lua_isnumber(L,n)) {
    double aux = lua_tonumber(L, n);
    *r = aux;
    if ((double)*r == aux) return 1;
  }
  return 0;
}

/* checks a returned int and pushes it into Lua in case of success, otherwise
   pushes a nil value followed by the error string */
int check_int(lua_State *L, int r) {
  if (r < 0) {
    int err = nn_errno();
    lua_pushnil(L);
    lua_pushstring(L, nn_strerror(err));
    return 2;
  } else {
    lua_pushinteger(L, r);
    return 1;
  }
}

/* checks a returned int and pushes true into Lua in case of success, otherwise
   pushes a nil value followed by the error string */
int check_succ(lua_State *L, int r) {
  if (r < 0) {
    int err = nn_errno();
    lua_pushnil(L);
    lua_pushstring(L, nn_strerror(err));
    return 2;
  } else {
    lua_pushboolean(L, 1);
    return 1;
  }
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

/* id,error_msg = xemsg.bind(s, addr) */
int xemsg_bind(lua_State *L) {
  return check_int(L, nn_bind( luaL_checkinteger(L, 1),
                               luaL_checkstring(L,2) ));
}

/* ok,error_msg = xemsg.close(s) */
int xemsg_close(lua_State *L) {
  return check_succ(L, nn_close( luaL_checkinteger(L, 1) ));
}

/* NOT IMPLEMENTED nn_cmsg */

/* id,error_msg = xemsg.connect(s, addr) */
int xemsg_connect(lua_State *L) {
  return check_int(L, nn_connect( luaL_checkinteger(L, 1),
                                  luaL_checkstring(L, 2) ));
}

/* optval,error_msg = xemsg.getsockopt(s, level, option) */
int xemsg_getsockopt(lua_State *L) {
  void *optval = NULL;
  size_t optvallen;
  int option = luaL_checkinteger(L, 3);
  int r = nn_getsockopt( luaL_checkinteger(L, 1),
                         luaL_checkinteger(L, 2),
                         option,
                         optval,
                         &optvallen );
  if (r < 0) {
    lua_pushnil(L);
    lua_pushstring(L, nn_strerror(nn_errno()));
    return 2;
  }
  switch(option) {
  case NN_SOCKET_NAME: lua_pushlstring(L, optval, optvallen); break;
  default: lua_pushinteger(L, *((int*)optval)); break;
  }
  return 1;
}

/* n,error_msg = xemsg_poll({ {fd=fd, events=events}, {fd=fd, events=events}, ...}, timeout) */
int xemsg_poll(lua_State *L) {
  double seconds;
  struct nn_pollfd *fds;
  int n = luaL_len(L,1), i, milis, r;
  fds = (struct nn_pollfd*)malloc(sizeof(struct nn_pollfd)*n);
  for (i=1; i<=n; ++i) {
    lua_rawgeti(L, 1, i);
    lua_getfield(L, -1, "fd"); /* fd */
    lua_getfield(L, -2, "events"); /* events */
    if (!xelua_isshort(L, -1, &fds[i-1].events) ||
        !xelua_isinteger(L, -2, &fds[i-1].fd)) {
      free(fds);
      return luaL_error(L, "Expected integer values");
    }
    lua_pop(L, 3);
  }
  seconds = luaL_checknumber(L, 2);
  milis = (int)(seconds * 1000.0);
  r = nn_poll( fds, n, milis ); /* LIBRARY CALL */
  if (r < 0) {
    free(fds);
    lua_pushnil(L);
    lua_pushstring(L, nn_strerror(nn_errno()));
    return 2;
  }
  if (r == 0) {
    free(fds);
    lua_pushnumber(L, 0);
    lua_pushstring(L, "timeout");
    return 2;
  }
  for (i=1; i<=n; ++i) {
    lua_rawgeti(L, 1, i);
    lua_pushnumber(L, fds[i-1].revents);
    lua_setfield(L, -2, "revents");
    lua_pop(L, 1);
  }
  lua_pushnumber(L, r);
  free(fds);
  return 1;
}

/* msg,error_msg = xemsg.recv(s, [n,] flags) */
int xemsg_recv(lua_State *L) {
  int n = lua_gettop(L);
  if (n == 2) {
    void *buf;
    int r = nn_recv( luaL_checkinteger(L, 1), &buf, NN_MSG,
                     luaL_checkinteger(L, 2) );
    if (r < 0) {
      lua_pushnil(L);
      lua_pushstring(L, nn_strerror(nn_errno()));
      return 2;
    }
    lua_pushlstring(L, buf, r);
    nn_freemsg(buf);
    return 1;
  } else if (n == 3) {
    int s     = luaL_checkinteger(L, 1);
    size_t sz = luaL_checkinteger(L, 2);
    int flags = luaL_checkinteger(L, 3);
    luaL_Buffer buf;
    char *buf_str = luaL_buffinitsize(L, &buf, sz);
    int r = nn_recv( s, buf_str, sz, flags );
    if (r < 0) {
      lua_pushnil(L);
      lua_pushstring(L, nn_strerror(nn_errno()));
      luaL_pushresultsize(&buf, sz); lua_pop(L, 1);
      return 2;
    }
    luaL_pushresultsize(&buf, sz);
    return 1;
  } else {
    return luaL_error(L, "Incorrect number of arguments, expected 2 or 3");
  }
}

/* size,error_msg = xemsg.send(s, buf, flags) */
int xemsg_send(lua_State *L) {
  const void *buf;
  size_t len;
  int type = lua_type(L, 2);
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

/* ok,error_msg = xemsg.setsockopt(s, level, option, optval) */
int xemsg_setsockopt(lua_State *L) {
  union { int vint; const char *vstr; } optval_u;
  const void *optval;
  size_t optvallen;
  int type = lua_type(L, 4);
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

/* ok,error_msg = xemsg.shutdown(s, how) */
int xemsg_shutdown(lua_State *L) {
  return check_succ(L, nn_shutdown( luaL_checkinteger(L, 1),
                                    luaL_checkinteger(L, 2) ));
}

/* id,error_msg = xemsg.socket(domain, protocol) */
int xemsg_socket(lua_State *L) {
  return check_int(L, nn_socket( luaL_checkinteger(L, 1),
                                 luaL_checkinteger(L, 2) ));
}

/***************************************************************************/

int luaopen_xemsg(lua_State *L) {
  static const luaL_Reg nn_pollfd_meta[] = {
    {NULL, NULL}
  };
  static const luaL_Reg funcs[] = {
    {"bind", xemsg_bind},
    {"close", xemsg_close},
    {"connect", xemsg_connect},
    {"getsockopt", xemsg_getsockopt},
    {"poll", xemsg_poll},
    {"recv", xemsg_recv},
    {"send", xemsg_send},
    {"setsockopt", xemsg_setsockopt},
    {"shutdown", xemsg_shutdown},
    {"socket", xemsg_socket},
    {NULL, NULL}
  };
  
  luaL_newmetatable(L, "nn_pollfd");
  luaL_setfuncs(L, nn_pollfd_meta, 0);
  lua_pop(L, 1);
  
  luaL_newlib(L, funcs);
  
  /* socket domains */
  lua_pushinteger(L, AF_SP); lua_setfield(L, -2, "AF_SP");
  lua_pushinteger(L, AF_SP_RAW); lua_setfield(L, -2, "AF_SP_RAW");

  /* socket protocols */
  lua_pushinteger(L, NN_BUS); lua_setfield(L, -2, "NN_BUS");
  lua_pushinteger(L, NN_PAIR); lua_setfield(L, -2, "NN_PAIR");
  lua_pushinteger(L, NN_PUB); lua_setfield(L, -2, "NN_PUB");
  lua_pushinteger(L, NN_PULL); lua_setfield(L, -2, "NN_PULL");
  lua_pushinteger(L, NN_PUSH); lua_setfield(L, -2, "NN_PUSH");
  lua_pushinteger(L, NN_REQ); lua_setfield(L, -2, "NN_REQ");
  lua_pushinteger(L, NN_REP); lua_setfield(L, -2, "NN_REP");
  lua_pushinteger(L, NN_RESPONDENT); lua_setfield(L, -2, "NN_RESPONDENT");
  lua_pushinteger(L, NN_SUB); lua_setfield(L, -2, "NN_SUB");
  lua_pushinteger(L, NN_SURVEYOR); lua_setfield(L, -2, "NN_SURVEYOR");
  
  /* socket options */
  lua_pushinteger(L, NN_LINGER); lua_setfield(L, -2, "NN_LINGER");
  lua_pushinteger(L, NN_SNDBUF); lua_setfield(L, -2, "NN_SNDBUF");
  lua_pushinteger(L, NN_RCVBUF); lua_setfield(L, -2, "NN_RCVBUF");
  lua_pushinteger(L, NN_SNDTIMEO); lua_setfield(L, -2, "NN_SNDTIMEO");
  lua_pushinteger(L, NN_RCVTIMEO); lua_setfield(L, -2, "NN_RCVTIMEO");
  lua_pushinteger(L, NN_RECONNECT_IVL); lua_setfield(L, -2, "NN_RECONNECT_IVL");
  lua_pushinteger(L, NN_RECONNECT_IVL_MAX); lua_setfield(L, -2, "NN_RECONNECT_IVL_MAX");
  lua_pushinteger(L, NN_SNDPRIO); lua_setfield(L, -2, "NN_SNDPRIO");
  lua_pushinteger(L, NN_RCVPRIO); lua_setfield(L, -2, "NN_RCVPRIO");
  lua_pushinteger(L, NN_IPV4ONLY); lua_setfield(L, -2, "NN_IPV4ONLY");
  lua_pushinteger(L, NN_SOCKET_NAME); lua_setfield(L, -2, "NN_SOCKET_NAME");
  
  /* send flags */
  lua_pushinteger(L, NN_DONTWAIT); lua_setfield(L, -2, "NN_DONTWAIT");
  
  /* poll events */
  lua_pushinteger(L, NN_POLLIN); lua_setfield(L, -2, "NN_POLLIN");
  lua_pushinteger(L, NN_POLLOUT); lua_setfield(L, -2, "NN_POLLOUT");
  lua_pushinteger(L, NN_POLLIN & NN_POLLOUT); lua_setfield(L, -2, "NN_POLLINOUT");
  
  return 1;
}

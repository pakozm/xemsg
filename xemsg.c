#define _GNU_SOURCE /* to activate pthread_tryjoin_np in pthread.h */

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <nanomsg/nn.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

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

typedef struct{
  int s;
  char *addr;
} bind_data_t ;

typedef struct{
  int client, err;
} bind_result_t;

void *bind_routine(void *data) {
  bind_result_t *result;
  bind_data_t *bind_data = (bind_data_t*)data;
  int r = nn_bind( bind_data->s, bind_data->addr );
  free(bind_data->addr);
  free(bind_data);
  result = (bind_result_t*)malloc(sizeof(bind_result_t));
  result->client = r;
  result->err = (r<0) ? nn_errno() : 0;
  return (void*)result;
}

int push_bind_result(lua_State *L, bind_result_t *result) {
  if (result->client < 0) {
    lua_pushnil(L);
    lua_pushstring(L, nn_strerror(result->err));
    free(result);
    return 2;
  }
  lua_pushinteger(L, result->client);
  free(result);
  return 1;
}

int xmesg_run_bind_thread(lua_State *L) {
  int r;
  pthread_attr_t attr;
  pthread_t *th = (pthread_t*)lua_newuserdata(L, sizeof(pthread_t));
  bind_data_t *bind_data = malloc(sizeof(bind_data_t));
  bind_data->s = luaL_checkinteger(L, 1);
  bind_data->addr = strdup(luaL_checkstring(L,2));
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  r = pthread_create(th, NULL, bind_routine, bind_data);
  pthread_attr_destroy(&attr);
  if (r < 0) {
    lua_pop(L, 1);
    free(bind_data->addr);
    free(bind_data);
    lua_pushnil(L);
    lua_pushstring(L, strerror(r));
    return 2;
  }
  luaL_setmetatable(L, "pthread_t");
  return 1;
}

int xmesg_join_bind_thread(lua_State *L) {
  pthread_t *th = (pthread_t*)luaL_checkudata(L, 1, "pthread_t");
  bind_result_t *result;
  int r = pthread_join(*th, (void**)&result);
  if (r < 0) {
    lua_pushnil(L);
    lua_pushstring(L, strerror(r));
    return 2;
  }
  return push_bind_result(L, result);
}

int xmesg_tryjoin_bind_thread(lua_State *L) {
  pthread_t *th = (pthread_t*)luaL_checkudata(L, 1, "pthread_t");
  bind_result_t *result;
  int r = pthread_tryjoin_np(*th, (void**)&result);
  if (r == EBUSY) {
    lua_pushboolean(L, 0);
    return 1;
  }
  lua_pushboolean(L, 1);
  if (r < 0) {
    lua_pushnil(L);
    lua_pushstring(L, strerror(r));
    return 3;
  }
  return 1 + push_bind_result(L, result);
}

/***************************************************************************/

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

/***************************************************************************/

int luaopen_xemsg(lua_State *L) {
  static const luaL_Reg pthread_meta[] = {
    {NULL, NULL}
  };
  
  static const luaL_Reg funcs[] = {
    {"run_bind_thread", xmesg_run_bind_thread},
    {"join_bind_thread", xmesg_join_bind_thread},
    {"tryjoin_bind_thread", xmesg_tryjoin_bind_thread},
    /**/
    {"bind", xemsg_bind},
    {"close", xemsg_close},
    {"connect", xemsg_connect},
    {NULL, NULL}
  };
  
  luaL_newmetatable(L, "pthread_t");
  luaL_setfuncs(L, pthread_meta, 0);
  lua_pop(L, 1);
  
  luaL_newlib(L, funcs);
  return 1;
}

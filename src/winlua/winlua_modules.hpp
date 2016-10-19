#ifndef WINLUA_MODULES_HPP_INCLUDED
#define WINLUA_MODULES_HPP_INCLUDED


int luaopen_winos(lua_State *L);
int luaopen_fs(lua_State *L);
int luaopen_shell(lua_State *L);
int luaopen_registry(lua_State *L);

int luaopen_dispatch_typeinfo(lua_State *L);
int luaopen_dispatch_thin(lua_State *L);

int winlua_print(lua_State *L);


#endif
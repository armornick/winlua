#ifndef WINLUA_MODULES_HPP_INCLUDED
#define WINLUA_MODULES_HPP_INCLUDED


int luaopen_winos(lua_State *L);
int luaopen_fs(lua_State *L);

int winlua_print(lua_State *L);


#endif
#include <lua.hpp>

#include <io.h>
#define access _access
#ifndef F_OK
#define F_OK 00
#endif

/* default main file to load */
#define DEFAULT_MAIN_FILE "main.lua"

/* Lua main function */
static int lua_main(lua_State *L);

int main(int argc, char const *argv[])
{
	/* allocate Lua state */
	lua_State *L = luaL_newstate();
	if (L == NULL)
	{
		printf("%s: could not allocate Lua state\n", argv[0]);
	}
	/* prepare Lua environment */
	luaL_openlibs(L);
	/* call the Lua main function */
	lua_pushcfunction(L, lua_main);
	lua_pushinteger(L, argc);
	lua_pushlightuserdata(L, static_cast<void*>(argv));
	if (lua_pcall(L, 2, 0, 0) != LUA_OK)
	{
		printf("%s: %s\n", argv[0], lua_tostring(L, -1));
		lua_pop(L, 1);
	}
	/* clean up resources and exit */
	lua_close(L);
	return 0;
}

static int lua_main(lua_State *L)
{
	const char *mainfile = DEFAULT_MAIN_FILE;
	/* fetch command-line arguments */
	int argc = static_cast<int>(lua_tointeger(L, 1));
	char **argv = static_cast<char **>(lua_touserdata(L, 2));
	if (argv == NULL)
	{
		return luaL_error(L, "could not load command-line arguments");
	}
	/* create arg table */
	lua_newtable(L);
	for (int i = 0; i < argc; i++)
	{
		lua_pushstring(L, argv[i]);
		lua_rawseti(L, -2, i+1);
	}
	lua_setglobal(L, "arg");
	/* check and run main file */
	if (argc > 1 && access(argv[1], F_OK) == 0)
	{
		mainfile = argv[1];
	}
	if (luaL_dofile(L, mainfile) != LUA_OK)
	{
		/* propagate existing error */
		return lua_error(L);
	}
	return 0;
}

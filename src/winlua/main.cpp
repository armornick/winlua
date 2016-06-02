#include "winlua.hpp"
#include "winlua_modules.hpp"

#include <io.h>
#define access _access
#ifndef F_OK
#define F_OK 00
#endif

/* default main file to load */
#define DEFAULT_MAIN_FILE "main.lua"

/* Lua main function */
static int lua_main(lua_State *L);

/* setup the WinLua environment */
static void winlua_setup_env(lua_State *L);

// ----------------------------------------------------------------------------------------------------

int main(int argc, char const *argv[])
{
	/* allocate Lua state */
	lua_State *L = luaL_newstate();
	if (L == NULL)
	{
		printf("%s: could not allocate Lua state\n", argv[0]);
	}
	
	/* prepare Lua environment */
	// luaL_openlibs(L);
	winlua_setup_env(L);

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

// ----------------------------------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------------------------------

static const luaL_Reg loadedlibs[] = {
  {"_G", luaopen_base},
  {LUA_LOADLIBNAME, luaopen_package},
  {LUA_COLIBNAME, luaopen_coroutine},
  {LUA_TABLIBNAME, luaopen_table},
  {LUA_IOLIBNAME, luaopen_io},
  {LUA_OSLIBNAME, luaopen_winos},
  {LUA_STRLIBNAME, luaopen_string},
  {LUA_MATHLIBNAME, luaopen_math},
  {LUA_UTF8LIBNAME, luaopen_utf8},
  {NULL, NULL}
};

static const luaL_Reg preloadlibs[] = {
	{LUA_DBLIBNAME, luaopen_debug},
	{ "fs", luaopen_fs },
	{NULL, NULL}
};

static void winlua_setup_env(lua_State *L)
{
	const luaL_Reg *lib;
	
	/* "require" functions from 'loadedlibs' and set results to global table */
	for (lib = loadedlibs; lib->func; lib++) {
		luaL_requiref(L, lib->name, lib->func, 1);
		lua_pop(L, 1);  /* remove lib */
	}

	/* add functions from 'preloadlibs' to package.preload table */
	luaL_getsubtable(L, LUA_REGISTRYINDEX, "_PRELOAD");
	for (lib = preloadlibs; lib->func; lib++) {
		lua_pushcfunction(L, lib->func);
		lua_setfield(L, -2, lib->name);
	}
	lua_pop(L, 1);  // remove _PRELOAD table

	/* replace print with Unicode-enabled version */
	lua_pushcfunction(L, winlua_print);
	lua_setglobal(L, "print");
}

// ----------------------------------------------------------------------------------------------------

/*
Unicode-enabled version of the Lua print function
*/
int winlua_print (lua_State *L) {
	HANDLE StdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	int n = lua_gettop(L);  /* number of arguments */
	lua_getglobal(L, "tostring");

	for (int i = 1; i <= n; i++) {
		
		lua_pushvalue(L, -1);  /* function to be called */
		lua_pushvalue(L, i);   /* value to print */
		lua_call(L, 1, 1);
		
		const char *output = lua_tostring(L, -1);  /* get result */
		if (output == NULL)
			return luaL_error(L, "'tostring' must return a string to 'print'");

		if (i > 1) WriteConsoleW(StdOut, L"\t", 1, NULL, NULL);

		wchar_t *outputW = utf8_to_wstring(L, output);
		WriteConsoleW(StdOut, outputW, lstrlenW(outputW), NULL, NULL);
		lua_pop(L, 2);  /* pop result and temporary wstring */
	}

	WriteConsoleW(StdOut, L"\n", 1, NULL, NULL);
	return 0;
}
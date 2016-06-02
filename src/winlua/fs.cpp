#include "winlua.hpp"

/* ------------------------------------------------------------
WinLua Filesystem functions
------------------------------------------------------------ */

static int fs_attributes(lua_State *L)
{
	const char *filepath = luaL_checkstring(L, 1);
	wchar_t *filepathW = utf8_to_wstring(L, filepath);

	WIN32_FIND_DATAW fileData;
	HANDLE findHandle = FindFirstFileW(filepathW, &fileData);
	lua_pop(L, 1); // pop temporary wstring

	if (findHandle == INVALID_HANDLE_VALUE)
	{
		DWORD err = GetLastError();

		if (err == ERROR_FILE_NOT_FOUND)
		{
			lua_pushnil(L);
			return 1;
		}
		else
		{
			return luaL_error(L, "could not get attributes for file '%s' (%d)", filepath, err);
		}
	}

	FindClose(findHandle);

	lua_createtable(L, 0, 5);
	winlua_push_date(L, fileData.ftCreationTime);
	lua_setfield(L, -2, "creation");
	winlua_push_date(L, fileData.ftLastAccessTime);
	lua_setfield(L, -2, "access");
	winlua_push_date(L, fileData.ftLastWriteTime);
	lua_setfield(L, -2, "modification");
	lua_pushstring(L, fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? "directory" : "file");
	lua_setfield(L, -2, "mode");
	lua_pushinteger(L, (fileData.nFileSizeHigh * (MAXDWORD+1)) + fileData.nFileSizeLow );
	lua_setfield(L, -2, "size");
	return 1;
}



/* ------------------------------------------------------------
WinLua Filesystem module
------------------------------------------------------------ */

static const luaL_Reg library_methods[] = {
	{"attributes", fs_attributes},
	{NULL, NULL}
};

int luaopen_fs(lua_State *L)
{
	luaL_newlib(L, library_methods);
	return 1;
}
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
			return luaL_error(L, "could not get attributes for path '%s' (%d)", filepath, err);
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

static int fs_chdir(lua_State *L)
{
	const char *filepath = luaL_checkstring(L, 1);
	wchar_t *filepathW = utf8_to_wstring(L, filepath);

	BOOL ret = SetCurrentDirectoryW(filepathW);
	if (ret == 0)
	{
		return luaL_error(L, "could not change current directory to '%s' (%d)", filepath, GetLastError());
	}

	lua_pushboolean(L, 1);
	return 1;
}

static int fs_currentdir(lua_State *L)
{
	DWORD needed = GetCurrentDirectoryW(0, NULL);
	if (needed == 0)
	{
		return luaL_error(L, "could not get buffer size for current directory (%d)", GetLastError());
	}

	wchar_t *buff = static_cast<wchar_t*>(lua_newuserdata(L, sizeof(wchar_t) * needed));
	needed = GetCurrentDirectoryW(needed, buff);
	
	if (needed == 0)
	{
		return luaL_error(L, "could not get current directory (%d)", GetLastError());
	}

	wstring_to_utf8(L, buff);
	lua_remove(L, -2); // pop temporary buffer
	return 1;
}

static int fs_symlink(lua_State *L)
{
	const char *filepath = luaL_checkstring(L, 1);
	wchar_t *filepathW = utf8_to_wstring(L, filepath);
	const char *linkpath = luaL_checkstring(L, 1);
	wchar_t *linkpathW = utf8_to_wstring(L, linkpath);

	DWORD is_dir = GetFileAttributesW(filepathW);

	if (is_dir == INVALID_FILE_ATTRIBUTES)
	{
		return luaL_error(L, "could not determine attributes of '%s' (%d)", filepath, GetLastError());
	}

	DWORD flags = (is_dir & FILE_ATTRIBUTE_DIRECTORY) ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0;

	BOOLEAN ret = CreateSymbolicLinkW(linkpathW, filepathW, flags);
	if (ret == 0)
	{
		return luaL_error(L, "could not create link '%s' to '%s' (%d)", linkpath, filepath, GetLastError());
	}

	lua_pushboolean(L, 1);
	return 1;
}

static int fs_mkdir(lua_State *L)
{
	const char *filepath = luaL_checkstring(L, 1);
	wchar_t *filepathW = utf8_to_wstring(L, filepath);

	BOOL ret = CreateDirectoryW(filepathW, NULL);
	if (ret == 0)
	{
		return luaL_error(L, "could not create directory '%s' (%d)", filepath, GetLastError());
	}

	lua_pushboolean(L, 1);
	return 1;
}

/* ------------------------------------------------------------
File iterator functions & structures
------------------------------------------------------------ */

#define WINLUA_FINDFILES_META "WinLuaFindFilesUdata"

struct WinLuaFindFilesUdata
{
	HANDLE handle;
	WIN32_FIND_DATAW data;
	bool finished;
};

static int findfiles__gc(lua_State *L)
{
	WinLuaFindFilesUdata *udata = static_cast<WinLuaFindFilesUdata*>(luaL_checkudata(L, 1, WINLUA_FINDFILES_META));
	if (udata->handle != INVALID_HANDLE_VALUE)
	{
		FindClose(udata->handle);
	}
	return 0;
}

static void create_findfiles_meta(lua_State *L)
{
	luaL_newmetatable(L, WINLUA_FINDFILES_META);
	lua_pushcfunction(L, findfiles__gc);
	lua_setfield(L, -2, "__gc");
	lua_pop(L, 1);
}

static int findfiles_next(lua_State *L)
{
	WinLuaFindFilesUdata *udata = static_cast<WinLuaFindFilesUdata*>(luaL_checkudata(L, lua_upvalueindex(1), WINLUA_FINDFILES_META));

	/* was the end-of-search flag set during the previous iteration? */
	if (udata->finished) { return 0; }

	/* print the file name found in the previous iteration */
	wstring_to_utf8(L, udata->data.cFileName);

	/* move to the next file, checking for errors */
	BOOL ret = FindNextFileW(udata->handle, &udata->data);
	if (ret == 0)
	{
		DWORD err = GetLastError();
		if (err == ERROR_NO_MORE_FILES)
		{
			udata->finished = true;
		}
		else
		{
			return luaL_error(L, "could not find next file (%d)", err);
		}
	}

	return 1;
}

static int fs_find(lua_State *L)
{
	const char *filespec = luaL_checkstring(L, 1);
	wchar_t *filespecW = utf8_to_wstring(L, filespec);
	WinLuaFindFilesUdata *udata = static_cast<WinLuaFindFilesUdata*>(lua_newuserdata(L, sizeof(WinLuaFindFilesUdata)));
	luaL_setmetatable(L, WINLUA_FINDFILES_META);

	udata->handle = FindFirstFileW(filespecW, &udata->data);
	if (udata->handle == INVALID_HANDLE_VALUE)
	{
		return luaL_error(L, "could not start search for '%s' (%d)", filespec, GetLastError());
	}

	lua_pushcclosure(L, findfiles_next, 1);
	return 1;
}


/* ------------------------------------------------------------
WinLua Filesystem module
------------------------------------------------------------ */

static const luaL_Reg library_methods[] = {
	{"attributes", fs_attributes},
	{"chdir", fs_chdir},
	{"currentdir", fs_currentdir},
	{"symlink", fs_symlink},
	{"mkdir", fs_mkdir},
	{"find", fs_find},
	{NULL, NULL}
};

int luaopen_fs(lua_State *L)
{
	create_findfiles_meta(L);

	luaL_newlib(L, library_methods);
	return 1;
}
#include "winlua.hpp"

#include <windows.h>
#include <Objbase.h>
#include <Shellapi.h>
#include <Shlobj.h>

#define WINLUA_SHELL_ATEXIT "WinluaShellCom"

/* ------------------------------------------------------------
WinLua Shell functions
------------------------------------------------------------ */
static int shell_displayname(lua_State *L)
{
	const char *path = luaL_checkstring(L, 1);
	wchar_t *pathW = utf8_to_wstring(L, path);

	SHFILEINFOW sfi;
	DWORD_PTR ret = SHGetFileInfoW(pathW, 0, &sfi, sizeof(sfi), SHGFI_DISPLAYNAME);
	if (ret == 0)
	{
		return luaL_error(L, "could not get display name of '%s'", path);
	}

	wstring_to_utf8(L, sfi.szDisplayName);
	return 1;
}

static int shell_filetype(lua_State *L)
{
	const char *path = luaL_checkstring(L, 1);
	wchar_t *pathW = utf8_to_wstring(L, path);

	SHFILEINFOW sfi;
	DWORD_PTR ret = SHGetFileInfoW(pathW, 0, &sfi, sizeof(sfi), SHGFI_TYPENAME);
	if (ret == 0)
	{
		return luaL_error(L, "could not get file type of '%s'", path);
	}

	wstring_to_utf8(L, sfi.szTypeName);
	return 1;
}

static int shell_shldisp_filerun(lua_State *L)
{
	IShellDispatch *shell = NULL;
	
	HRESULT ret = CoCreateInstance(CLSID_Shell, NULL, CLSCTX_INPROC_SERVER, IID_IShellDispatch, reinterpret_cast<void**>(&shell));
	if (SUCCEEDED(ret))
	{
		shell->FileRun();

		shell->Release();
	}

	return 0;
}


/* ------------------------------------------------------------
WinLua Shell module
------------------------------------------------------------ */
static const luaL_Reg library_methods[] = {
	{"displayname", shell_displayname},
	{"filetype", shell_filetype},
	{"FileRun", shell_shldisp_filerun},
	{"rundialog", shell_shldisp_filerun},
	{NULL, NULL}
};

static int deinit_shell(lua_State *L)
{
	printf("Shell module: unloading COM\n");
	CoUninitialize();
	return 0;
}

int luaopen_shell(lua_State *L)
{
	HRESULT ret = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(ret))
	{
		return luaL_error(L, "could not intialize COM runtime (%d)", ret);
	}
	winlua_at_exit(L, WINLUA_SHELL_ATEXIT, deinit_shell);

	luaL_newlib(L, library_methods);
	return 1;
}

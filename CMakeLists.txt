cmake_minimum_required (VERSION 3.0)
project (WinLua)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# remove 'lib' prefix for dlls 
if(WIN32 AND CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif()

if (MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# ------------------------------------------------------------------------------------

option (DYNAMIC_LUA "Link with the Lua DLL instead of the static library" OFF)

# ====================================================================================

set (LUA_DIR vendor/lua-5.3.2/src)

set (LUA_SRCS 
	${LUA_DIR}/lapi.c
	${LUA_DIR}/lcode.c
	${LUA_DIR}/lctype.c
	${LUA_DIR}/ldebug.c
	${LUA_DIR}/ldo.c
	${LUA_DIR}/ldump.c
	${LUA_DIR}/lfunc.c
	${LUA_DIR}/lgc.c
	${LUA_DIR}/llex.c
	${LUA_DIR}/lmem.c
	${LUA_DIR}/lobject.c
	${LUA_DIR}/lopcodes.c
	${LUA_DIR}/lparser.c
	${LUA_DIR}/lstate.c
	${LUA_DIR}/lstring.c
	${LUA_DIR}/ltable.c
	${LUA_DIR}/ltm.c
	${LUA_DIR}/lundump.c
	${LUA_DIR}/lvm.c
	${LUA_DIR}/lzio.c
	${LUA_DIR}/lauxlib.c
	${LUA_DIR}/lbaselib.c
	${LUA_DIR}/lbitlib.c
	${LUA_DIR}/lcorolib.c
	${LUA_DIR}/ldblib.c
	${LUA_DIR}/liolib.c
	${LUA_DIR}/lmathlib.c
	${LUA_DIR}/loslib.c
	${LUA_DIR}/lstrlib.c
	${LUA_DIR}/ltablib.c
	${LUA_DIR}/lutf8lib.c
	${LUA_DIR}/loadlib.c
	${LUA_DIR}/linit.c
)

# ------------------------------------------------------------------------------------

if (DYNAMIC_LUA)

	add_library (lua SHARED ${LUA_SRCS})
	target_compile_definitions(lua PUBLIC LUA_BUILD_AS_DLL)

else ()

	add_library (lua STATIC ${LUA_SRCS})

endif(DYNAMIC_LUA)

# ====================================================================================


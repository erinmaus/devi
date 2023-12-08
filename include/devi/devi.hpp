#pragma once

#ifndef DEVI_DEVI_HPP
#define DEVI_DEVI_HPP

extern "C"
{
    #include "lua.h"
    #include "lauxlib.h"
    #include "lualib.h"
    #include "luaconf.h"
}

#ifdef _WIN32
    #define DEVI_EXPORT __declspec(dllexport)
#else
    #define DEVI_EXPORT
#endif

namespace devi
{
    void luax_pushcfunction(lua_State* L, lua_CFunction func);
    void luax_register(lua_State* L, const luaL_Reg* methods);
}

#endif

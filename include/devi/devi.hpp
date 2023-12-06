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

namespace devi
{
    void luax_pushcfunction(lua_State* L, lua_CFunction func);
    void luax_register(lua_State* L, const luaL_Reg* methods);
}

#endif

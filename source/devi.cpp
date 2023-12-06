#include <exception>
#include "devi/devi.hpp"

static int wrap_func(lua_State* L)
{
    auto func = lua_tocfunction(L, lua_upvalueindex(1));

    if (func)
    {
        try
        {
            return func(L);
        }
        catch (const std::exception& error)
        {
            return luaL_error(L, "%s", error.what());
        }
        catch (...)
        {
            return luaL_error(L, "unknown error");
        }
    }

    return 0;
}

void devi::luax_pushcfunction(lua_State* L, lua_CFunction func)
{
    lua_pushcfunction(L, func);
    lua_pushcclosure(L, &wrap_func, 1);
}

void devi::luax_register(lua_State* L, const luaL_Reg* methods)
{
    lua_newtable(L);
    while (methods->func && methods->name)
    {
        luax_pushcfunction(L, methods->func);
        lua_setfield(L, -2, methods->name);

        ++methods;
    }
}

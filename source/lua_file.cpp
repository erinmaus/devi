#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <string>

#include "devi/lua_file.hpp"

devi::LuaFile::LuaFile(lua_State* L, int index) : L(L)
{
    if (lua_type(L, index) != LUA_TTABLE)
    {
        luaL_error(L, "expected table at index %d", index);
    }

    lua_pushvalue(L, index);
    reference = luaL_ref(L, LUA_REGISTRYINDEX);
}

devi::LuaFile::LuaFile(LuaFile&& other) noexcept : L(other.L), reference(other.reference)
{
    other.L = nullptr;
}

devi::LuaFile::~LuaFile()
{
    if (L)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, reference);
    }
}

void devi::LuaFile::handle_error(int result)
{
    if (result)
    {
        switch (result)
        {
        case LUA_ERRRUN:
        case LUA_ERRMEM:
        {
            std::string message(lua_tostring(L, -1));
            lua_pop(L, 2);

            throw std::runtime_error(message);
        }

        default:
            lua_pop(L, 1);
            throw std::runtime_error("unknown error");
        }
    }
}

std::size_t devi::LuaFile::read(std::uint8_t* buffer, std::size_t size)
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, reference);

    lua_getfield(L, -1, "read");
    lua_pushvalue(L, -2);
    lua_pushinteger(L, size);

    handle_error(lua_pcall(L, 2, 1, 0));

    std::size_t result_buffer_size;
    auto result_buffer = lua_tolstring(L, -1, &result_buffer_size);

    if (!result_buffer)
    {
        throw std::runtime_error("expected buffer as result from read");
    }

    if (result_buffer_size > size)
    {
        throw std::runtime_error("read more bytes than expected");
    }

    std::memcpy(buffer, result_buffer, std::min(size, result_buffer_size));

    return result_buffer_size;
}

std::size_t devi::LuaFile::write(const std::uint8_t* buffer, std::size_t size)
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, reference);

    lua_getfield(L, -1, "write");
    lua_pushvalue(L, -2);
    lua_pushlstring(L, (const char *)buffer, size);

    handle_error(lua_pcall(L, 2, 1, 0));

    auto num_bytes_written = luaL_optinteger(L, -1, 0);
    auto return_value_type = lua_type(L, -1);
    lua_pop(L, 2);

    if (lua_type(L, -1) != LUA_TNUMBER)
    {
        throw std::runtime_error("read should return then number of bytes written");
    }

    if (num_bytes_written < 0)
    {
        throw std::runtime_error("invalid number of bytes written (less than zero)");
    }

    return (std::size_t)num_bytes_written;
}

void devi::LuaFile::flush()
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, reference);

    lua_getfield(L, -1, "flush");
    lua_pushvalue(L, -2);

    handle_error(lua_pcall(L, 1, 0, 0));

    lua_pop(L, 1);
}

void devi::LuaFile::open()
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, reference);

    lua_getfield(L, -1, "open");
    lua_pushvalue(L, -2);

    handle_error(lua_pcall(L, 1, 0, 0));

    lua_pop(L, 1);
}

void devi::LuaFile::close()
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, reference);

    lua_getfield(L, -1, "close");
    lua_pushvalue(L, -2);

    handle_error(lua_pcall(L, 1, 0, 0));

    lua_pop(L, 1);
}

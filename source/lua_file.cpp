#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <string>

#include "devi/lua_file.hpp"

devi::LuaFile::LuaFile(lua_State* L, int index) : L(L)
{
    if (lua_type(L, index) == LUA_TSTRING)
    {
        reference = -1;

        std::size_t size;
        auto value = luaL_checklstring(L, index, &size);

        read_buffer.resize(size);
        std::memcpy(&read_buffer[0], value, size);
    }
    else if (lua_type(L, index) == LUA_TTABLE)
    {
        lua_pushvalue(L, index);
        reference = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    else
    {
        luaL_error(L, "expected table or string at index %d", index);
    }
}

devi::LuaFile::LuaFile(LuaFile&& other) noexcept : L(other.L), reference(other.reference), read_buffer(other.read_buffer), read_offset(other.read_offset)
{
    other.L = nullptr;
    other.read_buffer.clear();
    other.read_offset = 0;
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
    if (reference < 0)
    {
        auto end_offset = std::min(read_offset + size, read_buffer.size());
        auto num_bytes_read = end_offset - read_offset;

        std::memcpy(buffer, &read_buffer[read_offset], num_bytes_read);

        read_offset = end_offset;
        return num_bytes_read;
    }

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
    if (reference < 0)
    {
        throw std::runtime_error("cannot write to read-only buffer");
    }

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
    if (reference < 0)
    {
        throw std::runtime_error("cannot flush read-only buffer");
    }

    lua_rawgeti(L, LUA_REGISTRYINDEX, reference);

    lua_getfield(L, -1, "flush");
    lua_pushvalue(L, -2);

    handle_error(lua_pcall(L, 1, 0, 0));

    lua_pop(L, 1);
}

void devi::LuaFile::open()
{
    if (reference < 0)
    {
        read_offset = 0;
        return;
    }

    lua_rawgeti(L, LUA_REGISTRYINDEX, reference);

    lua_getfield(L, -1, "open");
    lua_pushvalue(L, -2);

    handle_error(lua_pcall(L, 1, 0, 0));

    lua_pop(L, 1);
}

void devi::LuaFile::close()
{
    if (reference < 0)
    {
        read_offset = read_buffer.size();
        return;
    }

    lua_rawgeti(L, LUA_REGISTRYINDEX, reference);

    lua_getfield(L, -1, "close");
    lua_pushvalue(L, -2);

    handle_error(lua_pcall(L, 1, 0, 0));

    lua_pop(L, 1);
}

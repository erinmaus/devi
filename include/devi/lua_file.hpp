#pragma once

#ifndef DEVI_LUA_FILE_HPP
#define DEVI_LUA_FILE_HPP

#include <cstdint>
#include <span>
#include "devi.hpp"

namespace devi
{
    class LuaFile
    {
    private:
        lua_State* L;
        int reference;
        std::vector<std::uint8_t> read_buffer;
        std::size_t read_offset = 0;

        void handle_error(int result);

    public:
        LuaFile(lua_State* L, int index);
        LuaFile(LuaFile&& other) noexcept;
        ~LuaFile();

        std::size_t read(std::uint8_t* buffer, std::size_t size);
        std::size_t write(const std::uint8_t* buffer, std::size_t size);

        void open();
        void close();

        void flush();
    };
}

#endif

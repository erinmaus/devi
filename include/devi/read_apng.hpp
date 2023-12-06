#pragma once

#ifndef DEVI_READ_APNG_HPP
#define DEVI_READ_APNG_HPP

#include "png.h"

#include "devi.hpp"
#include "image.hpp"
#include "lua_file.hpp"

namespace devi
{
    class APNGImageReader : public ImageReader
    {
    private:
        png_structp png_ptr = nullptr;
        png_infop info_ptr = nullptr;
        LuaFile file;
        int current_frame = 0;

        png_bytepp row_pointers = nullptr;

        void open();
        void save_stack();
        void release();

    public:
        APNGImageReader(LuaFile&& file);
        ~APNGImageReader();

        int get_width() const override;
        int get_height() const override;

        int get_num_frames() const override;
        int get_current_frame() const override;

        bool read(Frame& frame) override;
        virtual void restart() override;
    };
}

#endif

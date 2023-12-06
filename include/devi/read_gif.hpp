#pragma once

#ifndef DEVI_READ_GIF_HPP
#define DEVI_READ_GIF_HPP

#include "gif_lib.h"

#include "devi.hpp"
#include "image.hpp"
#include "lua_file.hpp"

namespace devi
{
    class GIFImageReader : public ImageReader
    {
    private:
        GifFileType* gif = nullptr;
        LuaFile file;
        int current_frame = 0;

        void open();
        void release();
        void render(Frame& frame, const GifPixelType* row, int y, int transparent_color);

    public:
        GIFImageReader(LuaFile &&file);
        ~GIFImageReader();

        int get_width() const override;
        int get_height() const override;

        int get_num_frames() const override;
        int get_current_frame() const override;

        bool read(Frame &frame) override;
        virtual void restart() override;
    };
}

#endif

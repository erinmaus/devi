#pragma once

#ifndef DEVI_IMAGE_HPP
#define DEVI_IMAGE_HPP

#include <cstdint>
#include <vector>

#include "devi.hpp"

namespace devi
{
    enum
    {
        // 'replace' in LÖVE
        BLEND_OP_SOURCE,

        // 'alpha' in LÖVE
        BLEND_OP_OVER
    };

    enum
    {
        DISPOSE_OP_NONE,
        DISPOSE_OP_BACKGROUND,
        DISPOSE_OP_PREVIOUS
    };

    struct Pixel
    {
        std::uint8_t red;
        std::uint8_t green;
        std::uint8_t blue;
        std::uint8_t alpha;
    };

    struct Frame
    {
        std::uint32_t x = 0;
        std::uint32_t y = 0;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        int blend_op = BLEND_OP_SOURCE;
        int dispose_op = DISPOSE_OP_BACKGROUND;
        float delay = 0.0f;
        std::vector<Pixel> pixels;
    };

    class ImageReader
    {
    public:
        virtual ~ImageReader() {}

        virtual int get_width() const = 0;
        virtual int get_height() const = 0;

        virtual int get_num_frames() const = 0;
        virtual int get_current_frame() const = 0;

        virtual bool read(Frame &frame) = 0;
        virtual void restart() = 0;
    };

    void push_image_reader(lua_State* L, ImageReader* reader);
}

#endif

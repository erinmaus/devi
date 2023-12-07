#include <csetjmp>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include "devi/read_apng.hpp"

devi::APNGImageReader::APNGImageReader(LuaFile&& file) : file(std::move(file))
{
    open();
}

devi::APNGImageReader::~APNGImageReader()
{
    release();
}

static void devi_apng_read(png_structp png_ptr, png_bytep buffer, png_size_t size)
{
    auto reader = (devi::LuaFile*)png_get_io_ptr(png_ptr);
    if (reader)
    {
        reader->read(buffer, size);
    }
}

void devi::APNGImageReader::open()
{
    release();

    file.open();

    std::uint8_t signature[8];
    if (file.read(signature, sizeof(signature)) != 8 || !png_check_sig(signature, sizeof(signature)))
    {
        throw std::runtime_error("couldn't read PNG signature or PNG signature not valid");
    }

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    info_ptr = png_create_info_struct(png_ptr);

    png_set_read_fn(png_ptr, &file, &devi_apng_read);
    png_set_sig_bytes(png_ptr, sizeof(signature));

    save_stack();
    png_read_info(png_ptr, info_ptr);

    if (!png_get_valid(png_ptr, info_ptr, PNG_INFO_acTL))
    {
        throw std::runtime_error("file is not animated");
    }

    row_pointers = (png_bytepp)png_malloc(png_ptr, sizeof(png_bytep) * get_height());
    if (!row_pointers)
    {
        throw std::runtime_error("could not allocate row pointers");
    }

    for (auto i = 0; i < get_height(); ++i)
    {
        row_pointers[i] = (png_bytep)png_malloc(png_ptr, png_get_rowbytes(png_ptr, info_ptr));
        if (!row_pointers)
        {
            for (auto j = 0; j < i; ++j)
            {
                png_free(png_ptr, row_pointers[i]);
                row_pointers[i] = nullptr;
            }

            png_free(png_ptr, row_pointers);
            row_pointers = nullptr;

            throw std::runtime_error("could not allocate row pointers");
        }
    }
}

void devi::APNGImageReader::save_stack()
{
    if (!png_ptr)
    {
        throw std::runtime_error("PNG not valid");
    }
    
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        throw std::runtime_error("error reading APNG");
    }
}

void devi::APNGImageReader::release()
{
    current_frame = 0;

    if (png_ptr && row_pointers)
    {
        for (auto i = 0; i < get_height(); ++i)
        {
            png_free(png_ptr, row_pointers[i]);
        }

        png_free(png_ptr, row_pointers);
        row_pointers = nullptr;
    }

    if (png_ptr && info_ptr)
    {
        png_read_end(png_ptr, info_ptr);
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

        png_ptr = nullptr;
        info_ptr = nullptr;
    }
}

int devi::APNGImageReader::get_width() const
{
    if (png_ptr && info_ptr)
    {
        return png_get_image_width(png_ptr, info_ptr);
    }

    return 0;
}

int devi::APNGImageReader::get_height() const
{
    if (png_ptr && info_ptr)
    {
        return png_get_image_width(png_ptr, info_ptr);
    }

    return 0;
}

int devi::APNGImageReader::get_current_frame() const
{
    return current_frame;
}

int devi::APNGImageReader::get_num_frames() const
{
    if (png_ptr && info_ptr)
    {
        return png_get_num_frames(png_ptr, info_ptr);
    }

    return 0;
}

bool devi::APNGImageReader::read(Frame& frame)
{
    while (current_frame < png_get_num_frames(png_ptr, info_ptr))
    {
        save_stack();
        png_read_frame_head(png_ptr, info_ptr);
        ++current_frame;

        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_fcTL))
        {
            std::uint16_t delay_numerator, delay_denominator;
            std::uint8_t dispose_op, blend_op;

            png_get_next_frame_fcTL(
                png_ptr, info_ptr,
                &frame.width, &frame.height,
                &frame.x, &frame.y,
                &delay_numerator, &delay_denominator,
                &dispose_op, &blend_op
            );

            switch (dispose_op)
            {
                case PNG_DISPOSE_OP_NONE:
                    frame.dispose_op = DISPOSE_OP_NONE;
                    break;
                
                case PNG_DISPOSE_OP_PREVIOUS:
                    frame.dispose_op = DISPOSE_OP_PREVIOUS;
                    break;

                case PNG_DISPOSE_OP_BACKGROUND:
                default:
                    frame.dispose_op = DISPOSE_OP_BACKGROUND;
                    break;
            }

            switch (blend_op)
            {
                case PNG_BLEND_OP_OVER:
                    frame.blend_op = BLEND_OP_OVER;
                    break;
                
                case PNG_BLEND_OP_SOURCE:
                default:
                    frame.blend_op = BLEND_OP_SOURCE;
                    break;
            }

            if (delay_denominator == 0)
            {
                delay_denominator = 100;
            }

            if (delay_numerator == 0)
            {
                frame.delay = 0;
            }
            else
            {
                frame.delay = (float)delay_numerator / (float)delay_denominator;
            }

            png_read_image(png_ptr, row_pointers);
            frame.pixels.clear();

            for (auto j = 0; j < frame.height; ++j)
            {
                for (auto i = 0; i < frame.width; ++i)
                {
                    switch (png_get_color_type(png_ptr, info_ptr))
                    {
                        case PNG_COLOR_TYPE_RGB:
                        {
                            auto pixel_byte_data = row_pointers[j] + 3 * i;
                            Pixel pixel = { pixel_byte_data[0], pixel_byte_data[1], pixel_byte_data[2], 255 };
                            frame.pixels.push_back(pixel);

                            break;
                        }

                        case PNG_COLOR_TYPE_RGBA:
                        {
                            auto pixel_byte_data = row_pointers[j] + 4 * i;
                            Pixel pixel = { pixel_byte_data[0], pixel_byte_data[1], pixel_byte_data[2], pixel_byte_data[3] };
                            frame.pixels.push_back(pixel);

                            break;
                        }
                    }
                }
            }

            return true;
        }
    }

    return false;
}

void devi::APNGImageReader::restart()
{
    open();
}

static int devi_apng_image_reader_new(lua_State* L)
{
    devi::LuaFile file(L, 1);

    devi::push_image_reader(L, new devi::APNGImageReader(std::move(file)));

    return 1;
}

extern "C"
int luaopen_devi_APNGImageReader(lua_State* L)
{
    devi::luax_pushcfunction(L, &devi_apng_image_reader_new);

    return 1;
}

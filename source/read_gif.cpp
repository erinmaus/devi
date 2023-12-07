#include <cstdint>
#include <utility>
#include <stdexcept>
#include "devi/read_gif.hpp"

devi::GIFImageReader::GIFImageReader(LuaFile&& file) : file(std::move(file))
{
    open();
}

devi::GIFImageReader::~GIFImageReader()
{
    release();
}

static int devi_gif_read(GifFileType* gif, GifByteType* buffer, int size)
{
    auto reader = (devi::LuaFile*)gif->UserData;
    if (reader)
    {
        return reader->read(buffer, size);
    }

    return 0;
}

void devi::GIFImageReader::open()
{
    release();

    file.open();

    gif = DGifOpen(&file, &devi_gif_read, nullptr);
    if (!gif)
    {
        throw std::runtime_error("could not open GIF");
    }
}

void devi::GIFImageReader::release()
{
    current_frame = 0;

    if (gif)
    {
        DGifCloseFile(gif, nullptr);
        gif = nullptr;
    }
}

void devi::GIFImageReader::render(Frame& frame, const GifPixelType* row, int y, int transparent_color)
{
    ColorMapObject* color_map = gif->Image.ColorMap ? gif->Image.ColorMap : gif->SColorMap;
    if (!color_map)
    {
        throw std::runtime_error("couldn't render GIF row; no color map found");
    }

    for (auto i = 0; i < frame.width; ++i)
    {
        auto color_index = row[i];

        auto& pixel = frame.pixels.at(y * frame.width + i);
        if (transparent_color >= 0 and color_index == transparent_color)
        {
            pixel.red = 0;
            pixel.green = 0;
            pixel.blue = 0;
            pixel.alpha = 0;
        }
        else if (color_index < color_map->ColorCount)
        {
            auto color_map_entry = &color_map->Colors[color_index];
            pixel.red = color_map_entry->Red;
            pixel.green = color_map_entry->Green;
            pixel.blue = color_map_entry->Blue;
            pixel.alpha = 255;
        }
        else
        {
            throw std::runtime_error("color index invalid or out of bounds");
        }
    }
}

int devi::GIFImageReader::get_width() const
{
    if (gif)
    {
        return gif->SWidth;
    }

    return 0;
}

int devi::GIFImageReader::get_height() const
{
    if (gif)
    {
        return gif->SHeight;
    }

    return 0;
}

int devi::GIFImageReader::get_current_frame() const
{
    return current_frame;
}

int devi::GIFImageReader::get_num_frames() const
{
    return 0;
}

bool devi::GIFImageReader::read(Frame& frame)
{
    static const int interlaced_offsets[] = { 0, 4, 2, 1 };
    static const int interlaced_jumps[] = { 8, 8, 4, 2 };

    GifRecordType type = UNDEFINED_RECORD_TYPE;

    int transparent_color = -1;
    while (type != IMAGE_DESC_RECORD_TYPE)
    {
        if (!DGifGetRecordType(gif, &type))
        {
            throw std::runtime_error("could not read GIF");
        }

        switch (type)
        {
            case EXTENSION_RECORD_TYPE:
            {
                GifByteType* gif_extension_buffer;
                int gif_extension_code;

                if (!DGifGetExtension(gif, &gif_extension_code, &gif_extension_buffer))
                {
                    throw std::runtime_error("could not read GIF extension block");
                }

                GraphicsControlBlock graphics_control_block;
                if (gif_extension_code == GRAPHICS_EXT_FUNC_CODE && gif_extension_buffer[0] == 4)
                {
                    DGifExtensionToGCB(4, &gif_extension_buffer[1], &graphics_control_block);

                    switch (graphics_control_block.DisposalMode)
                    {
                        case DISPOSE_DO_NOT:
                        default:
                            frame.dispose_op = DISPOSE_OP_NONE;
                            break;
                        case DISPOSE_BACKGROUND:
                            frame.dispose_op = DISPOSE_OP_BACKGROUND;
                            break;
                        case DISPOSE_PREVIOUS:
                            frame.dispose_op = DISPOSE_OP_PREVIOUS;
                            break;
                    }

                    frame.delay = graphics_control_block.DelayTime / 100.0f;
                    transparent_color = graphics_control_block.TransparentColor;
                }

                while (gif_extension_buffer)
                {
                    if (!DGifGetExtensionNext(gif, &gif_extension_buffer))
                    {
                        throw std::runtime_error("could not read next GIF extension block");
                    }
                }

                break;
            }

            case TERMINATE_RECORD_TYPE:
                return false;
            
            case IMAGE_DESC_RECORD_TYPE:
                break;
            
            default:
                throw std::runtime_error("unhandled GIF record type");
        }
    }

    if (!DGifGetImageDesc(gif))
    {
        throw std::runtime_error("error reading GIF frame");
    }

    frame.blend_op = BLEND_OP_OVER;
    frame.x = gif->Image.Left;
    frame.y = gif->Image.Top;
    frame.width = gif->Image.Width;
    frame.height = gif->Image.Height;
    frame.pixels.resize(frame.width * frame.height);

    std::vector<GifPixelType> gif_row;
    gif_row.resize(frame.width, transparent_color >= 0 ? transparent_color : 0);

    if (gif->Image.Interlace)
    {
        for (auto i = 0; i < 4; ++i)
        {
            for (auto j = interlaced_offsets[i]; j < frame.height; j += interlaced_jumps[i])
            {
                if (!DGifGetLine(gif, &gif_row[0], frame.width))
                {
                    throw std::runtime_error("couldn't read interlaced GIF row");
                }

                render(frame, &gif_row[0], j, transparent_color);
            }
        }
    }
    else
    {
        for (auto j = 0; j < frame.height; ++j)
        {
            if (!DGifGetLine(gif, &gif_row[0], frame.width))
            {
                throw std::runtime_error("couldn't read GIF row");
            }

            render(frame, &gif_row[0], j, transparent_color);
        }
    }

    ++current_frame;
    return true;
}

void devi::GIFImageReader::restart()
{
    open();
}

static int devi_gif_image_reader_new(lua_State* L)
{
    devi::LuaFile file(L, 1);

    devi::push_image_reader(L, new devi::GIFImageReader(std::move(file)));

    return 1;
}

extern "C"
int luaopen_devi_GIFImageReader(lua_State *L)
{
    devi::luax_pushcfunction(L, &devi_gif_image_reader_new);

    return 1;
}

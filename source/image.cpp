#include "devi/image.hpp"

static int devi_image_reader_get_width(lua_State* L)
{
    auto image_reader = *((devi::ImageReader**)luaL_checkudata(L, 1, "devi.ImageReader"));
    lua_pushinteger(L, image_reader->get_width());

    return 1;
}

static int devi_image_reader_get_height(lua_State *L)
{
    auto image_reader = *((devi::ImageReader **)luaL_checkudata(L, 1, "devi.ImageReader"));
    lua_pushinteger(L, image_reader->get_height());

    return 1;
}

static int devi_image_reader_get_num_frames(lua_State *L)
{
    auto image_reader = *((devi::ImageReader **)luaL_checkudata(L, 1, "devi.ImageReader"));
    lua_pushinteger(L, image_reader->get_num_frames());

    return 1;
}

static int devi_image_reader_get_current_frame(lua_State *L)
{
    auto image_reader = *((devi::ImageReader **)luaL_checkudata(L, 1, "devi.ImageReader"));
    lua_pushinteger(L, image_reader->get_current_frame());

    return 1;
}

static int devi_image_reader_read(lua_State *L)
{
    auto image_reader = *((devi::ImageReader **)luaL_checkudata(L, 1, "devi.ImageReader"));

    devi::Frame frame;
    if (image_reader->read(frame))
    {
        lua_newtable(L);

        lua_pushinteger(L, frame.x);
        lua_setfield(L, -2, "x");

        lua_pushinteger(L, frame.y);
        lua_setfield(L, -2, "y");

        lua_pushinteger(L, frame.width);
        lua_setfield(L, -2, "width");

        lua_pushinteger(L, frame.height);
        lua_setfield(L, -2, "height");

        lua_pushnumber(L, frame.delay);
        lua_setfield(L, -2, "delay");

        switch (frame.blend_op)
        {
            case devi::BLEND_OP_SOURCE:
            default:
                lua_pushstring(L, "replace");
                break;
            
            case devi::BLEND_OP_OVER:
                lua_pushstring(L, "alpha");
                break;
        }
        lua_setfield(L, -2, "blendMode");

        switch (frame.dispose_op)
        {
            case devi::DISPOSE_OP_BACKGROUND:
            default:
                lua_pushstring(L, "background");
                break;
            
            case devi::DISPOSE_OP_NONE:
                lua_pushstring(L, "none");
                break;
            
            case devi::DISPOSE_OP_PREVIOUS:
                lua_pushstring(L, "previous");
                break;
        }
        lua_setfield(L, -2, "dispose");

        lua_pushlstring(L, (const char*)&frame.pixels[0], frame.pixels.size() * sizeof(devi::Pixel));
        lua_setfield(L, -2, "pixels");

        return 1;
    }

    return 0;
}

static int devi_image_reader_restart(lua_State *L)
{
    auto image_reader = *((devi::ImageReader **)luaL_checkudata(L, 1, "devi.ImageReader"));
    image_reader->restart();

    return 0;
}

static int devi_image_reader_gc(lua_State *L)
{
    auto image_reader = *((devi::ImageReader **)luaL_checkudata(L, 1, "devi.ImageReader"));
    delete image_reader;

    return 0;
}

static luaL_Reg DEVI_IMAGE_READER_METHODS[] = {
    { "getWidth", &devi_image_reader_get_width },
    { "getHeight", &devi_image_reader_get_height },
    { "getNumFrames", &devi_image_reader_get_num_frames },
    { "getCurrentFrame", &devi_image_reader_get_current_frame },
    { "read", &devi_image_reader_read },
    { "restart", &devi_image_reader_restart },
    { nullptr, nullptr }
};

void devi::push_image_reader(lua_State* L, ImageReader* reader)
{
    auto image_reader = (devi::ImageReader**)lua_newuserdata(L, sizeof(devi::ImageReader*));
    *image_reader = reader;

    if (luaL_newmetatable(L, "devi.ImageReader"))
    {
        luax_register(L, DEVI_IMAGE_READER_METHODS);
        lua_setfield(L, -2, "__index");

        luax_pushcfunction(L, &devi_image_reader_gc);
        lua_setfield(L, -2, "__gc");
    }

    lua_setmetatable(L, -2);
}

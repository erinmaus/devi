# devi

devi is a GIF/APNG image loading library for LÖVE.

## Usage

1. Add the `devi` folder to your LÖVE project.
2. Add the `devi.dll`/`devi.dylib`/`devi.so` (depending on your platform) to a place that can be found by `love` (see the handy function [love.filesystem.setCRequirePath](https://love2d.org/wiki/love.filesystem.setCRequirePath) on the wiki).

```lua
local devi = require "devi"

function love.load()
    gif = devi.newImage("coolbear.gif")
end

function love.draw()
    local x, y = love.mouse.getPosition()
    gif:draw(x, y)
end
```

### Notes for Windows
If you use the builds from Github Actions, `devi.dll` and and all other DLLs provided will have to be put in the same folder as `love.exe` (fused or not). You'll have to build with Visual Studio if you want just a static DLL. (This is an exercise left to the reader.)

## Documentaion

`image = devi.newImage(file, { minDelay = 0, format = "png", file = false })`
* `file` should point to a valid APNG or GIF or be a LÖVE `Data` object containing a valid APNG or GIF.
* A second, optional parameter is a config table:
  * `minDelay` is the minimum time a frame from a GIF or APNG can last. **Be warned, if set to 0, an image with all 0 delays will freeze the game!**
  * `format` can be `gif` or `png`. This is only useful if the file lacks at extension or if you pass in a LÖVE `Data` object. devi will try and determine the right format even if this value is not provided or wrong.
  * If `file` is true, the file will be streamed. **This is very slow at the moment, but does use a lot less memory.** If false or not provided, then (if a filename is provided), the entire file will be read into a buffer and used to parse images.

```image:getWidth()```
* Returns the width of the image.

```image:getHeight()```
* Returns the height of the image.

```image:getCurrentFrameIndex()```
* Returns the current frame index of the image.

```image:draw(...)```
* Takes the same arguments as `love.graphics.draw` (including an optional quad, etc). Will update the current progress of the image as well.
* **You should use a pre-multiplied blend mode (e.g., `love.graphics.setBlendMode("alpha", "premultiplied")`) when drawing an APNG with alpha.**

```image:update()```
* If you don't want to use `image:draw()` but still want to update the animation for use with `image:getTexture()`. **Calling both `image:update()` and `image:draw()` is wasteful** - if you want to the GIF/APNG as a texture for a mesh or an input to a shader uniform, use `image:update()` and `image:getTexture()` otherwise just use `image:draw(...)`.
* Can be called in an update method.

```image:getTexture()```
* Gets the current texture. Must either have called `image:update()` **or** `image:draw()` at least once before and more preferrably once a frame.

## Building

devi can be built on Windows via MSYS2, Linux, and macOS. The handy `Makefile` will download static dependencies, compile them, and compile devi on these platforms. Pre-built binaries are provided by Github Actions as well.

## License

devi is licensed under the MPL. See the `LICENSE` file. This means you can use it in your projects pretty much however you want, but any modifications to devi must be returned to the community.

## Final Remarks

Read "I Feel Sick" by Jhonen Vasquez.

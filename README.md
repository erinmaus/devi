# devi

devi is a GIF/APNG image loading library for LÖVE.

## Usage

1. Add the `devi` folder to your LÖVE project.
2. Add the `devi.dll`/`devi.dylib`/`devi.so` (depending on your platform) to a place that can be found by `love` (see the handy function [love.filesystem.setCRequirePath](https://love2d.org/wiki/love.filesystem.setCRequirePath) on the wiki).

```lua
local devi = require "devi"

function love.load()
    gif = devi.newImage("gif", "coolbear.gif")
end

function love.draw()
    local x, y = love.mouse.getPosition()
    gif:draw(x, y)
end
```

## Documentaion

`image = devi.newImage(format, filename, { minDelay = 0 })`
* Format can be `gif` or `apng`.
* Filename should point to a valid APNG or GIF.
* A third, optional parameter is a config table. Currently only `minDelay` is supported; this is the minimum time a frame from a GIF or APNG can last. **Be warned, if set to 0, an image with all 0 delays will freeze the game!**

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

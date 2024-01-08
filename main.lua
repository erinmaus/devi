-- Copy the GitHub Actions shared libraries to the bin folder
local function copyLib()
    if love.system.getOS() == "Windows" then
        love.filesystem.write("devi.dll", love.filesystem.read("bin/devi.dll"))
    elseif love.system.getOS() == "OS X" then
        love.filesystem.write("devi.dylib", love.filesystem.read("bin/devi.dylib"))
    elseif love.system.getOS() == "Linux" then
        love.filesystem.write("devi.so", love.filesystem.read("bin/devi.so"))
    end
end

local devi = require "devi"

function love.load()
    copyLib()

    devi.init(love.filesystem.getSaveDirectory())
    gif = devi.newImage("coolbear.gif")
end

function love.draw()
    local x, y = love.mouse.getPosition()
    gif:draw(x, y)
end

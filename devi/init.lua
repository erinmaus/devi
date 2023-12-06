local APNGImageReader = require "devi.APNGImageReader"
local GIFImageReader = require "devi.GIFImageReader"

local devi = {}

local FileReader = {}
function FileReader:open()
    if self._file then
        self:close()
    end

    self._file = love.filesystem.newFile(self._filename)
    self._file:open(self._mode)
end

function FileReader:close()
    if self._file then
        self._file:close()
    end
end

function FileReader:read(numBytes)
    if not self._file then
        error("file not open")
    end

    local result, size = self._file:read(numBytes)
    return result:sub(1, size)
end

function FileReader:write(value)
    if not self._file then
        error("file not open")
    end

    local success, message = self._file:write(value)
    if not success then
        error(message)
    end

    return #success
end

function FileReader:flush()
    if not self._file then
        error("file not open")
    end

    self._file:flush()
end

local FileReaderType = { __index = FileReader }

local function newFile(filename, mode)
    return setmetatable({
        _filename = filename,
        _mode = mode
    }, FileReaderType)
end

local READERS = {
    ["apng"] = APNGImageReader,
    ["gif"] = GIFImageReader
}

local Image = {}

function Image:_init()
    self._canvas = love.graphics.newCanvas(self:getWidth(), self:getHeight())
end

function Image:getWidth()
    return self._reader:getWidth()
end

function Image:getHeight()
    return self._reader:getHeight()
end

function Image:getNumFrames()
    return self._reader:getNumFrames()
end

function Image:getCurrentFrameIndex()
    return self._reader:getCurrentFrame()
end

function Image:_render(frame)
    local imageData = love.image.newImageData(frame.width, frame.height, 'rgba8', frame.pixels)
    local image = love.graphics.newImage(imageData)

    love.graphics.push('all')

    love.graphics.origin()
    love.graphics.setShader()
    love.graphics.setCanvas(self._canvas)

    love.graphics.setBlendMode('replace')
    if frame.dispose == 'none' and self._currentImage then
        love.graphics.draw(self._currentImage, 0, 0)
    elseif frame.dispose == 'background' then
        love.graphics.clear(0, 0, 0, 0)
    elseif frame.dispose == 'previous' and self._previousImage then
        love.graphics.draw(self._previousImage, 0, 0)
    else
        -- Error state.
        love.graphics.clear(1, 0, 0, 1)
    end

    love.graphics.setBlendMode(frame.blendMode)
    love.graphics.draw(image, frame.x, frame.y)

    love.graphics.setCanvas()

    self._previousImage = self._currentImage
    self._currentImage = love.graphics.newImage(self._canvas:newImageData())

    love.graphics.pop('all')
end

function Image:_update()
    local currentTime = love.timer.getTime()
    local difference = currentTime - self._currentTime

    self._currentDelay = self._currentDelay - difference

    while self._currentDelay < 0 do
        local frame = self._reader:read()
        if not frame then
            self._reader:restart()
            self._currentDelay = self._currentDelay + self._minDelay
        else
            self._currentDelay = self._currentDelay + math.max(frame.delay, self._minDelay)
            self:_render(frame)
        end

        if self:getCurrentFrameIndex() == self:getNumFrames() then
            self._reader:restart()
        end
    end

    self._currentTime = currentTime
end

function Image:getTexture()
    return self._currentImage
end

function Image:update()
    self:_update()
end

function Image:draw(...)
    self:_update()

    if self._currentImage then
        love.graphics.draw(self._currentImage, ...)
    end
end

local ImageType = { __index = Image }

local DEFAULT_CONFIG = {
    minDelay = 1 / 60
}

function devi.newImage(format, filename, config)
    config = config or DEFAULT_CONFIG

    local NativeImageReader = READERS[format]

    if not NativeImageReader then
        error(string.format("unsupported image reader format: %s", tostring(format)), 1)
    end

    local result = setmetatable({
        _format = format,
        _reader = NativeImageReader(newFile(filename, 'r')),
        _currentTime = love.timer.getTime(),
        _currentDelay = 0,
        _minDelay = config.minDelay or DEFAULT_CONFIG.minDelay
    }, ImageType)
    result:_init()

    return result
end

return devi

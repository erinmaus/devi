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

local DataReader = {}
function DataReader:open()
    self._offset = 0
end

function DataReader:close()
    self._offset = nil
end

function DataReader:read(numBytes)
    if not self._offset then
        error("file not open")
    end

    local buffer = self._data:getString():sub(self._offset + 1, self._offset + numBytes)
    self._offset = self._offset + numBytes

    return buffer
end

function DataReader:write()
    error("cannot write to data")
end

function DataReader:flush()
    -- Nothing,
end

local DataReaderType = { __index = DataReader }

local function newFile(file, mode)
    if type(file) == "string" then
        return setmetatable({
            _filename = file,
            _mode = mode
        }, FileReaderType)
    elseif file:typeOf("Data") then
        if mode == "r" then
            return setmetatable({
                _data = file
            }, DataReaderType)
        else
            error("cannot write to data")
        end
    else
        error("expected filename or Data")
    end
end

local function newBuffer(file)
    if type(file) == "string" then
        return love.filesystem.read(file)
    elseif file:typeOf("Data") then
        return file:getString()
    else
        error("expected filename or Data")
    end
end

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
    love.graphics.setColor(1, 1, 1, 1)
    love.graphics.setScissor()
    love.graphics.setShader()
    love.graphics.setCanvas(self._canvas)

    love.graphics.setBlendMode("replace")
    if frame.dispose == "none" and self._currentImage then
        love.graphics.draw(self._currentImage, 0, 0)
    elseif frame.dispose == "background" then
        love.graphics.clear(0, 0, 0, 0)
    elseif frame.dispose == "previous" and self._previousImage then
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

local READERS = {}

local function tryLoad(format, filename, config)
    local NativeImageReader = READERS[format]

    if not NativeImageReader then
        return false
    end

    local success, file, reader
    
    if config.file then
        success, file = pcall(newFile, filename, "r")
        if not success then
            return false
        end
    else
        success, file = pcall(newBuffer, filename)
        if not success then
            return false
        end
    end

    success, reader = pcall(NativeImageReader, file)
    if not success then
        return false
    end

    local result = setmetatable({
        _format = format,
        _reader = reader,
        _currentTime = love.timer.getTime(),
        _currentDelay = 0,
        _minDelay = config.minDelay or DEFAULT_CONFIG.minDelay,
    }, ImageType)
    result:_init()

    return result
end

function devi.init(sourcePath)
    if sourcePath then
        local newCPath = string.format(
            "%s/?.dll;%s/?.so;%s/?.dylib;%s",
            sourcePath,
            sourcePath,
            sourcePath,
            package.cpath)
        package.cpath = newCPath
    end

    local APNGImageReader = require "devi.APNGImageReader"
    local GIFImageReader = require "devi.GIFImageReader"

    READERS.png = APNGImageReader
    READERS.gif = GIFImageReader
end

function devi.newImage(file, config)
    if not next(READERS, nil) then
        devi.init()
    end

    config = config or DEFAULT_CONFIG
    
    local defaultFormat = config.format or (type(file) == "string" and file:match(".*%.(%w+)$"))
    defaultFormat = defaultFormat and defaultFormat:lower()

    local result
    if defaultFormat then
        result = tryLoad(defaultFormat, file, config)
    end

    if not result then
        for format in pairs(READERS) do
            if format ~= defaultFormat then
                result = tryLoad(format, file, config)
                if result then
                    break
                end
            end
        end
    end

    if not result then
        if type(file) == "string" then
            error(string.format("couldn't load %s: not a valid animated image", file))
        else
            error(string.format("couldn't load %s: not a valid animated image", file:type()))
        end
    end

    return result
end

return devi

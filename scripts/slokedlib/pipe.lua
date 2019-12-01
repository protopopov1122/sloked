local promise = require 'slokedlib/promise'

local Pipe = {}
Pipe.__index = Pipe

local PromisifiedPipe = {}
PromisifiedPipe.__index = PromisifiedPipe

function Pipe:new(pipe)
    local obj = {
        pipe=pipe,
        listeners={}
    }
    setmetatable(obj, self)
    obj.pipe:listen(function()
        if #obj.listeners > 0 then
            local callback = table.remove(obj.listeners, 1)
            callback()
        end
    end)
    return obj
end

function Pipe:promisify(pipe)
    return PromisifiedPipe:new(pipe)
end

function applyCallback(callback, that, fn, ...)
    local status, result = pcall(fn, that, ...)
    if status then
        callback(nil, result)
    else
        callback(result, nil)
    end
end

function Pipe:isOpen()
    return self.pipe:isOpen()
end

function Pipe:count()
    return self.pipe:count()
end

function Pipe:empty()
    return self.pipe:empty()
end

function Pipe:close(callback)
    sloked.sched:defer(function ()
        applyCallback(callback, self.pipe, self.pipe.close)
    end)
end

function Pipe:read(callback)
    if not self:empty() then
        applyCallback(callback, self.pipe, self.pipe.read)
    else
        self.listeners[#self.listeners + 1] = function()
            applyCallback(callback, self.pipe, self.pipe.read)
        end
    end
end

function Pipe:wait(callback, count)
    if self:count() >= count then
        applyCallback(callback, self.pipe, self.pipe.wait, count)
    else
        self.listeners[#self.listeners + 1] = function()
            self:wait(callback, count)
        end
    end
end

function Pipe:drop(callback, count)
    if self:count() >= count then
        applyCallback(callback, self.pipe, self.pipe.drop, count)
    else
        self.listeners[#self.listeners + 1] = function()
            self:drop(callback, count)
        end
    end
end

function Pipe:dropAll(callback)
    applyCallback(callback, self.pipe, self.pipe.dropAll)
end

function Pipe:write(callback, value)
    applyCallback(callback, self.pipe, self.pipe.write, value)
end

function PromisifiedPipe:new(pipe)
    local obj = {
        pipe=Pipe:new(pipe)
    }
    setmetatable(obj, self)
    return obj
end

function PromisifiedPipe:isOpen()
    return self.pipe:isOpen()
end

function PromisifiedPipe:count()
    return self.pipe:count()
end

function PromisifiedPipe:empty()
    return self.pipe:empty()
end

function PromisifiedPipe:close()
    return promise:promisify(self.pipe, self.pipe.close)
end

function PromisifiedPipe:read()
    return promise:promisify(self.pipe, self.pipe.read)
end

function PromisifiedPipe:wait(count)
    return promise:promisify(self.pipe, self.pipe.wait, count)
end

function PromisifiedPipe:drop(count)
    return promise:promisify(self.pipe, self.pipe.drop, count)
end

function PromisifiedPipe:dropAll()
    return promise:promisify(self.pipe, self.pipe.dropAll)
end

function PromisifiedPipe:write(value)
    return promise:promisify(self.pipe, self.pipe.write, value)
end

return Pipe
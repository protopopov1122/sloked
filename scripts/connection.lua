local Connection = {}
Connection.__index = Connection

function sendAll(pipe, msgs)
    for key, message in pairs(msgs) do
        if key > 1 then
            pipe:write(message)
        end
    end
end

function checkStatus(coro, pipe)
    if coroutine.status(coro) ~= 'suspended' then
        pipe:listen(nil)
    end
end

function Connection:new(pipe, handler)
    local obj = {
        pipe=pipe,
        coro=coroutine.create(handler),
        skip_count=0
    }
    setmetatable(obj, self)
    
    obj.pipe:listen(function()
        local value = obj.pipe:read()
        if obj.skip_count == 0 then
            sendAll(obj, {coroutine.resume(obj.coro, value)})
            checkStatus(obj.coro, obj.pipe)
        else
            obj.skip_count = obj.skip_count - 1
        end
    end)
    sendAll(obj, {coroutine.resume(obj.coro, obj)})
    checkStatus(obj.coro, obj.pipe)
    return obj
end

function Connection:isOpen()
    return self.pipe:isOpen()
end

function Connection:count()
    return self.pipe:count()
end

function Connection:empty()
    return self.pipe:empty()
end

function Connection:drop(count)
    if count > self:count() then
        count = self:count()
    end
    self.pipe:drop(count)
    return count
end

function Connection:skip(count)
    self.skip_count = self.skip_count + count
    return self.skip_count
end

function Connection:receive(...)
    return coroutine.yield(...)
end

function Connection:dropAll()
    local count = self.count()
    self.pipe:drop()
    return count
end

function Connection:read()
    return self.pipe:read()
end

function Connection:tryRead()
    return self.pipe:tryRead()
end

function Connection:write(value)
    self.pipe:write(value)
    return self
end

function Connection:tryWrite(value)
    self.pipe:tryWrite(value)
    return self
end

function Connection:close()
    self.pipe:close()
end

return Connection
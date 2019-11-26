Connection = {}
Connection.__index = Connection

function Connection:new(pipe, handler)
    local obj = {
        pipe=pipe,
        coro=coroutine.create(handler)
    }
    setmetatable(obj, self)
    
    obj.pipe:listen(function()
        value = obj.pipe:read()
        obj:_sendAll({coroutine.resume(obj.coro, value)})
        obj:_checkStatus()
    end)
    obj:_sendAll({coroutine.resume(obj.coro, obj)})
    obj:_checkStatus()
    return obj
end

function Connection:write(value)
    self.pipe:write(value)
end

function Connection:close()
    self.pipe:close()
end

function Connection:_checkStatus()
    if coroutine.status(self.coro) ~= 'suspended' then
        self.pipe:listen(nil)
    end
end

function Connection:_sendAll(msgs)
    for key, message in pairs(msgs) do
        if key > 1 then
            self.pipe:write(message)
        end
    end
end

Connection:new(sloked.servers.main:connect('document::cursor'), function(conn)
    local cursor = conn
    if coroutine.yield({
        id=0,
        method='connect',
        params=1
    }) then
        Connection:new(sloked.servers.main:connect('document::notify'), function(conn)
            conn:write(1)
            local i = 0
            while i < 5 do
                coroutine.yield()
                i = i + 1
                cursor:write({
                    id=1,
                    method='insert',
                    params='Hello, world!'
                })
                coroutine.yield()
            end
        end)
    end
end)


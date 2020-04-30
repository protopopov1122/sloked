local promise = require 'slokedlib/promise'

local Mutex = {}
Mutex.__index = Mutex

function Mutex:new()
    local mtx = {
        locked=false,
        queue={},
        owner=nil
    }
    setmetatable(mtx, self)
    return mtx
end

function Mutex:lock()
    if not self.locked then
        self.locked = true
        self.owner = coroutine.running()
        return promise:resolve()
    else
        return promise:new(function(resolve, reject)
            self.queue[#self.queue + 1] = resolve
        end):next(function()
            return self:lock()
        end)
    end
end

function Mutex:unlock()
    if self.locked and self.owner == coroutine.running() then
        self.locked = false
        self.owner = nil
        for key, callback in pairs(self.queue) do
            callback()
        end
        self.queue = {}
    else
        error('Mutex not locked by current thread')
    end
end

function Mutex:tryLock()
    if not self.locked then
        self.locked = true
        return true
    else
        return false
    end
end

return Mutex
local promise = require 'slokedlib/promise'

local Async = {}
Async.__call = function(tbl, handler)
    local coro = coroutine.create(handler)
    local status, result = coroutine.resume(coro, function(promise)
        return coroutine.yield(promise)
    end)
    local onFulfill = function(self, result)
        if coroutine.status(coro) == 'suspended' then
            local status, next = coroutine.resume(coro, result, nil)
            if status then
                if promise:is(next) then
                    return next:next(self.onFulfill, self.onReject)
                else
                    return promise:resolve(next)
                end
            else
                return promise:reject(next)
            end
        else
            return promise:resolve(result)
        end
    end
    local onReject = function(self, result)
        if coroutine.status(coro) == 'suspended' then
            local status, next = coroutine.resume(coro, nil, result)
            if status then
                if promise:is(next) then
                    return next:next(self.onFulfill, self.onReject)
                else
                    return promise:resolve(next)
                end
            else
                return promise:reject(next)
            end
        else
            return promise:reject(result)
        end
    end
    local callbackSelf = {}
    callbackSelf.onFulfill = function(value)
        return onFulfill(callbackSelf, value)
    end
    callbackSelf.onReject = function(value)
        return onReject(callbackSelf, value)
    end
    if status then
        if promise:is(result) then
            return result:next(callbackSelf.onFulfill, callbackSelf.onReject)
        else
            return promise:resolve(result)
        end
    else
        return promise:reject(result)
    end
end

function async_unwrap(await)
    return function(coro)
        local result, err = await(coro)
        if err then
            error(err, 0)
        else
            return result
        end
    end
end

local async = {
    unwrap=async_unwrap
}
setmetatable(async, Async)

return async
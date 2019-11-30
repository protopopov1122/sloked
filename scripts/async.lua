local Promise = require 'promise'

function async(handler)
    local thread, running = coroutine.running()
    if not running then
        return Promise:new(function(resolve, reject)
            local status, result = pcall(handler, function(promise)
                return coroutine.yield(promise)
            end)
            if status then
                resolve(result)
            else
                reject(result)
            end
        end)
    end
    local coro = coroutine.create(handler)
    local status, result = coroutine.resume(coro, function(promise)
        return coroutine.yield(promise)
    end)
    function onFulfill(result)
        if coroutine.status(coro) == 'suspended' then
            local status, result = coroutine.resume(coro, result, nil)
            if status then
                if Promise:is(result) then
                    return result:next(onFulfill, onReject)
                else
                    return Promise:resolve(result)
                end
            else
                return Promise:reject(result)
            end
        else
            return result
        end
    end
    function onReject(result)
        if coroutine.status(coro) == 'suspended' then
            local status, result = coroutine.resume(coro, nil, result)
            if status then
                if Promise:is(result) then
                    return result:next(onFulfill, onReject)
                else
                    return Promise:resolve(result)
                end
            else
                return Promise:reject(result)
            end
        else
            return result
        end
    end
    if status then
        if Promise:is(result) then
            return result:next(onFulfill, onReject)
        else
            return Promise:resolve(result)
        end
    else
        return Promise:reject(result)
    end
end

return async
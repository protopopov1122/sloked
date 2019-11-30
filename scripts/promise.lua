local Promise = {}
Promise.__index = Promise
local Promisified = {}

local Pending = 0
local Fulfilled = 1
local Rejected = 2

function Promise:new(process)
    local promise = {
        value=nil,
        state=Pending,
        on_fulfill={},
        on_reject={} 
    }
    setmetatable(promise, self)
    local status, err = pcall(process, function(result)
        promise:_fulfill(result)
    end, function(err)
        promise:_reject(err)
    end)
    if not status then
        promise:_reject(err)
    end
    return promise
end

function Promise:resolve(value)
    return Promise:new(function(resolve, reject)
        resolve(value)
    end)
end

function Promise:reject(value)
    return Promise:new(function(resolve, reject)
        reject(value)
    end)
end

function Promise:promisify(obj, fn, ...)
    local args = {...}
    if obj then
        return Promise:new(function(resolve, reject)
            local status, result = pcall(fn, obj, function(err, result)
                if err then
                    reject(err)
                else
                    resolve(result)
                end
            end, unpack(args))
            if not status then
                reject(result)
            end
        end)
    else
        return Promise:new(function(resolve, reject)
            local status, result = pcall(fn, function(err, result)
                if err then
                    reject(err)
                else
                    resolve(result)
                end
            end, unpack(args))
            if not status then
                reject(result)
            end
        end)
    end
end

function Promise:onFulfill(callback)
    if self.state == Pending then
        self.on_fulfill[#self.on_fulfill + 1] = callback
    elseif self.state == Fulfilled then
        callback(self.value)
    end
end

function Promise:onReject(callback)
    if self.state == Pending then
        self.on_reject[#self.on_reject + 1] = callback
    elseif self.state == Rejected then
        callback(self.value)
    end
end

function Promise:_fulfill(value)
    if self.state == Pending then
        self.value = value
        self.state = Fulfilled
        local on_fulfill = self.on_fulfill
        self.on_fulfill = {}
        for key, callback in pairs(on_fulfill) do
            callback(self.value)
        end
    else
        error("Non-pending promise can't be fulfilled")
    end
end

function Promise:_reject(value)
    if self.state == Pending then
        self.value = value
        self.state = Rejected
        local on_reject = self.on_reject
        self.on_reject = {}
        for key, callback in pairs(on_reject) do
            callback(self.value)
        end
    else
        error("Non-pending promise can't be rejected")
    end
end

function isPromise(obj)
    return type(obj) == 'table' and getmetatable(obj) == Promise
end

function Promise:is(obj)
    return isPromise(obj)
end

function promiseNext(callback, arg, resolve, reject)
    local status, res = pcall(callback, arg)
    if status then
        if isPromise(res) then
            res:next(resolve, reject)
        else
            resolve(res)
        end
    else
        if isPromise(res) then
            res:next(resolve, reject)
        else
            reject(res)
        end
    end
end

function Promise:next(onFulfill, onReject)
    if onReject == nil then
        onReject = function(err)
            return Promise:reject(err)
        end
    end
    return Promise:new(function(resolve, reject)
        self:onFulfill(function(result)
            promiseNext(onFulfill, result, resolve, reject)
        end)
        self:onReject(function(err)
            promiseNext(onReject, err, resolve, reject)
        end)
    end)
end

function Promise:catch(onReject)
    return self:next(function(result)
        return result
    end, onReject)
end

function Promise:finally(callback)
    return self:next(function (result)
        return callback(nil, result)
    end, function(err)
        return callback(err, nil)
    end)
end

function Promise:unwrapError()
    self:onReject(function(err)
        sloked.sched:defer(function()
            error(err, 0)
        end)
    end)
end

return Promise
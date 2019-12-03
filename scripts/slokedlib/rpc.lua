local pipe = require 'slokedlib/pipe'
local promise = require 'slokedlib/promise'
local async = require 'slokedlib/async'
local sched = require 'slokedlib/sched'
local mutex = require 'slokedlib/mutex'

local RpcResponse = {}
RpcResponse.__index = RpcResponse

function RpcResponse:new(client, id)
    local obj = {
        client=client,
        id=id,
        queue={}
    }
    setmetatable(obj, self)
    client.responses[id] = obj
    return obj
end

function RpcResponse:close()
    if self.id ~= nil then
        table.remove(self.client.responses, self.id)
        self.id = nil
    end
end

function RpcResponse:__call()
    return async(function(await)
        local await_unwrap = async.unwrap(await)
        while #self.queue == 0 do
            await(self.client:_receive(function()
                return #self.queue == 0
            end))
        end
        return table.remove(self.queue, 1)
    end)
end

local RpcClient = {}
RpcClient.__index = RpcClient

function RpcClient:new(p)
    local obj = {
        pipe=pipe:promisify(p),
        nextId=0,
        responses={},
        mtx=mutex:new()
    }
    setmetatable(obj, self)
    return obj
end

function RpcClient:invoke(method, params, detach)
    if type(detach) ~= 'boolean' then
        detach = false
    end
    local id = self.nextId
    self.nextId = id + 1
    local writeReq = self.pipe:write({
        id=id,
        method=method,
        params=params
    })
    if detach then
        return writeReq
    else
        return writeReq:next(function()
            return RpcResponse:new(self, id)
        end)
    end
end

function RpcClient:__call(method, params)
    return async(function(await)
        local await_unwrap = async.unwrap(await)
        local req = await_unwrap(self:invoke(method, params))
        local res = await_unwrap(req())
        req:close()
        return res
    end)
end

function RpcClient:send(method, params)
    return self:invoke(method, params, true)
end

function RpcClient:_receive(continue_callback)
    return async(function(await)
        local await_unwrap = async.unwrap(await)
        repeat
            await_unwrap(self.mtx:lock())
            if continue_callback() then
                local msg = await_unwrap(self.pipe:read())
                if msg ~= nil and self.responses[msg.id] then
                    local queue = self.responses[msg.id].queue
                    queue[#queue + 1] = msg
                end
            end
            self.mtx:unlock()
        until self.pipe:empty() or not continue_callback()
    end)
end

return RpcClient
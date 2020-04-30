local promise = require 'slokedlib/promise'

    local sched = {}

                  function sched
                      .defer() return promise
    : new (function(resolve, reject) sloked.sched
           : defer(resolve) end) end

          function sched.sleep(timeout) return promise
    : new (function(resolve, reject) sloked.sched
           : setTimeout(resolve, timeout) end) end

      return sched
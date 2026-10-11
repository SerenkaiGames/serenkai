--!nolint FunctionUnused
local Event = require("serenkai:scripts/event.luau")

function on_update(dt: number)
	Event.emit("serenkai:events/update", dt)
end

function emit(event: string, ...: any)
	Event.emit(event, ...)
end

log.info("Main luau initialization successful.")

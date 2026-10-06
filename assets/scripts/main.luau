--!nolint FunctionUnused
local Event = require("serenkai:scripts/event.luau")

function on_update(dt: number)
	Event.emit("serenkai:events/update", dt)
end

print("Main luau initialization successful.")

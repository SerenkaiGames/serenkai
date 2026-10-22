--!nolint FunctionUnused
local Event = require("serenkai:scripts/event.luau")

function on_update(dt: number)
	Event.emit("serenkai:events/update", dt)
end

Loc.load("en_US", "locales/en_US.po")
Loc.set("en_US")

log.info("Main luau initialization successful.")

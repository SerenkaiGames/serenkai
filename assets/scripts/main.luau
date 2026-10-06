--!nolint FunctionUnused
local lifecycle = require("serenkai:scripts/lifecycle.luau")

function on_update(dt: number)
	lifecycle.emit("update", dt)
end

print("Main luau initialization successful.")

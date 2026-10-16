--!nolint FunctionUnused

local id: EntityId

local speed: number = 100

local Event = require("serenkai:scripts/event.luau")

local cleanups: { Cleanup } = {}

function on_init(e_id: EntityId)
	id = e_id

	local c = Event.on("serenkai:events/update_game", on_update)
	Event.on("serenkai:events/leave_game", World.Entities.destroy(id))

	table.insert(cleanups, c)
end

function on_update(dt: number)
	update_move(dt)
	update_zoom()
end

function update_move(dt: number)
	local dx = 0
	local dy = 0

	if Input.is_down(Input.Action["inputs/move_up"]) then
		dy -= dt * speed
	end

	if Input.is_down(Input.Action["inputs/move_down"]) then
		dy += dt * speed
	end

	if Input.is_down(Input.Action["inputs/move_left"]) then
		dx -= dt * speed
	end

	if Input.is_down(Input.Action["inputs/move_right"]) then
		dx += dt * speed
	end

	if dx ~= 0 or dy ~= 0 then
		World.Entities.set_prop(id, "props/pos", vector.create(dx, dy, 1)) -- z = 1 means on the ground.
		World.Camera.move(vector.create(dx, dy, 0))
	end
end

function update_zoom()
	if Input.is_pressed(Input.Action["inputs/zoom_in"]) then
		World.Camera.add_zoom(1)
	end

	if Input.is_pressed(Input.Action["inputs/zoom_out"]) then
		World.Camera.add_zoom(-1)
	end
end

function on_destroy()
	for _, f in cleanups do
		pcall(f)
	end
end

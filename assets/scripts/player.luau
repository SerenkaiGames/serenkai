--!nolint FunctionUnused

local id: EntityId

local speed: number = 100

local EntitySystem = require("serenkai:scripts/entity_system.luau")

function on_init(e_id: EntityId)
	id = e_id

	EntitySystem.on(id, "serenkai:events/update_game", on_update)
	EntitySystem.on(id, "serenkai:events/leave_game", function()
		EntitySystem.destroy(id)
	end)
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
		World.Entities.move(id, vector.create(dx, dy, 1)) -- z = 1 means on the ground.
		local player_pos = World.Entities.get_prop(id, "props/pos")
		World.Camera.set_pos(vector.create(player_pos.x, player_pos.y, 0))
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
	EntitySystem.cleanup(id)
end

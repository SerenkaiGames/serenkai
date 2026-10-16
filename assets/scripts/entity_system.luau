local EntitySystem = {}

function EntitySystem.create(loc: string)
	return World.Entities.create_with_loc(loc)
end

return EntitySystem

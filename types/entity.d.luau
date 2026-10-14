-- entity.d.luau

export type EntityId = number

export type EntityPropertyValue = boolean | number | string | vector

export type EntityManager = {
	NULL_ID: EntityId,
	create: (self: EntityManager) -> EntityId,
	destroy: (self: EntityManager, id: EntityId) -> (),
	alive: (self: EntityManager, id: EntityId) -> boolean,
	alive_size: (self: EntityManager) -> number,

	set_prop: (self: EntityManager, id: EntityId, loc: string, value: EntityPropertyValue) -> boolean,
	get_prop: (self: EntityManager, id: EntityId, loc: string) -> EntityPropertyValue?,
	has_prop: (self: EntityManager, id: EntityId, loc: string) -> boolean,
	remove_prop: (self: EntityManager, id: EntityId, loc: string) -> boolean,
}

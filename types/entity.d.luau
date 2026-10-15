-- entity.d.luau

export type EntityId = number

export type EntityPropertyValue = boolean | number | string | vector

export type WorldEntities = {
	NULL_ID: EntityId,
	create: () -> EntityId,
	destroy: (id: EntityId) -> (),
	alive: (id: EntityId) -> boolean,
	alive_size: () -> number,

	set_prop: (id: EntityId, loc: string, value: EntityPropertyValue) -> boolean,
	get_prop: (id: EntityId, loc: string) -> EntityPropertyValue?,
	has_prop: (id: EntityId, loc: string) -> boolean,
	remove_prop: (id: EntityId, loc: string) -> boolean,
}

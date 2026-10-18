-- entity.d.luau

export type EntityId = number

export type EntityPropertyValue = boolean | number | string | vector

export type WorldEntities = {
	NULL_ID: EntityId,
	create: () -> EntityId,
	create_with_loc: (loc: string) -> EntityId,

	destroy: (id: EntityId) -> (),
	alive: (id: EntityId) -> boolean,
	alive_size: () -> number,

	set_prop: (id: EntityId, loc: string, value: EntityPropertyValue) -> boolean,
	get_prop: (id: EntityId, loc: string) -> EntityPropertyValue?,
	has_prop: (id: EntityId, loc: string) -> boolean,
	remove_prop: (id: EntityId, loc: string) -> boolean,

	play_animation: (id: EntityId, clip: string) -> boolean,
	play_animation_once: (id: EntityId, clip: string) -> boolean,
	stop_animation: (id: EntityId) -> (),

	move: (id: EntityId, dist: vector) -> (),
}

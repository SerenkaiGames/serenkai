-- input.d.luau

export type ActionId = number

export type ActionTable = { [string]: ActionId }

export type Input = {
	Action: ActionTable,
	is_down: (id: ActionId) -> boolean,
	is_pressed: (id: ActionId) -> boolean,
	is_released: (id: ActionId) -> boolean,
	get_held_mask: () -> any,
}

declare Input: Input

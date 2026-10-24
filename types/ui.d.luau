-- ui.d.luau

export type Ui = {
    open: (loc: string) -> (),
    close: (loc: string) -> (),
    clear: () -> (),
    reload: () -> (),
}

declare Ui: Ui
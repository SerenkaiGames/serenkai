-- localization.d.luau

export type FormatArg = { [string]: string }

export type Localization = {
    current: () -> string,
    fallback: () -> string,
    set: (lang: string) -> boolean,
    set_fallback: (lang: string) -> boolean,
    load: (lang: string, loc: string) -> boolean,
    has: (key: string) -> boolean,
}

declare function Tr(key: string, args: FormatArg? ): string

declare Loc: Localization
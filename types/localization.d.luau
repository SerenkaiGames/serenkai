-- localization.d.luau

export type FormatArg = { [string]: string }

export type Localization = {
    current_language: () -> string,
    fallback_language: () -> string,
    set_language: (lang: string) -> boolean,
    set_fallback_language: (lang: string) -> boolean,
    load_language: (lang: string, loc: string) -> boolean,
    has: (key: string) -> boolean,
}

declare function Tr(key: string, args: { [string]: any }?): string

declare Loc: Localization
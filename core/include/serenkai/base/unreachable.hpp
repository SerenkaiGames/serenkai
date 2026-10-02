#pragma once
#include "serenkai/base/panic.hpp"

namespace serenkai {

[[noreturn]] inline void
todo(std::source_location loc = std::source_location::current()) {
    panic_at("not yet implemented", loc);
}

[[noreturn]] inline void
todo(std::string_view msg,
     std::source_location loc = std::source_location::current()) {
    panic_at(msg, loc);
}

[[noreturn]] inline void
unimplemented(std::source_location loc = std::source_location::current()) {
    panic_at("unimplemented", loc);
}

[[noreturn]] inline void
unimplemented(std::string_view msg,
              std::source_location loc = std::source_location::current()) {
    panic_at(msg, loc);
}

[[noreturn]] inline void
unreachable(std::source_location loc = std::source_location::current()) {
    panic_at("entered unreachable code", loc);
}

[[noreturn]] inline void
unreachable(std::string_view msg,
            std::source_location loc = std::source_location::current()) {
    panic_at(msg, loc);
}

} // namespace serenkai
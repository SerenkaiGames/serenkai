#pragma once

#include "serenkai/application/event.hpp"

#include <SDL3/SDL_events.h>
#include <optional>
namespace serenkai::input {

std::optional<Event> process_sdl_event(const SDL_Event& e);

} // namespace serenkai::input
#pragma once

#include "serenkai/application/event.hpp"

#include <SDL3/SDL_events.h>
#include <optional>
namespace serenkai {
class GuiContext;
namespace input {

/// @brief Converts an SDL event to a custom event.
std::optional<Event> process_sdl_event(const SDL_Event& e, GuiContext* context);
} // namespace input
} // namespace serenkai

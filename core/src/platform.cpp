#include "serenkai/platform.hpp"

#include "serenkai/application/application.hpp"

#include <SDL3/SDL.h>
#include <exception>
#include <spdlog/spdlog.h>

namespace serenkai::core {
int run() {
    try {
        Application app;
        app.run();
    } catch (const std::exception& e) {
        spdlog::error("Application error: {}", e.what());
        return 1;
    } catch (...) {
        spdlog::error("Application error: unknown error");
        return 1;
    }

    return 0;
}
} // namespace serenkai::core

#include "serenkai/platform.hpp"

#include "serenkai/application/application.hpp"

#include <SDL3/SDL.h>
#include <exception>
#include <spdlog/spdlog.h>

namespace serenkai {
int start_game(int argc, char** argv) {
    try {
        Application app{argc, argv};
        app.run();

    } catch (const Application::ExitException& e) {

        return e.code;
    } catch (const std::exception& e) {
        spdlog::error("Application error: {}", e.what());
        return 1;
    } catch (...) {
        spdlog::error("Application error: unknown error");
        return 1;
    }

    return 0;
}
} // namespace serenkai

#include "serenkai/gui/ui_manager.hpp"

#include "serenkai/gui/widget_parser.hpp"
#include "serenkai/resource/resource_location.hpp"

#include <spdlog/spdlog.h>

namespace serenkai {
UiManager::UiManager(const WidgetParserConfig& config) : m_parser(config) {}

void UiManager::open(std::string_view loc) {
    auto view = m_parser.parse(loc);
    if (!view) {
        return;
    }

    bind_callbacks(view.get(), view->widget());
    m_views.push_back(std::move(view));
}
void UiManager::close(std::string_view loc) {
    auto res = ResourceLocation::parse(loc);
    if (!res) {
        spdlog::error("Invalid resource location {}", loc);
        return;
    }

    std::erase_if(m_views, [&res](const std::unique_ptr<UiView>& view) {
        if (!view) {
            return true;
        }

        return view->loc() == res;
    });
}
void UiManager::clear() { m_views.clear(); }

void UiManager::reload_all() {}

} // namespace serenkai
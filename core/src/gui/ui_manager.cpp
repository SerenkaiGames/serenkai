#include "serenkai/gui/ui_manager.hpp"

#include "serenkai/gui/button.hpp"
#include "serenkai/gui/widget_parser.hpp"
#include "serenkai/resource/resource_location.hpp"

#include <algorithm>
#include <spdlog/spdlog.h>
#include <utility>
#include <variant>

namespace serenkai {
UiManager::UiManager(const WidgetParserConfig& config) : m_parser(config) {}

void UiManager::request_open(std::string_view loc) {
    auto res = ResourceLocation::parse(loc);
    if (!res) {
        spdlog::error("Invalid resource location {}", loc);
        return;
    }

    m_operations.emplace_back(Action::Open, std::move(*res));
}

void UiManager::request_close(std::string_view loc) {
    auto res = ResourceLocation::parse(loc);
    if (!res) {
        spdlog::error("Invalid resource location {}", loc);
        return;
    }
    m_operations.emplace_back(Action::Close, std::move(*res));
}

void UiManager::request_clear() {
    m_operations.emplace_back(Action::Clear, std::monostate{});
}

void UiManager::request_reload_all() {
    m_operations.emplace_back(Action::Reload, std::monostate{});
}

void UiManager::update(float dt) {
    handle_operation();

    for (auto& view : m_views) {
        if (view) {
            view->update(dt);
        }
    }
}
void UiManager::render(GuiContext* ctx) {
    for (auto& view : m_views) {
        if (view) {
            view->render(ctx);
        }
    }
}
bool UiManager::handle_event(const Event& e) {
    for (auto it = m_views.rbegin(); it != m_views.rend(); ++it) {
        if (*it) {
            if ((*it)->handle_event(e)) {
                return true;
            }
        }
    }
    return false;
}

void UiManager::handle_operation() {
    while (!m_operations.empty()) {
        auto& o = m_operations.front();
        switch (o.first) {
        case Action::Open:
            if (auto s = std::get_if<ResourceLocation>(&o.second)) {
                open(*s);
            }
            break;
        case Action::Close:
            if (auto s = std::get_if<ResourceLocation>(&o.second)) {
                close(*s);
            }
            break;
        case Action::Clear:
            clear();
            break;
        case Action::Reload:
            reload_all();
            break;
        }

        m_operations.pop_front();
    }
}

void UiManager::open(const ResourceLocation& loc) {

    if (std::ranges::any_of(m_views,
                            [&loc](const std::unique_ptr<UiView>& view) {
                                return view && view->loc() == loc;
                            })) {
        return;
    }

    auto view = m_parser.parse(loc);
    if (!view) {
        return;
    }

    bind_callbacks(view.get(), view->widget());
    m_views.push_back(std::move(view));
}

void UiManager::close(const ResourceLocation& loc) {
    std::erase_if(m_views, [&loc](const std::unique_ptr<UiView>& view) {
        if (!view) {
            return true;
        }

        return view->loc() == loc;
    });
}

void UiManager::clear() { m_views.clear(); }

void UiManager::reload_all() {
    for (auto& view : m_views) {
        if (!view) {
            continue;
        }
        auto new_view = m_parser.parse(view->loc());
        if (new_view) {
            bind_callbacks(new_view.get(), new_view->widget());
            view = std::move(new_view);
        }
    }
}

void UiManager::bind_callbacks(UiView* view, Widget* widget) {
    if (!widget) {
        return;
    }

    if (auto* button = dynamic_cast<Button*>(widget)) {
        if (!button->callback_name().empty()) {
            std::string cb_name{button->callback_name()};
            button->set_clicked([this, view, cb_name = std::move(cb_name)]() {
                invoke_callback(view, cb_name);
            });
        }
    }

    for (const auto& child : widget->children()) {
        bind_callbacks(view, child.get());
    }
}

void UiManager::invoke_callback(UiView* view, std::string_view cb_name) {
    if (view && view->script()) {
        if (view->script()->try_call<>(cb_name).has_value()) {
            return;
        }
    }

    auto it = m_callbacks.find(cb_name);
    if (it != m_callbacks.end()) {
        if (it->second) {
            it->second();
        }
        return;
    }

    spdlog::error("Can't find UI callback: {}", cb_name);
}

} // namespace serenkai
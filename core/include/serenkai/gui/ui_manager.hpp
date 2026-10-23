#pragma once

#include "serenkai/base/string_hash.hpp"
#include "serenkai/gui/ui_view.hpp"
#include "serenkai/gui/widget.hpp"
#include "serenkai/gui/widget_parser.hpp"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
namespace serenkai {
class AssetManager;
class UiManager {
public:
    enum class Action { Open, Close, Clear, Reload };

    explicit UiManager(const WidgetParserConfig& config);

    void request_open(std::string_view loc);
    void request_close(std::string_view loc);
    void request_clear();

    void request_reload_all();

    template <std::invocable Fn>
    void register_callback(std::string_view name, Fn f) {
        m_callbacks.try_emplace(std::string(name), std::move(f));
    }

private:
    using Callback = std::function<void()>;

    std::unordered_map<std::string, Callback, StringHash, StringEqual>
        m_callbacks;
    WidgetParser m_parser;
    std::vector<std::unique_ptr<UiView>> m_views;

    void open(std::string_view loc);
    void close(std::string_view loc);
    void clear();

    void reload_all();

    void bind_callbacks(UiView* view, Widget* widget);
    void invoke_callback(UiView* view, std::string_view cb_name);
};
} // namespace serenkai
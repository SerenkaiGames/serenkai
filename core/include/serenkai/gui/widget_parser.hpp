#pragma once

#include "serenkai/base/enum_meta.hpp"
#include "serenkai/gui/ui_view.hpp"
#include "serenkai/gui/widget.hpp"
#include "serenkai/resource/font_manager.hpp"

#include <concepts>
#include <functional>
#include <glaze/json/generic_fwd.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
namespace serenkai {

class AssetManager;
class TextureManager;
class LocalizationManager;
class ScriptEngine;

struct WidgetParserConfig {
    AssetManager* asset_manager = nullptr;
    FontManager* font_manager = nullptr;
    TextureManager* texture_manager = nullptr;
    LocalizationManager* localization_manager = nullptr;
    ScriptEngine* script_engine = nullptr;
};

/// @brief Constructs UI widgets from JSON.
class WidgetParser {

public:
    explicit WidgetParser(const WidgetParserConfig& config);
    std::unique_ptr<UiView> parse(std::string_view path);

    template <typename Fn>
        requires std::invocable<Fn, std::string_view, const glz::generic&>
    void register_factory(std::string_view name, Fn f) {
        m_factories.try_emplace(std::string(name), std::move(f));
    }

private:
    using CreateFunc = std::function<std::unique_ptr<Widget>(
        std::string_view key, const glz::generic& json)>;

    std::unordered_map<std::string, CreateFunc> m_factories;

    AssetManager* m_asset_manager = nullptr;
    FontManager* m_font_manager = nullptr;
    TextureManager* m_texture_manager = nullptr;
    LocalizationManager* m_localization_manager = nullptr;
    ScriptEngine* m_script_engine = nullptr;

    /// @brief Recursively traverse the JSON tree to construct the widget tree.
    ///
    /// @param json The input JSON data. Must contain a field for the name of a
    /// UI component, and an optional script field.
    std::unique_ptr<Widget> walk(const glz::generic& json) const;

    std::unique_ptr<Widget> parse_widget(std::string_view name,
                                         const glz::generic& json) const;

    std::unique_ptr<Widget> parse_label(std::string_view name,
                                        const glz::generic& json) const;
    std::unique_ptr<Widget> parse_rect(std::string_view name,
                                       const glz::generic& json) const;
    std::unique_ptr<Widget> parse_button(std::string_view name,
                                         const glz::generic& json) const;
    std::unique_ptr<Widget> parse_image(std::string_view name,
                                        const glz::generic& json) const;
    std::unique_ptr<Widget> parse_column(std::string_view name,
                                         const glz::generic& json) const;
    /// @brief Function that automatically handles the children field
    ///
    /// @param json JSON object containing the children field
    void parse_children(Widget* widget, const glz::generic& json) const;
};
} // namespace serenkai

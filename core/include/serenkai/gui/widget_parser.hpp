#pragma once

#include "serenkai/base/enum_meta.hpp"
#include "serenkai/gui/widget.hpp"
#include "serenkai/resource/font_manager.hpp"

#include <functional>
#include <glaze/json/generic_fwd.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
namespace serenkai {

class AssetManager;

/// @brief Constructs UI widgets from JSON.
class WidgetParser {

public:
    explicit WidgetParser(AssetManager* asset_manager,
                          FontManager* font_manager);
    std::unique_ptr<Widget> parse(std::string_view path);

private:
    using CreateFunc = std::function<std::unique_ptr<Widget>(
        std::string_view key, const glz::generic& json)>;

    using Callback = std::function<void()>;

    std::unordered_map<std::string, CreateFunc> m_factories;
    std::unordered_map<std::string, Callback> m_callbacks;

    AssetManager* m_asset_manager = nullptr;
    FontManager* m_font_manager = nullptr;

    /// @brief Recursively traverse the JSON tree to construct the widget tree.
    ///
    /// @param json The input JSON data. It must be a JSON object containing
    ///             exactly one field, whose name is the widget name and whose
    ///             value is a JSON object.
    std::unique_ptr<Widget> walk(const glz::generic& json) const;

    std::unique_ptr<Widget> parse_label(std::string_view name,
                                        const glz::generic& json) const;
    std::unique_ptr<Widget> parse_rect(std::string_view name,
                                       const glz::generic& json) const;
    std::unique_ptr<Widget> parse_button(std::string_view name,
                                         const glz::generic& json) const;

    /// @brief Function that automatically handles the children field
    ///
    /// @param json JSON object containing the children field
    void handle_children(Widget* widget, const glz::generic& json) const;
};
} // namespace serenkai
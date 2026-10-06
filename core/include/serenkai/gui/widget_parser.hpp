#pragma once

#include "serenkai/base/enum_meta.hpp"
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

struct WidgetParserConfig {
    AssetManager* asset_manager = nullptr;
    FontManager* font_manager = nullptr;
    TextureManager* texture_manager = nullptr;
};

/// @brief Constructs UI widgets from JSON.
class WidgetParser {

public:
    explicit WidgetParser(const WidgetParserConfig& config);
    std::unique_ptr<Widget> parse(std::string_view path);

    template <typename Fn>
        requires std::invocable<Fn, std::string_view, const glz::generic&>
    void register_factory(std::string_view name, Fn f) {
        m_factories.try_emplace(std::string(name), std::move(f));
    }

    template <std::invocable Fn>
    void register_callback(std::string_view name, Fn f) {
        m_callbacks.try_emplace(std::string(name), std::move(f));
    }

private:
    using CreateFunc = std::function<std::unique_ptr<Widget>(
        std::string_view key, const glz::generic& json)>;

    using Callback = std::function<void()>;

    std::unordered_map<std::string, CreateFunc> m_factories;
    std::unordered_map<std::string, Callback> m_callbacks;

    AssetManager* m_asset_manager = nullptr;
    FontManager* m_font_manager = nullptr;
    TextureManager* m_texture_manager = nullptr;

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
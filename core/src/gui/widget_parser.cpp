#include "serenkai/gui/widget_parser.hpp"

#include "serenkai/base/assert.hpp"
#include "serenkai/base/concepts.hpp"
#include "serenkai/base/type_name.hpp"
#include "serenkai/gui/anchor.hpp"
#include "serenkai/gui/button.hpp"
#include "serenkai/gui/color.hpp"
#include "serenkai/gui/column_layout.hpp"
#include "serenkai/gui/image_widget.hpp"
#include "serenkai/gui/label.hpp"
#include "serenkai/gui/rect.hpp"
#include "serenkai/gui/widget.hpp"
#include "serenkai/resource/asset_manager.hpp"
#include "serenkai/resource/font_manager.hpp"
#include "serenkai/resource/texture_manager.hpp"

#include <cstddef>
#include <glaze/core/reflect.hpp>
#include <glaze/glaze.hpp>
#include <glaze/json/generic_fwd.hpp>
#include <glaze/json/read.hpp>
#include <glm/ext/vector_int2.hpp>
#include <memory>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <utility>

namespace serenkai {
struct Root {
    glz::generic root;
};

struct LabelData {
    std::string text;
    size_t pixel_size = FontManager::DEFAULT_PIXEL_SIZE;
    std::string font = FontManager::DEFAULT_FONT;
    Anchor anchor = Anchor::TopLeft;
    glm::ivec2 offset{0, 0};
    Color color = Color::White;
};

struct RectData {

    std::optional<glm::ivec2> size;
    std::optional<bool> fill_parent;

    float alpha = 1.0f;
    Anchor anchor = Anchor::TopLeft;
    glm::ivec2 offset{0, 0};
    Color color = Color::White;
};

struct ButtonData {
    Anchor anchor = Anchor::TopLeft;
    glm::ivec2 offset{0, 0};
    std::optional<std::string> callback;
};

struct ImageWidgetData {
    Anchor anchor = Anchor::TopLeft;
    glm::ivec2 offset{0, 0};
    std::optional<glm::ivec2> size;
    std::string image;
};

struct ColumnData {
    Anchor anchor = Anchor::TopLeft;
    glm::ivec2 offset{0, 0};
    ChildAnchor child_anchor = ChildAnchor::Left;
    int spacing = 0;
};

} // namespace serenkai

template <> struct glz::meta<glm::ivec2> {
    using T = glm::ivec2;
    static constexpr auto value = glz::array(&T::x, &T::y); // NOLINT
};

template <> struct glz::meta<serenkai::ImageWidgetData> {
    static constexpr bool requires_key(std::string_view key, bool) {

        if (key == "image") {
            return true;
        }

        return false;
    }
};

namespace serenkai {

namespace {

void print_debug_json(const glz::generic& json) {
#ifndef NDEBUG
    std::string out{};
    auto ec = glz::write<glz::opts{.prettify = true}>(json, out);
    if (!ec) {
        spdlog::error("{}", out);
    } else {
        spdlog::error("Failed to write json");
    }
#endif
}

template <PlainType T>
std::optional<T> get(const glz::generic& json, std::string_view key) {
    if (!json.contains(key)) {
        spdlog::error("Widget json doesn't contain '{}' field", key);
        print_debug_json(json);
        return std::nullopt;
    }

    if (auto value = json[key].get_if<T>()) {
        return *value;
    } else {
        spdlog::error("Failed to get value, key {}, type {}", key,
                      type_name<T>());
        return std::nullopt;
    }
}

} // namespace

WidgetParser::WidgetParser(const WidgetParserConfig& config)
    : m_asset_manager(config.asset_manager),
      m_font_manager(config.font_manager),
      m_texture_manager(config.texture_manager) {

    register_factory("label",
                     [this](std::string_view name, const glz::generic& json) {
                         return parse_label(name, json);
                     });
    register_factory("rect",
                     [this](std::string_view name, const glz::generic& json) {
                         return parse_rect(name, json);
                     });
    register_factory("button",
                     [this](std::string_view name, const glz::generic& json) {
                         return parse_button(name, json);
                     });
    register_factory("image",
                     [this](std::string_view name, const glz::generic& json) {
                         return parse_image(name, json);
                     });
    register_factory("column",
                     [this](std::string_view name, const glz::generic& json) {
                         return parse_column(name, json);
                     });
}

std::unique_ptr<Widget> WidgetParser::parse(std::string_view loc) {
    spdlog::info("Parsing widget {} ...", loc);
    if (!m_asset_manager) {
        spdlog::error("AssetManager is nullptr");
        SE_ASSERT(false);
        return nullptr;
    }

    auto path = m_asset_manager->get(loc);

    if (!path) {
        spdlog::error("Unknown resource location {}", loc);
        return nullptr;
    }

    std::string buffer{};
    Root r{};

    auto ec = glz::read_file_json(r, *path, buffer);

    if (ec) {
        spdlog::error("Failed to read or parse {}, {}", loc,
                      glz::format_error(ec));
        return nullptr;
    }

    return walk(r.root);
}

std::unique_ptr<Widget> WidgetParser::walk(const glz::generic& json) const {

    if (!json.is_object()) {
        spdlog::error("Json is not an object");
        print_debug_json(json);
        return nullptr;
    }
    const auto& obj = json.get_object();
    if (obj.size() != 1) {
        spdlog::error("Json size is {} instead of 1", obj.size());
        print_debug_json(json);
        return nullptr;
    }

    auto& [name, value] = *obj.begin();

    auto type = get<std::string>(value, "type");
    if (!type) {
        return nullptr;
    }

    auto it = m_factories.find(*type);
    if (it == m_factories.end()) {
        spdlog::error("Unknown widget type {}", *type);
        print_debug_json(value);
        return nullptr;
    }
    return it->second(name, value);
}

void WidgetParser::handle_children(Widget* widget,
                                   const glz::generic& json) const {
    if (!widget) {
        return;
    }

    if (!json.is_object()) {
        return;
    }

    if (!json.contains("children")) {
        return;
    }

    if (!json["children"].is_array()) {
        spdlog::error("Widget json error, children is not an array");
        print_debug_json(json);
        return;
    }

    auto& children = json["children"].get_array();

    for (auto& child : children) {
        auto c = walk(child);
        if (c) {
            widget->add_child(std::move(c));
        }
    }
}

std::unique_ptr<Widget>
WidgetParser::parse_label(std::string_view name,
                          const glz::generic& json) const {
    auto label = std::make_unique<Label>(name, nullptr);
    LabelData data{};
    auto ec = glz::read<glz::opts{.error_on_unknown_keys = false}>(data, json);
    if (ec) {
        spdlog::error("Failed to read {}, {}", name, glz::format_error(ec));
        print_debug_json(json);
        return nullptr;
    }

    auto* font = m_font_manager
                     ? m_font_manager->get(data.font, data.pixel_size)
                     : nullptr;
    label->set_font(font);
    label->set_color(data.color);
    label->set_text(data.text);
    label->set_anchor(data.anchor);
    label->set_offset(data.offset);

    handle_children(label.get(), json);

    return label;
}

std::unique_ptr<Widget>
WidgetParser::parse_rect(std::string_view name,
                         const glz::generic& json) const {

    auto rect = std::make_unique<Rect>(name, nullptr);
    RectData data{};

    auto ec = glz::read<glz::opts{.error_on_unknown_keys = false}>(data, json);
    if (ec) {
        spdlog::error("Failed to read {}, {}", name, glz::format_error(ec));
        print_debug_json(json);
        return nullptr;
    }

    if (!data.size.has_value() && !data.fill_parent.has_value()) {
        spdlog::error("Failed to parse rect {}: must specify either 'size' or "
                      "'fill_parent'",
                      name);
        print_debug_json(json);
        return nullptr;
    }

    rect->set_alpha(data.alpha);
    rect->set_color(data.color);
    rect->set_anchor(data.anchor);
    rect->set_offset(data.offset);

    if (data.fill_parent) {
        rect->set_fill_parent(*data.fill_parent);
    }

    if (data.size) {
        rect->set_size(*data.size);
    }

    handle_children(rect.get(), json);

    return rect;
}

std::unique_ptr<Widget>
WidgetParser::parse_button(std::string_view name,
                           const glz::generic& json) const {
    auto button = std::make_unique<Button>(name, nullptr);
    ButtonData data{};

    auto ec = glz::read<glz::opts{.error_on_unknown_keys = false}>(data, json);
    if (ec) {
        spdlog::error("Failed to read {}, {}", name, glz::format_error(ec));
        print_debug_json(json);
        return nullptr;
    }

    button->set_anchor(data.anchor);
    button->set_offset(data.offset);
    if (data.callback) {
        auto it = m_callbacks.find(*data.callback);
        if (it != m_callbacks.end()) {
            button->set_clicked(it->second);
        } else {
            spdlog::error("Can't find button {} callback {}", name,
                          *data.callback);
        }
    }

    handle_children(button.get(), json);
    return button;
}

std::unique_ptr<Widget>
WidgetParser::parse_image(std::string_view name,
                          const glz::generic& json) const {

    if (!m_texture_manager) {
        spdlog::error("TextureManager is nullptr");
        SE_ASSERT(false);
        return nullptr;
    }

    auto image = std::make_unique<ImageWidget>(name, nullptr);
    ImageWidgetData data{};

    auto ec = glz::read<glz::opts{.error_on_unknown_keys = false,
                                  .error_on_missing_keys = true}>(data, json);
    if (ec) {
        spdlog::error("Failed to read {}, {}", name, glz::format_error(ec));
        print_debug_json(json);
        return nullptr;
    }

    image->set_image(data.image);
    image->set_anchor(data.anchor);
    image->set_offset(data.offset);
    if (data.size) {
        image->set_size(*data.size);
    } else {
        auto size = m_texture_manager->measure_size(data.image);
        image->set_size(size);
    }

    handle_children(image.get(), json);
    return image;
}

std::unique_ptr<Widget>
WidgetParser::parse_column(std::string_view name,
                           const glz::generic& json) const {
    auto column = std::make_unique<ColumnLayout>(name, nullptr);
    ColumnData data{};

    auto ec = glz::read<glz::opts{.error_on_unknown_keys = false,
                                  .error_on_missing_keys = true}>(data, json);
    if (ec) {
        spdlog::error("Failed to read {}, {}", name, glz::format_error(ec));
        print_debug_json(json);
        return nullptr;
    }

    column->set_spacing(data.spacing);
    column->set_child_anchor(data.child_anchor);
    column->set_anchor(data.anchor);
    column->set_offset(data.offset);

    handle_children(column.get(), json);
    column->layout();
    return column;
}

} // namespace serenkai
#include "serenkai/gui/widget_parser.hpp"

#include "serenkai/base/assert.hpp"
#include "serenkai/base/concepts.hpp"
#include "serenkai/base/type_name.hpp"
#include "serenkai/gui/anchor.hpp"
#include "serenkai/gui/color.hpp"
#include "serenkai/gui/label.hpp"
#include "serenkai/gui/widget.hpp"
#include "serenkai/resource/asset_manager.hpp"
#include "serenkai/resource/font_manager.hpp"

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

template <> struct glz::meta<glm::ivec2> {
    using T = glm::ivec2;
    static constexpr auto value = glz::array(&T::x, &T::y); // NOLINT
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

WidgetParser::WidgetParser(AssetManager* asset_manager,
                           FontManager* font_manager)
    : m_asset_manager(asset_manager), m_font_manager(font_manager) {

    m_factories.try_emplace(
        "label", [this](std::string_view name, const glz::generic& json) {
            return parse_label(name, json);
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

std::unique_ptr<Widget>
WidgetParser::parse_label(std::string_view name,
                          const glz::generic& json) const {
    auto label = std::make_unique<Label>(std::string(name), nullptr);
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

    if (!json.contains("children")) {
        return label;
    }

    if (!json["children"].is_array()) {
        spdlog::error("Label {} json: children is not an array", name);
        print_debug_json(json);
        return label;
    }

    auto& children = json["children"].get_array();

    for (auto& child : children) {
        auto c = walk(child);
        if (c) {
            label->add_child(std::move(c));
        }
    }

    return label;
}

} // namespace serenkai
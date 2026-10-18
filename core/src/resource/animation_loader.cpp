#include "serenkai/resource/animation_loader.hpp"

#include "serenkai/base/enum_meta.hpp"
#include "serenkai/resource/animation_data.hpp"
#include "serenkai/resource/asset_manager.hpp"
#include "serenkai/resource/resource_location.hpp"
#include "serenkai/resource/resource_utils.hpp"

#include <glaze/core/reflect.hpp>
#include <glaze/glaze.hpp>
#include <glaze/json/read.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/glm.hpp>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <vector>

struct Frame {
    float x, y, w, h;
};

template <> struct glz::meta<Frame> {
    using T = Frame;
    static constexpr auto value = // NOLINT
        glz::object("x", &T::x, "y", &T::y, "w", &T::w, "h", &T::h);
};

template <> struct glz::from<glz::JSON, glm::vec4> {
    template <auto Opts>
    static void op(glm::vec4& value, is_context auto&& ctx, auto&& it,
                   auto&& end) {
        Frame f{};
        glz::from<glz::JSON, Frame>::op<Opts>(f, ctx, it, end);
        value = glm::vec4(f.x, f.y, f.w, f.h);
    }
};

struct Size {
    int w, h;
};

template <> struct glz::meta<Size> {
    using T = Size;
    static constexpr auto value = glz::object("w", &T::w, "h", &T::h); // NOLINT
};

template <> struct glz::from<glz::JSON, glm::vec2> {
    template <auto Opts>
    static void op(glm::vec2& value, is_context auto&& ctx, auto&& it,
                   auto&& end) {
        Size s{};
        glz::from<glz::JSON, Size>::op<Opts>(s, ctx, it, end);
        value = glm::vec2(float(s.w), float(s.h));
    }
};

namespace serenkai {

struct AsepriteFrame {
    glm::vec4 frame{0.0f};
    bool trimmed = false;
    glm::vec4 spriteSourceSize{0.0f}; // NOLINT
    glm::vec2 sourceSize{0.0f};       // NOLINT
    float duration = 0.0f;
};

struct AsepriteFrameTag {
    std::string name;
    int from = 0;
    int to = 0;
    PlaybackMode direction = PlaybackMode::Forward;
};

struct AsepriteMeta {
    std::string image;
    std::vector<AsepriteFrameTag> frameTags; // NOLINT
};

struct AsepriteData {
    std::vector<AsepriteFrame> frames;
    AsepriteMeta meta;
};

namespace {

std::optional<SpriteAnimationData>
convert_aseprite(const AsepriteData& raw, const ResourceLocation& anim_loc) {
    auto image_loc = convert_aseprite_to_location(raw.meta.image, anim_loc);
    if (!image_loc) {
        spdlog::error("Failed to resolve image location '{}' for animation {}",
                      raw.meta.image, anim_loc.str());
        return std::nullopt;
    }

    SpriteAnimationData result{};
    result.image = *image_loc;
    result.frames.reserve(raw.frames.size());

    for (const auto& f : raw.frames) {
        SpriteFrame frame{};

        frame.src = f.frame;
        frame.offset =
            f.trimmed ? glm::vec2(f.spriteSourceSize.x, f.spriteSourceSize.y)
                      : glm::vec2(0.0f);
        frame.source_size = f.sourceSize;
        // Convert milliseconds to seconds
        frame.duration = f.duration / 1000.0f;
        result.frames.push_back(frame);
    }

    for (const auto& tag : raw.meta.frameTags) {
        if (tag.from < 0 || tag.to >= static_cast<int>(result.frames.size()) ||
            tag.from > tag.to) {
            spdlog::warn("Invalid frameTag '{}' range [{}-{}] in {}", tag.name,
                         tag.from, tag.to, anim_loc.str());
            continue;
        }

        // Put the action into {ns}:{anim_name}/{action}.
        // The tag must be in the form {anim_name}/{action}; ns is optional.
        std::string tag_loc_str =
            tag.name.find(':') != std::string::npos
                ? tag.name
                : fmt::format("{}:{}", anim_loc.ns(), tag.name);

        auto tag_loc = ResourceLocation::parse(tag_loc_str);
        if (!tag_loc) {
            spdlog::warn("Failed to parse ResourceLocation for frameTag '{}'",
                         tag.name);
            continue;
        }

        result.clips[*tag_loc] = AnimationClip{
            .name = tag.name,
            .from = tag.from,
            .to = tag.to,
            .mode = tag.direction,
        };
    }

    return result;
}
} // namespace

AnimationLoader::AnimationLoader(AssetManager* manager)
    : m_asset_manager(manager) {}

std::optional<SpriteAnimationData>
AnimationLoader::load(const ResourceLocation& loc) {
    if (!m_asset_manager) {
        return std::nullopt;
    }

    auto path = m_asset_manager->get(loc);
    if (!path) {
        spdlog::error("Failed to get {} path", loc.str());
        return std::nullopt;
    }

    AsepriteData data{};

    auto ec = glz::read_file_json<glz::opts{.error_on_unknown_keys = false}>(
        data, *path, std::string{});

    if (ec) {
        spdlog::error("Failed to parse json {}, {}", loc.str(),
                      glz::format_error(ec));
        return std::nullopt;
    }

    return convert_aseprite(data, loc);
}

std::optional<SpriteAnimationData> AnimationLoader::load(std::string_view loc) {
    auto res = ResourceLocation::parse(loc);
    if (!res) {
        spdlog::error("Invalid resource location {}", loc);
        return std::nullopt;
    }

    return load(*res);
}

} // namespace serenkai
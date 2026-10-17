#pragma once

#include "serenkai/resource/resource_location.hpp"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <magic_enum/magic_enum.hpp>
#include <string>
#include <unordered_map>
namespace serenkai {

enum class PlaybackMode {
    Forward,
    Reverse,
    Pingpong,
    PingpongReverse,
};

struct SpriteFrame {
    glm::vec4 src{0.0f};
    glm::vec2 offset{0.0f};
    glm::vec2 source_size{0.0f};
    float duration = 0.0f;
};

struct AnimationClip {
    std::string name;
    int from = 0;
    int to = 0;
    PlaybackMode mode = PlaybackMode::Forward;
};

struct SpriteAnimationData {
    ResourceLocation image;
    std::vector<SpriteFrame> frames;
    std::unordered_map<ResourceLocation, AnimationClip> clips;
};

} // namespace serenkai

template <>
constexpr magic_enum::customize::customize_t
magic_enum::customize::enum_name<serenkai::PlaybackMode>(
    serenkai::PlaybackMode value) noexcept {
    switch (value) {
    case serenkai::PlaybackMode::Forward:
        return "forward";
    case serenkai::PlaybackMode::Reverse:
        return "reverse";
    case serenkai::PlaybackMode::Pingpong:
        return "pingpong";
    case serenkai::PlaybackMode::PingpongReverse:
        return "pingpong_reverse";
    default:
        return magic_enum::customize::default_tag;
    }
}
#pragma once

#include "serenkai/resource/animation_data.hpp"
#include "serenkai/resource/resource_location.hpp"
namespace serenkai {
/// @brief State machine for animation playback
///
/// Internally plays and maintains animation playback.
class AnimationPlayer {
public:
    bool play(const SpriteAnimationData& data, const ResourceLocation& clip);

    void stop();

    void update(float dt);

    const SpriteFrame* frame() const;
    const ResourceLocation* image() const;
    bool playing() const;

private:
    const SpriteAnimationData* m_data = nullptr;
    const AnimationClip* m_clip = nullptr;
    int m_frame = 0;
    int m_step = 1;
    float m_elapsed = 0.0f;

    void advance();
};

} // namespace serenkai
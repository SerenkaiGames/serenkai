#pragma once

#include "serenkai/resource/animation_data.hpp"
#include "serenkai/resource/resource_location.hpp"
namespace serenkai {
/// @brief State machine for animation playback
///
/// Internally plays and maintains animation playback.
class AnimationPlayer {
public:
    bool play(const SpriteAnimationData& data, const ResourceLocation& clip,
              bool loop = false);

    void stop();

    void update(float dt);

    /// @brief Returns true when a non-looping clip has reached its end.
    bool finished() const;

    const SpriteFrame* frame() const;
    const ResourceLocation* image() const;
    const SpriteAnimationData* data() const;

    bool playing() const;

private:
    const SpriteAnimationData* m_data = nullptr;
    const AnimationClip* m_clip = nullptr;
    int m_frame = 0;
    int m_step = 1;
    float m_elapsed = 0.0f;
    bool m_loop = false;
    bool m_finished = false;
    int m_bounces = 0;

    void finish();
    void advance();
};

} // namespace serenkai
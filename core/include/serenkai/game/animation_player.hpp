#pragma once

#include "serenkai/resource/animation_data.hpp"
#include "serenkai/resource/resource_location.hpp"
namespace serenkai {

// @brief Animation playback status.
enum class PlayState : std::uint8_t {
    Stopped,   ///< Not playing any animation.
    Playing,   ///< Actively playing and advancing frames.
    Completed, ///< Finished a non-looping animation and holding final frame.
};

/// @brief State machine for animation playback
///
/// Internally plays and maintains animation playback.
class AnimationPlayer {
public:
    bool play(const SpriteAnimationData& data, const ResourceLocation& clip,
              bool loop);

    void stop();

    void update(float dt);

    bool playing() const;

    /// @brief Returns true when a non-looping clip has reached its end.
    bool finished() const;

    PlayState state() const;

    const SpriteFrame* frame() const;
    const ResourceLocation* image() const;
    const SpriteAnimationData* data() const;

private:
    const SpriteAnimationData* m_data = nullptr;
    const AnimationClip* m_clip = nullptr;

    PlayState m_state = PlayState::Stopped;
    int m_frame = 0;
    int m_step = 1;
    float m_elapsed = 0.0f;
    bool m_loop = false;
    int m_bounces = 0;

    void advance();
};

} // namespace serenkai
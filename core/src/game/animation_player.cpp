#include "serenkai/game/animation_player.hpp"

#include <spdlog/spdlog.h>

namespace {
/// @brief Maximum number of frames to advance in a single update.
///
/// Frames exported by Aseprite may have a duration of 0; in that case m_elapsed
/// will not be consumed, and without an upper limit this would cause an
/// infinite loop. It also guards against abnormally large dt.
constexpr int MAX_FRAME_STEPS = 1024;
} // namespace

namespace serenkai {
bool AnimationPlayer::play(const SpriteAnimationData& data,
                           const ResourceLocation& clip_loc, bool loop) {
    auto it = data.clips.find(clip_loc);

    if (it == data.clips.end()) {
        spdlog::warn("Clip {} not found", clip_loc.str());
        return false;
    }

    // If already playing the same clip and not completed, just update loop
    // flag.
    if (m_clip == &it->second && m_state == PlayState::Playing) {
        m_loop = loop;
        return true;
    }

    m_data = &data;
    m_clip = &it->second;
    m_elapsed = 0.0f;
    m_loop = loop;
    m_bounces = 0;
    m_state = PlayState::Playing;

    switch (m_clip->mode) {
    case PlaybackMode::Forward:
    case PlaybackMode::Pingpong:
        m_frame = m_clip->from;
        m_step = 1;
        break;
    case PlaybackMode::Reverse:
    case PlaybackMode::PingpongReverse:
        m_frame = m_clip->to;
        m_step = -1;
        break;
    }
    return true;
}

void AnimationPlayer::stop() {
    m_data = nullptr;
    m_clip = nullptr;
    m_state = PlayState::Stopped;
    m_frame = 0;
    m_step = 1;
    m_elapsed = 0.0f;
    m_loop = false;
    m_bounces = 0;
}

void AnimationPlayer::update(float dt) {

    if (m_state != PlayState::Playing || dt <= 0.0f || !m_data || !m_clip) {
        return;
    }

    m_elapsed += dt;

    for (int steps = 0; steps < MAX_FRAME_STEPS; ++steps) {
        if (m_frame < 0 || m_frame >= static_cast<int>(m_data->frames.size())) {
            break;
        }

        const float duration = m_data->frames[m_frame].duration;

        if (m_elapsed < duration) {
            break;
        }

        m_elapsed -= duration;

        advance();

        // If advance caused completion, stop processing immediately.
        if (m_state != PlayState::Playing) {
            m_elapsed = 0.0f;
            break;
        }
    }
}

void AnimationPlayer::advance() {
    const int from = m_clip->from;
    const int to = m_clip->to;

    if (from == to) {
        if (!m_loop) {
            m_state = PlayState::Completed;
        }
        return;
    }

    switch (m_clip->mode) {
    case PlaybackMode::Forward:
        if (m_frame >= to) {
            if (m_loop) {
                m_frame = from;
            } else {
                m_state = PlayState::Completed;
            }
        } else {
            ++m_frame;
        }
        break;
    case PlaybackMode::Reverse:
        if (m_frame <= from) {
            if (m_loop) {
                m_frame = to;
            } else {
                m_state = PlayState::Completed;
            }
        } else {
            --m_frame;
        }
        break;
    case PlaybackMode::Pingpong:
    case PlaybackMode::PingpongReverse:
        m_frame += m_step;
        if (m_frame > to) {
            // A one-shot ping-pong finishes after the return pass.
            if (!m_loop && ++m_bounces >= 2) {
                m_frame = to;
                m_state = PlayState::Completed;
            } else {
                m_frame = to - 1;
                m_step = -1;
            }
        } else if (m_frame < from) {
            if (!m_loop && ++m_bounces >= 2) {
                m_frame = from;
                m_state = PlayState::Completed;
            } else {
                m_frame = from + 1;
                m_step = 1;
            }
        }
        break;
    }
}

bool AnimationPlayer::finished() const {
    return m_state == PlayState::Completed;
}

bool AnimationPlayer::playing() const { return m_state == PlayState::Playing; }

PlayState AnimationPlayer::state() const { return m_state; }

const SpriteFrame* AnimationPlayer::frame() const {
    if (m_state == PlayState::Stopped) {
        return nullptr;
    }
    if (m_frame < 0 || m_frame >= static_cast<int>(m_data->frames.size())) {
        return nullptr;
    }
    return &m_data->frames[static_cast<size_t>(m_frame)];
}

const ResourceLocation* AnimationPlayer::image() const {
    if (!m_data) {
        return nullptr;
    }
    return &m_data->image;
}

const SpriteAnimationData* AnimationPlayer::data() const { return m_data; }

} // namespace serenkai
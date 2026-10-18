#pragma once

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
                           const ResourceLocation& clip_loc) {
    auto it = data.clips.find(clip_loc);

    if (it == data.clips.end()) {
        spdlog::warn("Clip {} not found", clip_loc.str());
        return false;
    }

    if (m_clip == &it->second) {
        return true;
    }

    m_data = &data;

    m_clip = &it->second;

    m_elapsed = 0.0f;

    switch (m_clip->mode) {
    case PlaybackMode::Forward:
    case PlaybackMode::Pingpong:
        m_frame = m_clip->from;
        m_step = 1;
        break;
    case PlaybackMode::Reverse:
    case PlaybackMode::PingpongReverse:
        m_frame = m_clip->to;
        m_step = 1;
        break;
    }
    return true;
}

void AnimationPlayer::stop() {
    m_data = nullptr;
    m_clip = nullptr;
    m_frame = 0;
    m_step = 1;
    m_elapsed = 0.0f;
}

bool AnimationPlayer::playing() const {
    return m_data != nullptr && m_clip != nullptr;
}

void AnimationPlayer::update(float dt) {
    if (!m_data || !m_clip || dt <= 0.0f) {
        return;
    }
    if (m_clip->from == m_clip->to) {
        return; // Single-frame animation.
    }

    m_elapsed += dt;

    for (int steps = 0; steps < MAX_FRAME_STEPS; ++steps) {
        const float duration = m_data->frames[m_frame].duration;

        if (m_elapsed < duration) {
            break;
        }

        m_elapsed -= duration;

        advance();
    }
}

void AnimationPlayer::advance() {
    const int from = m_clip->from;
    const int to = m_clip->to;

    switch (m_clip->mode) {
    case PlaybackMode::Forward:
        m_frame = (m_frame >= to) ? from : m_frame + 1;
        break;
    case PlaybackMode::Reverse:
        m_frame = (m_frame <= from) ? to : m_frame - 1;
        break;
    case PlaybackMode::Pingpong:
    case PlaybackMode::PingpongReverse:
        m_frame += m_step;
        if (m_frame > to) {
            m_frame = to - 1;
            m_step = -1;
        } else if (m_frame < from) {
            m_frame = from + 1;
            m_step = 1;
        }
        break;
    }
}

const SpriteFrame* AnimationPlayer::frame() const {
    if (!playing()) {
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

} // namespace serenkai
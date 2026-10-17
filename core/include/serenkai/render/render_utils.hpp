#pragma once
#include "serenkai/render/flip_flag.hpp"

#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <utility>

namespace serenkai {
struct FlipTransform {
    double angle = 0.0;
    SDL_FlipMode flip_mode = SDL_FLIP_NONE;
};

/// @brief Calculate the rotation angle and flip mode required for rendering
/// based on FlipFlag, and adjust the destination rectangle dst when performing
/// a diagonal flip.
inline FlipTransform resolve_flip_transform(FlipFlag flip,
                                            SDL_FRect& in_out_dst) noexcept {
    FlipTransform result{};
    if (flip == FlipFlag::None) {
        return result;
    }

    const bool h = (flip & FlipFlag::Horizontal) != FlipFlag::None;
    const bool v = (flip & FlipFlag::Vertical) != FlipFlag::None;
    const bool d = (flip & FlipFlag::Diagonal) != FlipFlag::None;

    if (!d) {
        if (h) {
            result.flip_mode = static_cast<SDL_FlipMode>(result.flip_mode |
                                                         SDL_FLIP_HORIZONTAL);
        }
        if (v) {
            result.flip_mode =
                static_cast<SDL_FlipMode>(result.flip_mode | SDL_FLIP_VERTICAL);
        }
    } else {
        // Diagonal flip swaps width and height while keeping the tile center
        // invariant.
        const float cx = in_out_dst.x + in_out_dst.w * 0.5f;
        const float cy = in_out_dst.y + in_out_dst.h * 0.5f;
        std::swap(in_out_dst.w, in_out_dst.h);
        in_out_dst.x = cx - in_out_dst.w * 0.5f;
        in_out_dst.y = cy - in_out_dst.h * 0.5f;

        if (!h && !v) {
            result.angle = 90.0;
            result.flip_mode = SDL_FLIP_VERTICAL;
        } else if (h && !v) {
            result.angle = 90.0;
            result.flip_mode = SDL_FLIP_NONE;
        } else if (!h && v) {
            result.angle = 270.0;
            result.flip_mode = SDL_FLIP_NONE;
        } else {
            result.angle = 90.0;
            result.flip_mode = SDL_FLIP_HORIZONTAL;
        }
    }

    return result;
}
} // namespace serenkai
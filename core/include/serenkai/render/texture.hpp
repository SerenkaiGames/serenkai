#pragma once

#include <SDL3/SDL_render.h>

namespace serenkai {

/// @brief A simple RAII wrapper for SDL_Texture.
class Texture {
public:
    Texture() = default;

    explicit Texture(SDL_Texture* tex) noexcept;

    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&& other) noexcept;

    Texture& operator=(Texture&& other) noexcept;

    void reset(SDL_Texture* tex = nullptr) noexcept;

    SDL_Texture* get() const noexcept;
    SDL_Texture* release() noexcept;
    explicit operator bool() const noexcept;

private:
    SDL_Texture* m_tex = nullptr;
};
} // namespace serenkai
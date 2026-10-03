#include "serenkai/render/texture.hpp"

#include <utility>

namespace serenkai {

Texture::Texture(SDL_Texture* tex) noexcept : m_tex(tex) {}

Texture::~Texture() { reset(); }

Texture::Texture(Texture&& other) noexcept
    : m_tex(std::exchange(other.m_tex, nullptr)) {}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        reset();
        m_tex = std::exchange(other.m_tex, nullptr);
    }
    return *this;
}

void Texture::reset(SDL_Texture* tex) noexcept {
    if (m_tex) {
        SDL_DestroyTexture(m_tex);
    }
    m_tex = tex;
}

SDL_Texture* Texture::get() const noexcept { return m_tex; }
SDL_Texture* Texture::release() noexcept {
    return std::exchange(m_tex, nullptr);
}
Texture::operator bool() const noexcept { return m_tex != nullptr; }
} // namespace serenkai
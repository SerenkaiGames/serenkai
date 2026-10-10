#pragma once

#include <glm/ext/vector_float2.hpp>
namespace serenkai {
class Camera {
public:
    Camera(const Camera&) = delete;
    Camera(Camera&&) = delete;
    Camera& operator=(const Camera&) = delete;
    Camera& operator=(Camera&&) = delete;

    Camera() = default;
    ~Camera() = default;

    glm::vec2 pos() const;
    float zoom() const;

private:
    glm::vec2 m_pos{0.0f};
    float m_zoom{1.0f};
};
} // namespace serenkai
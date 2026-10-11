#pragma once

#include <glm/ext/vector_float2.hpp>
namespace serenkai {

/// @brief 2D camera.
///
/// The camera position is the center of the viewport.
/// World space has its origin at the top-left of the map, with +x to the right
/// and +y downward.
class Camera {
public:
    static constexpr float MAX_ZOOM = 15.0f;

    Camera(const Camera&) = delete;
    Camera(Camera&&) = delete;
    Camera& operator=(const Camera&) = delete;
    Camera& operator=(Camera&&) = delete;

    Camera() = default;
    ~Camera() = default;

    glm::vec2 pos() const;
    float zoom() const;

    void set_pos(glm::vec2 pos);
    void set_zoom(float zoom);

    void add_zoom(float zoom);

    void move(glm::vec2 distance);

private:
    glm::vec2 m_pos{0.0f};
    float m_zoom{3.0f};
};
} // namespace serenkai
#include "serenkai/game/camera.hpp"

#include <algorithm>
#include <glm/ext/vector_float2.hpp>

namespace serenkai {
glm::vec2 Camera::pos() const { return m_pos; }
float Camera::zoom() const { return m_zoom; }

void Camera::set_pos(glm::vec2 pos) { m_pos = pos; }
void Camera::set_zoom(float zoom) {
    if (zoom < 1.0f || zoom > MAX_ZOOM) {
        return;
    }
    m_zoom = zoom;
}

void Camera::add_zoom(float zoom) {
    m_zoom = std::clamp(m_zoom + zoom, 1.0f, MAX_ZOOM);
}

void Camera::move(glm::vec2 distance) { m_pos += distance; }

} // namespace serenkai
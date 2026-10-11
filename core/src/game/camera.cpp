#include "serenkai/game/camera.hpp"

#include <glm/ext/vector_float2.hpp>

namespace serenkai {
glm::vec2 Camera::pos() const { return m_pos; }
float Camera::zoom() const { return m_zoom; }

void Camera::set_pos(glm::vec2 pos) { m_pos = pos; }
void Camera::set_zoom(float zoom) { m_zoom = zoom; }

void Camera::move(glm::vec2 distance) { m_pos += distance; }

} // namespace serenkai
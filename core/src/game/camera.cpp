#include "serenkai/game/camera.hpp"

#include <glm/ext/vector_float2.hpp>

namespace serenkai {
glm::vec2 Camera::pos() const { return m_pos; }
float Camera::zoom() const { return m_zoom; }
} // namespace serenkai
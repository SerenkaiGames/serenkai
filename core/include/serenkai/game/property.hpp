#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <string>
#include <variant>
namespace serenkai {
using PropValue = std::variant<std::monostate, bool, int, float, std::string,
                               glm::vec4, glm::vec2, glm::vec3>;
}
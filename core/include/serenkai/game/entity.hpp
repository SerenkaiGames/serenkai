#pragma once

#include "serenkai/game/property.hpp"
#include "serenkai/resource/resource_location.hpp"

#include <cstdint>
#include <unordered_map>
#include <variant>
namespace serenkai {

// generation is in the upper 32 bits
// idx is in the lower 32 bits
using EntityId = std::uint64_t;

/// @brief Entity class, providing only basic entity attributes
///
/// The specific operations on entities are determined externally.
struct Entity {

    Entity() = default;
    ~Entity() = default;

    Entity(const Entity&) = delete;
    Entity& operator=(const Entity&) = delete;
    Entity(Entity&&) = default;
    Entity& operator=(Entity&&) = default;

    std::uint32_t generation = 1;
    bool alive = true;
    // Use ResourceLocation as the key to normalize properties while ensuring
    // good hash lookup performance.
    std::unordered_map<ResourceLocation, PropValue> props;

    template <typename T> const T* get_prop(const ResourceLocation& loc) const {
        auto it = props.find(loc);
        if (it == props.end()) {
            return nullptr;
        }

        return std::get_if<T>(&it->second);
    }
};
} // namespace serenkai
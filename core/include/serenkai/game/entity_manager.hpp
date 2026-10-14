#pragma once

#include "serenkai/game/entity.hpp"

#include <cstdint>
#include <utility>
#include <vector>
namespace serenkai {

class EntityManager {
public:
    EntityId create();
    void destroy(EntityId id);
    bool alive(EntityId id) const;
    bool set_prop(EntityId id, const ResourceLocation& key, PropValue value);
    std::optional<PropValue> get_prop(EntityId id,
                                      const ResourceLocation& key) const;
    bool has_prop(EntityId id, const ResourceLocation& key) const;
    bool remove_prop(EntityId id, const ResourceLocation& key);

private:
    static EntityId pack(std::uint32_t index, std::uint32_t generation);
    static std::pair<std::uint32_t, std::uint32_t> unpack(EntityId id);

    /// @brief Get the entity; return nullptr if invalid.
    Entity* fetch(EntityId id);
    const Entity* fetch(EntityId id) const;

    std::vector<Entity> m_entities;
    std::vector<std::uint32_t> m_free_list;
};
} // namespace serenkai
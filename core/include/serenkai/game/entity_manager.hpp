#pragma once

#include "serenkai/game/animation_player.hpp"
#include "serenkai/game/entity.hpp"
#include "serenkai/resource/animation_manager.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
namespace serenkai {
class AssetManager;
class ScriptEngine;

class EntityManager {
public:
    EntityManager(AssetManager* asset_manager, ScriptEngine* engine);
    ~EntityManager();
    inline static constexpr EntityId NULL_ID = 0;

    EntityId create();
    EntityId create_with_loc(std::string_view loc);

    void destroy(EntityId id);
    bool alive(EntityId id) const;

    bool set_prop_by_loc(EntityId id, const ResourceLocation& key,
                         PropValue value);
    bool set_prop(EntityId id, std::string_view loc, PropValue value);

    std::optional<PropValue> get_prop_by_loc(EntityId id,
                                             const ResourceLocation& key) const;
    std::optional<PropValue> get_prop(EntityId id, std::string_view loc) const;

    bool has_prop_by_loc(EntityId id, const ResourceLocation& key) const;
    bool has_prop(EntityId id, std::string_view loc) const;

    bool remove_prop_by_loc(EntityId id, const ResourceLocation& key);
    bool remove_prop(EntityId id, std::string_view loc);

    bool play_animation(EntityId id, std::string_view clip_loc, bool loop);
    void stop_animation(EntityId id);
    const AnimationPlayer* animation_player(EntityId id) const;

    std::size_t alive_size() const;

    void update(float dt);

    /// @brief Perform an operation for each alive entity.
    ///
    /// Interface for batch processing, intended for engine use.
    /// @tparam Fn The type is void(EntityId, Entity&).
    /// @note If you need to obtain a pointer to the entity, ensure that the
    /// pointer is valid.
    template <typename Fn> void for_each_alive(Fn&& fn) {
        for (std::uint32_t i = 0; i < m_entities.size(); ++i) {
            if (m_entities[i].alive)
                fn(pack(i, m_entities[i].generation), m_entities[i]);
        }
    }
    template <typename Fn> void for_each_alive(Fn&& fn) const {
        for (std::uint32_t i = 0; i < m_entities.size(); ++i) {
            if (m_entities[i].alive)
                fn(pack(i, m_entities[i].generation), m_entities[i]);
        }
    }

private:
    AssetManager* m_asset_manager = nullptr;
    ScriptEngine* m_script_engine = nullptr;

    std::unique_ptr<AnimationManager> m_animation_mgr;
    std::unordered_map<EntityId, AnimationPlayer> m_animation_players;
    /// @brief Get the entity; return nullptr if invalid.
    ///
    /// @warning If the vector reallocates, the pointer will be invalidated.
    Entity* fetch(EntityId id);
    const Entity* fetch(EntityId id) const;

    std::vector<Entity> m_entities;
    std::vector<std::uint32_t> m_free_list;
    static EntityId pack(std::uint32_t index, std::uint32_t generation);
    static std::pair<std::uint32_t, std::uint32_t> unpack(EntityId id);
};
} // namespace serenkai
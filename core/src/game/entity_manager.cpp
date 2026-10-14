#include "serenkai/game/entity_manager.hpp"

#include "serenkai/resource/resource_location.hpp"

#include <cstdint>
#include <optional>
#include <spdlog/spdlog.h>

namespace serenkai {
EntityId EntityManager::create() {
    std::uint32_t idx{0};
    if (!m_free_list.empty()) {
        idx = m_free_list.back();
        m_free_list.pop_back();
    } else {
        idx = static_cast<std::uint32_t>(m_entities.size());
        m_entities.emplace_back();
    }

    auto& e = m_entities[idx];
    e.alive = true;
    return pack(idx, e.generation);
}

void EntityManager::destroy(EntityId id) {

    auto e = fetch(id);
    if (!e) {
        return;
    }

    auto [idx, _] = unpack(id);

    e->props.clear();
    e->alive = false;
    e->generation++;
    m_free_list.emplace_back(idx);
}

bool EntityManager::alive(EntityId id) const { return fetch(id) != nullptr; }
bool EntityManager::set_prop(EntityId id, const ResourceLocation& key,
                             PropValue value) {
    auto e = fetch(id);
    if (!e) {
        return false;
    }

    e->props[key] = std::move(value);
    return true;
}
bool EntityManager::set_prop(EntityId id, std::string_view loc,
                             PropValue value) {
    auto res = ResourceLocation::parse(loc);
    if (!res) {
        spdlog::error("Invalid loc {}", loc);
        return false;
    }

    return set_prop(id, *res, std::move(value));
}

std::optional<PropValue>
EntityManager::get_prop(EntityId id, const ResourceLocation& key) const {
    auto e = fetch(id);
    if (!e) {
        return std::nullopt;
    }
    auto it = e->props.find(key);
    if (it == e->props.end()) {
        return std::nullopt;
    }
    return it->second;
}
std::optional<PropValue> EntityManager::get_prop(EntityId id,
                                                 std::string_view loc) const {
    auto res = ResourceLocation::parse(loc);
    if (!res) {
        spdlog::error("Invalid loc {}", loc);
        return std::nullopt;
    }
    return get_prop(id, *res);
}

bool EntityManager::has_prop(EntityId id, const ResourceLocation& key) const {
    auto e = fetch(id);
    if (!e) {
        return false;
    }

    auto it = e->props.find(key);
    return it != e->props.end();
}
bool EntityManager::has_prop(EntityId id, std::string_view loc) const {
    auto res = ResourceLocation::parse(loc);
    if (!res) {
        spdlog::error("Invalid loc {}", loc);
        return false;
    }
    return has_prop(id, *res);
}

bool EntityManager::remove_prop(EntityId id, const ResourceLocation& key) {
    if (auto e = fetch(id)) {
        return e->props.erase(key);
    }
    return false;
}
bool EntityManager::remove_prop(EntityId id, std::string_view loc) {
    auto res = ResourceLocation::parse(loc);
    if (!res) {
        spdlog::error("Invalid loc {}", loc);
        return false;
    }
    return remove_prop(id, *res);
}

Entity* EntityManager::fetch(EntityId id) {
    auto [idx, gen] = unpack(id);
    if (idx >= m_entities.size()) {
        return nullptr;
    }
    auto& e = m_entities[idx];

    if (!e.alive || (e.generation != gen)) {
        return nullptr;
    }

    return &e;
}

const Entity* EntityManager::fetch(EntityId id) const {
    return const_cast<EntityManager*>(this)->fetch(id);
}

EntityId EntityManager::pack(std::uint32_t index, std::uint32_t generation) {
    return (static_cast<std::uint64_t>(generation) << 32) |
           static_cast<std::uint64_t>(index);
}
std::pair<std::uint32_t, std::uint32_t> EntityManager::unpack(EntityId id) {
    return {static_cast<std::uint32_t>(id & 0xffffffffULL),
            static_cast<std::uint32_t>(id >> 32)};
}

} // namespace serenkai
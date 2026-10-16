#include "serenkai/game/entity_manager.hpp"

#include "serenkai/base/glm_parse.hpp"
#include "serenkai/game/property.hpp"
#include "serenkai/resource/asset_manager.hpp"
#include "serenkai/resource/resource_location.hpp"
#include "serenkai/script/script.hpp"
#include "serenkai/script/script_engine.hpp"

#include <cstddef>
#include <cstdint>
#include <glaze/core/reflect.hpp>
#include <glaze/json/read.hpp>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace serenkai {

struct EntityData {
    std::unordered_map<std::string, PropValue> props;
    std::vector<std::string> scripts;
};

EntityManager::EntityManager(AssetManager* asset_manager, ScriptEngine* engine)
    : m_asset_manager(asset_manager), m_script_engine(engine) {}

EntityManager::~EntityManager() {}

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

EntityId EntityManager::create_with_loc(std::string_view loc) {
    if (!m_asset_manager) {
        return NULL_ID;
    }

    auto path = m_asset_manager->get(loc);

    if (!path) {
        spdlog::error("Failed to get assets path {}", loc);
        return NULL_ID;
    }
    EntityData data{};

    auto ec = glz::read_file_json(data, *path, std::string{});

    if (ec) {
        spdlog::error("Failed to parse json {}, {}", loc,
                      glz::format_error(ec));
        return NULL_ID;
    }

    auto id = create();

    auto e = fetch(id);

    if (!e) {
        return NULL_ID;
    }

    for (auto& [prop_loc, value] : data.props) {
        auto key = ResourceLocation::parse(prop_loc);
        if (!key) {
            spdlog::error("Invalid resource location {}", prop_loc);
            continue;
        }
        e->props.try_emplace(*key, value);
    }

    if (!m_script_engine) {
        return id;
    }

    std::vector<Script> scripts;
    scripts.reserve(data.scripts.size());

    for (auto& script_loc : data.scripts) {
        auto s = m_script_engine->create(script_loc);
        if (!s) {
            spdlog::error("Failed to load script {}", script_loc);
            continue;
        }
        scripts.emplace_back(std::move(*s));
    }

    e = fetch(id);
    if (!e) {
        return NULL_ID;
    }
    e->scripts = std::move(scripts);

    for (std::size_t i = 0;; ++i) {
        // Update the pointer to prevent a dangling pointer caused by
        // reallocation during init.
        auto* cur_e = fetch(id);

        if (!cur_e || !cur_e->alive || i >= cur_e->scripts.size()) {
            break;
        }
        cur_e->scripts[i].call<>("on_init", static_cast<double>(id));
    }

    return id;
}

void EntityManager::destroy(EntityId id) {

    auto e = fetch(id);
    if (!e) {
        return;
    }

    auto [idx, _] = unpack(id);

    for (auto& s : e->scripts) {
        s.try_call<>("on_destroy");
    }

    e->scripts.clear();
    e->props.clear();
    e->alive = false;
    e->generation++;
    m_free_list.emplace_back(idx);
}

bool EntityManager::alive(EntityId id) const { return fetch(id) != nullptr; }
bool EntityManager::set_prop_by_loc(EntityId id, const ResourceLocation& key,
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

    return set_prop_by_loc(id, *res, std::move(value));
}

std::optional<PropValue>
EntityManager::get_prop_by_loc(EntityId id, const ResourceLocation& key) const {
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
    return get_prop_by_loc(id, *res);
}

bool EntityManager::has_prop_by_loc(EntityId id,
                                    const ResourceLocation& key) const {
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
    return has_prop_by_loc(id, *res);
}

bool EntityManager::remove_prop_by_loc(EntityId id,
                                       const ResourceLocation& key) {
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
    return remove_prop_by_loc(id, *res);
}

std::size_t EntityManager::alive_size() const {
    std::size_t count = 0;
    for (auto& e : m_entities) {
        if (e.alive) {
            ++count;
        }
    }
    return count;
}

Entity* EntityManager::fetch(EntityId id) {
    return const_cast<Entity*>(std::as_const(*this).fetch(id));
}

const Entity* EntityManager::fetch(EntityId id) const {
    if (id == NULL_ID) {
        return nullptr;
    }

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

EntityId EntityManager::pack(std::uint32_t index, std::uint32_t generation) {
    return (static_cast<std::uint64_t>(generation) << 32) |
           static_cast<std::uint64_t>(index);
}
std::pair<std::uint32_t, std::uint32_t> EntityManager::unpack(EntityId id) {
    return {static_cast<std::uint32_t>(id & 0xffffffffULL),
            static_cast<std::uint32_t>(id >> 32)};
}

} // namespace serenkai
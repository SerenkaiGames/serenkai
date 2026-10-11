#include "serenkai/application/input_manager.hpp"

#include "serenkai/application/key.hpp"
#include "serenkai/base/assert.hpp"
#include "serenkai/base/enum_meta.hpp"

#include <glaze/json/generic.hpp>
#include <map>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

namespace serenkai {

namespace detail {
struct Config {
    std::map<std::string, std::vector<Key>> bindings;
};
} // namespace detail

bool InputManager::merge_config(std::string_view json_content) {
    detail::Config cfg{};
    auto ec = glz::read_json(cfg, json_content);
    if (ec) {
        spdlog::error("Failed to read key config json {}", json_content);
        return false;
    }

    for (const auto& [k, v] : cfg.bindings) {
        ActionId id = 0;
        auto it = m_action_to_id.find(k);

        if (it != m_action_to_id.end()) {

            id = it->second;

        } else {

            if (m_next_id >= MAX_ACTIONS) {
                spdlog::error(
                    "Exceeded MAX_ACTIONS limit when registering '{}'", k);
                return false;
            }

            id = m_next_id++;
            m_action_to_id.try_emplace(k, id);
            m_id_to_string.emplace_back(k);
        }

        for (Key key : v) {
            bind_action(key, id);
        }
    }

    return true;
}

void InputManager::bind_action(Key key, ActionId id) {
    auto& actions = m_key_to_actions[key];

    if (std::find(actions.begin(), actions.end(), id) == actions.end()) {
        actions.push_back(id);
    }
}

void InputManager::process_key_event(const KeyEvent& e) {

    // Ignore OS-level key repeat (Repeat); game logic is computed continuously
    // each frame based on the held state.
    if (e.action == KeyAction::Repeat) {
        return;
    }

    auto it = m_key_to_actions.find(e.key);
    if (it == m_key_to_actions.end()) {
        return;
    }

    const auto& action_ids = it->second;

    if (e.action == KeyAction::Press) {
        for (ActionId id : action_ids) {
            auto prev_count = m_action_press_counts[id]++;

            if (prev_count == 0) {
                m_held_bits.set(id, true);
                m_pressed_bits.set(id, true);
            }
        }
    } else if (e.action == KeyAction::Release) {
        for (ActionId id : action_ids) {
            if (m_action_press_counts[id] > 0) {
                --m_action_press_counts[id];

                if (m_action_press_counts[id] == 0) {
                    m_held_bits.set(id, false);
                    m_released_bits.set(id, true);
                }
            }
        }
    }
}

void InputManager::tick() {
    m_pressed_bits.reset();
    m_released_bits.reset();
}

bool InputManager::is_down(ActionId id) const noexcept {
    if (id >= MAX_ACTIONS) [[unlikely]] {
        SE_ASSERT(false);
        return false;
    }
    return m_held_bits[id];
}
bool InputManager::is_pressed(ActionId id) const noexcept {
    if (id >= MAX_ACTIONS) [[unlikely]] {
        SE_ASSERT(false);
        return false;
    }
    return m_pressed_bits[id];
}
bool InputManager::is_released(ActionId id) const noexcept {
    if (id >= MAX_ACTIONS) [[unlikely]] {
        SE_ASSERT(false);
        return false;
    }
    return m_released_bits[id];
}

std::vector<InputManager::ActionId>
InputManager::get_held_actions() const noexcept {
    std::vector<ActionId> held;

    for (ActionId id = 0; id < m_next_id; ++id) {
        if (m_held_bits[id]) {
            held.push_back(id);
        }
    }
    return held;
}

const std::unordered_map<std::string, InputManager::ActionId, StringHash,
                         StringEqual>&
InputManager::get_action_id_map() const {
    return m_action_to_id;
}
const std::string& InputManager::get_action_name(ActionId id) const {
    static std::string u = "Unknown";
    if (id >= MAX_ACTIONS) [[unlikely]] {
        SE_ASSERT(false);
        return u;
    }
    return m_id_to_string[id];
}
} // namespace serenkai
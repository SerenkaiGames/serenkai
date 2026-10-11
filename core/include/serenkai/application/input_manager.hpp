#pragma once

#include "serenkai/application/event.hpp"
#include "serenkai/application/key.hpp"
#include "serenkai/base/string_hash.hpp"

#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
namespace serenkai {

/// @brief Manages the bindings between input and Luau
///
/// Handles input events and provides interfaces for Luau to call.
class InputManager {
public:
    static constexpr std::size_t MAX_ACTIONS = 256;
    using ActionBits = std::bitset<MAX_ACTIONS>;
    using ActionId = std::uint16_t;

    bool merge_config(std::string_view json_content);

    void process_key_event(const KeyEvent& e);

    /// @brief Reset the pressed and released states.
    void tick();

    bool is_down(ActionId id) const noexcept;
    /// @note Only handles the frame in which it is triggered.
    bool is_pressed(ActionId id) const noexcept;
    /// @note Only handles the frame in which it is triggered.
    bool is_released(ActionId id) const noexcept;

    std::vector<ActionId> get_held_actions() const noexcept;

    const std::unordered_map<std::string, ActionId, StringHash, StringEqual>&
    get_action_id_map() const;

    const std::string& get_action_name(ActionId id) const;

private:
    ActionBits m_held_bits;     // All Actions currently in the held state.
    ActionBits m_pressed_bits;  // Actions that just became pressed this frame.
    ActionBits m_released_bits; // Actions that just became released this frame.

    ActionId m_next_id = 0;

    std::array<std::uint8_t, MAX_ACTIONS> m_action_press_counts{};

    std::unordered_map<std::string, ActionId, StringHash, StringEqual>
        m_action_to_id;

    std::vector<std::string> m_id_to_string;

    std::unordered_map<Key, std::vector<ActionId>> m_key_to_actions;

    void bind_action(Key key, ActionId id);
};
} // namespace serenkai
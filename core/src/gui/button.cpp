#include "serenkai/gui/button.hpp"

#include "serenkai/base/assert.hpp"
#include "serenkai/gui/gui_context.hpp"
#include "serenkai/gui/widget.hpp"

#include <algorithm>
#include <glm/ext/vector_int2.hpp>
#include <spdlog/spdlog.h>

namespace serenkai {
Button::Button(std::string_view name, Widget* parent) : Widget(name, parent) {}

void Button::set_enable(bool enable) { m_enable = enable; }

bool Button::is_hovered() const { return m_hovered; }
bool Button::is_enable() const { return m_enable; }

bool Button::handle_mouse_move_event(const MouseMoveEvent& e) {
    // Process its own children first, i.e., the top-level ones.
    if (Widget::handle_mouse_move_event(e)) {
        return true;
    }

    if (m_enable) {
        // Convert from the logical coordinate system to the physical coordinate
        // system for mouse position hit-testing.
        const auto logical_pos = pos();
        const auto p = e.gui_context->to_physical_coord(logical_pos);

        const auto s = size();

        const auto w = s.x * e.gui_context->ui_scale();

        const auto h = s.y * e.gui_context->ui_scale();

        if (e.xpos >= p.x && e.xpos <= p.x + w && e.ypos >= p.y &&
            e.ypos <= p.y + h) {
            m_hovered = true;

            return true;
        }
    }
    m_hovered = false;
    return false;
}
bool Button::handle_key_event(const KeyEvent& e) {
    if (Widget::handle_key_event(e)) {
        return true;
    }

    if (e.action == KeyAction::Press && e.key == Key::MouseLeft) {
        if (m_hovered && m_clicked && m_enable) {
            m_clicked();
            return true;
        }
    }

    return false;
}

void Button::on_update(float) { measure_from_children(); }

void Button::set_size(glm::ivec2) {
    spdlog::error("Button::set_size is deleted");
    SE_ASSERT(false);
}

void Button::measure_from_children() {
    auto children_span = children();

    auto max_size = std::ranges::fold_left(
        children_span, glm::ivec2{0}, [](glm::ivec2 acc, const auto& c) {
            glm::ivec2 c_szie = c->size();
            return glm::ivec2{std::max(c_szie.x, acc.x),
                              std::max(c_szie.y, acc.y)};
        });

    Widget::set_size(max_size);
}

} // namespace serenkai
#include "serenkai/gui/button.hpp"

#include "serenkai/gui/gui_context.hpp"
#include "serenkai/gui/widget.hpp"

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

} // namespace serenkai
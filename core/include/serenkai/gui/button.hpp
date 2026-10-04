#pragma once

#include "serenkai/application/event.hpp"
#include "serenkai/gui/widget.hpp"

#include <functional>
#include <glm/ext/vector_int2.hpp>
#include <string_view>
namespace serenkai {
/// @brief Button class
///
/// Provides click detection and function invocation after a click.
/// Automatically calculates its size from its children.
class Button : public Widget {
public:
    Button(std::string_view name, Widget* parent);

    void set_enable(bool enable);

    bool is_hovered() const;
    bool is_enable() const;

    bool handle_mouse_move_event(const MouseMoveEvent& e) override;
    bool handle_key_event(const KeyEvent& e) override;

    template <typename F> void set_clicked(F&& f) {
        m_clicked = std::forward<F>(f);
        return;
    }

private:
    void set_size(glm::ivec2 size) override;

    bool m_hovered = false;
    bool m_enable = true;
    std::function<void()> m_clicked;
};
} // namespace serenkai
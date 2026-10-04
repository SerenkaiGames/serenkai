#include "serenkai/gui/rect.hpp"

#include "serenkai/gui/gui_context.hpp"
#include "serenkai/gui/widget.hpp"

#include <spdlog/spdlog.h>

namespace serenkai {
Rect::Rect(std::string_view name, Widget* parent) : Widget(name, parent) {}

void Rect::set_color(Color color) { m_color = color; }
void Rect::set_alpha(float alpha) { m_alpha = alpha; }
void Rect::set_fill_parent(bool fill) { m_fill_parent = fill; }

void Rect::set_size(glm::ivec2 size) {
    if (m_fill_parent) {
        spdlog::warn(
            "Rect {} fill parent is true, set_size will not take effect ",
            name());
    }
    Widget::set_size(size);
}

Color Rect::color() const { return m_color; }
float Rect::alpha() const { return m_alpha; }

void Rect::on_render(GuiContext* context) { context->render_rect(*this); }
void Rect::on_update(float) {
    if (m_fill_parent) {
        Widget::set_size(Widget::has_parent() ? Widget::parent()->size()
                                              : Widget::logical_window_size());
    }
}

} // namespace serenkai
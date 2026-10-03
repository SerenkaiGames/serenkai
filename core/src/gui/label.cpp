#include "serenkai/gui/label.hpp"

#include "serenkai/gui/gui_context.hpp"
#include "serenkai/gui/widget.hpp"
#include "serenkai/render/renderer.hpp"
#include "serenkai/resource/font.hpp"

#include <spdlog/spdlog.h>
namespace serenkai {
Lable::Lable(Widget* parent) : Widget(parent) {}

void Lable::set_text(std::string text) {
    m_text = text;
    measure_size();
}
void Lable::set_color(Color color) { m_color = color; }
void Lable::set_font(Font* font) {
    if (!font) {
        spdlog::warn("Lable {} font is nullptr", m_text);
    }
    m_font = font;
    measure_size();
}

std::string Lable::text() const { return m_text; }
Color Lable::color() const { return m_color; }

void Lable::measure_size() {
    if (!m_font) {
        return;
    }
    set_size(glm::ivec2{m_font->measure_width(m_text), m_font->line_height()});
}

void Lable::on_render(GuiContext& context) {
    if (!m_font) {
        return;
    }
    context.get_renderer()->render_lable(*this, context);
}

} // namespace serenkai
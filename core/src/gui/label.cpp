#include "serenkai/gui/label.hpp"

#include "serenkai/base/assert.hpp"
#include "serenkai/gui/gui_context.hpp"
#include "serenkai/gui/widget.hpp"
#include "serenkai/render/renderer.hpp"
#include "serenkai/resource/font.hpp"

#include <spdlog/spdlog.h>
#include <string>
namespace serenkai {
Label::Label(std::string_view name, Widget* parent) : Widget(name, parent) {}

void Label::set_text(std::string text) {
    m_text = text;
    measure_size();
}
void Label::set_color(Color color) { m_color = color; }
void Label::set_font(Font* font) {
    if (!font) {
        spdlog::warn("Label {} font is nullptr", m_text);
    }
    m_font = font;
    measure_size();
}

std::string Label::text() const { return m_text; }
Color Label::color() const { return m_color; }
Font* Label::font() const { return m_font; }

void Label::set_size(glm::ivec2) {
    spdlog::error("Label::set_size is deleted");
    SE_ASSERT(false);
}

void Label::measure_size() {
    if (!m_font) {
        return;
    }
    Widget::set_size(
        glm::ivec2{m_font->measure_width(m_text), m_font->line_height()});
}

void Label::on_render(GuiContext* context) {
    if (!m_font) {
        return;
    }
    if (!context) {
        return;
    }
    context->render_label(*this);
}

} // namespace serenkai
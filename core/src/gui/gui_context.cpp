#include "serenkai/gui/gui_context.hpp"

#include "serenkai/base/assert.hpp"
#include "serenkai/base/glm_fmt.hpp"
#include "serenkai/gui/widget.hpp"
#include "serenkai/render/renderer.hpp"
#include "serenkai/resource/font_manager.hpp"
#include "serenkai/resource/texture_manager.hpp"

#include <algorithm>
#include <spdlog/spdlog.h>

namespace serenkai {
GuiContext::GuiContext(GuiConfig config)
    : m_renderer(config.renderer), m_texture_manager(config.texture_manager),
      m_widget_parser(config.widget_parser),
      m_font_manager(config.font_manager) {}

Renderer* GuiContext::get_renderer() const {
    SE_ASSERT(m_renderer);
    return m_renderer;
}

WidgetParser* GuiContext::get_widget_parser() const {
    SE_ASSERT(m_widget_parser);
    return m_widget_parser;
}

size_t GuiContext::ui_scale() const { return m_ui_scale; }

bool GuiContext::handle_window_resize_event(const WindowResizeEvent& e) {
    const int w = e.width;
    const int h = e.height;

    if (w <= 0 || h <= 0) {
        return false;
    }

    int new_scale = (h + 270) / 540;
    if (w >= 1920 && h >= 1080) {
        new_scale = std::max(new_scale, 3);
    }
    new_scale = std::clamp(new_scale, 2, 8);

    glm::ivec2 new_logical_size{w / static_cast<int>(new_scale),
                                h / static_cast<int>(new_scale)};

    if (static_cast<size_t>(new_scale) == m_ui_scale &&
        new_logical_size == m_logical_window_size) {
        return false;
    }

    m_ui_scale = static_cast<size_t>(new_scale);

    m_logical_window_size = new_logical_size;

    Widget::set_logical_window_size(m_logical_window_size);

    spdlog::debug("New ui scale {}, window size {}, logical window size {}",
                  m_ui_scale, glm::ivec2{w, h}, m_logical_window_size);

    // Let other functions that need to update the window size continue
    // processing without consuming the event.
    return false;
}

void GuiContext::render_label(const Label& label) {

    if (!label.font()) {
        return;
    }

    auto pos = label.pos();
    Font* font_to_render = label.font();
    float scale = static_cast<float>(ui_scale());

    // If a FontManager is available, get the font for the physical screen
    // resolution, and set the draw scale to 1.0f. No stretching
    if (m_font_manager) {
        if (auto* scaled_font =
                m_font_manager->get_scaled(*label.font(), ui_scale())) {
            font_to_render = scaled_font;
            scale = 1.0f;
        }
    }

    m_renderer->draw_text(*font_to_render, label.text(), to_physical_coord(pos),
                          label.color(), scale);
}

void GuiContext::render_rect(const Rect& rect) {
    auto pos = rect.pos();
    m_renderer->draw_rect(to_physical_coord(pos), rect.size(), rect.color(),
                          rect.alpha(), ui_scale());
}

void GuiContext::render_image(const ImageWidget& image) {
    if (!m_texture_manager) {
        return;
    }

    auto texture = m_texture_manager->get(image.get_image());

    m_renderer->draw_image(texture, to_physical_coord(image.pos()),
                           image.size(), ui_scale());
}

glm::ivec2 GuiContext::to_physical_coord(glm::ivec2 pos) const {
    return {pos.x * ui_scale(), pos.y * ui_scale()};
}

glm::vec2 GuiContext::to_logical_coord(glm::vec2 physical_pos) const {
    return physical_pos / static_cast<float>(m_ui_scale);
}

} // namespace serenkai
#include "serenkai/gui/gui_context.hpp"

#include "serenkai/base/assert.hpp"
#include "serenkai/base/glm_fmt.hpp"
#include "serenkai/gui/widget.hpp"
#include "serenkai/render/renderer.hpp"

#include <algorithm>
#include <spdlog/spdlog.h>

namespace serenkai {
GuiContext::GuiContext(GuiConfig config) : m_renderer(config.renderer) {}

Renderer* GuiContext::get_renderer() const {
    SE_VERIFY(m_renderer);
    return m_renderer;
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

    auto pos = label.pos();
    m_renderer->draw_text(*label.font(), label.text(), to_physical_coord(pos),
                          label.color(), ui_scale());
}

void GuiContext::render_rect(const Rect& rect) {
    auto pos = rect.pos();
    m_renderer->draw_rect(to_physical_coord(pos), rect.size(), rect.color(),
                          rect.alpha(), ui_scale());
}

glm::ivec2 GuiContext::to_physical_coord(glm::ivec2 pos) {
    return {pos.x * ui_scale(), pos.y * ui_scale()};
}

} // namespace serenkai
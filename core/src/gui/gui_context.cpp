#include "serenkai/gui/gui_context.hpp"

#include "serenkai/base/assert.hpp"
#include "serenkai/render/renderer.hpp"

namespace serenkai {
GuiContext::GuiContext(Renderer* renderer) : m_renderer(renderer) {}

Renderer* GuiContext::get_renderer() const {
    SE_VERIFY(m_renderer);
    return m_renderer;
}

} // namespace serenkai
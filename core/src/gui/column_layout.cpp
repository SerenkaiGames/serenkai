#include "serenkai/gui/column_layout.hpp"

#include "serenkai/base/assert.hpp"
#include "serenkai/gui/widget.hpp"

#include <glm/ext/vector_int2.hpp>
#include <spdlog/spdlog.h>

namespace serenkai {
ColumnLayout::ColumnLayout(std::string_view name, Widget* parent)
    : Widget(name, parent) {}

void ColumnLayout::set_spacing(int spacing) { m_spacing = spacing; }
void ColumnLayout::set_child_anchor(ChildAnchor anchor) {
    m_child_anchor = anchor;
}

int ColumnLayout::spacing() { return m_spacing; }

void ColumnLayout::layout() {
    auto children = Widget::children();
    int y = 0;
    Anchor anchor = Anchor::TopLeft;
    switch (m_child_anchor) {
    case ChildAnchor::Left:
        anchor = Anchor::TopLeft;
        break;
    case ChildAnchor::Center:
        anchor = Anchor::TopCenter;
        break;
    case ChildAnchor::Right:
        anchor = Anchor::TopRight;
        break;
    }
    glm::ivec2 self_size{0};
    for (auto& child : children) {
        child->set_anchor(anchor);
        child->set_offset({0, y});
        auto size = child->size();
        y += size.y + m_spacing;
        self_size.x = std::max(self_size.x, size.x);
    }
    self_size.y = children.empty() ? 0 : (y - m_spacing);
}

void ColumnLayout::on_update(float) { layout(); }

void ColumnLayout::set_size(glm::ivec2) {
    spdlog::error("ColumnLayout::set_size is deleted");
    SE_ASSERT(false);
}

} // namespace serenkai
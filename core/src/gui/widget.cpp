#include "serenkai/gui/widget.hpp"

#include <glm/ext/vector_int2.hpp>
#include <spdlog/spdlog.h>
#include <utility>

namespace serenkai {
Widget::Widget(std::string name, Widget* parent)
    : m_name(std::move(name)), m_parent(parent) {}

void Widget::set_logical_window_size(glm::ivec2 size) {
    m_logical_window_size = size;
}

glm::ivec2 Widget::logical_window_size() { return m_logical_window_size; }

void Widget::update(float dt) {
    on_update(dt);

    for (auto& child : m_children) {
        child->update(dt);
    }
}

void Widget::render(GuiContext* context) {
    if (!m_visible) {
        return;
    }
    on_render(context);

    for (auto& child : m_children) {
        child->render(context);
    }
}

glm::ivec2 Widget::compute_position() const {
    glm::ivec2 pos{0, 0};
    glm::ivec2 parent_size{0, 0};

    if (!m_parent) {
        if (m_logical_window_size.x == 0 || m_logical_window_size.y == 0) {
            spdlog::error("Logical window size is 0");
        }
        parent_size = m_logical_window_size;
    } else {
        parent_size = m_parent->size();
    }

    const glm::ivec2 self_size = size();
    const int parent_w = parent_size.x;
    const int parent_h = parent_size.y;
    const int w = self_size.x;
    const int h = self_size.y;

    switch (m_anchor) {
    case Anchor::TopLeft:
        pos = {0, 0};
        break;
    case Anchor::TopCenter:
        pos = {(parent_w - w) / 2, 0};
        break;
    case Anchor::TopRight:
        pos = {parent_w - w, 0};
        break;
    case Anchor::CenterLeft:
        pos = {0, (parent_h - h) / 2};
        break;
    case Anchor::Center:
        pos = {(parent_w - w) / 2, (parent_h - h) / 2};
        break;
    case Anchor::CenterRight:
        pos = {parent_w - w, (parent_h - h) / 2};
        break;
    case Anchor::BottomLeft:
        pos = {0, parent_h - h};
        break;
    case Anchor::BottomCenter:
        pos = {(parent_w - w) / 2, parent_h - h};
        break;
    case Anchor::BottomRight:
        pos = {parent_w - w, parent_h - h};
        break;
    }
    if (m_parent) {
        pos += m_parent->pos();
    }
    pos += m_offset;
    return pos;
}

void Widget::on_update(float) {}
void Widget::on_render(GuiContext*) {}

void Widget::set_anchor(Anchor anchor) { m_anchor = anchor; }
void Widget::set_offset(glm::ivec2 offset) { m_offset = offset; }
void Widget::set_size(glm::ivec2 size) { m_size = size; }
void Widget::set_visible(bool visible) { m_visible = visible; }

void Widget::add_child(std::unique_ptr<Widget> child) {
    if (!child) {
        return;
    }
    child->m_parent = this;
    m_children.emplace_back(std::move(child));
}

glm::ivec2 Widget::size() const { return m_size; }
glm::ivec2 Widget::offset() const { return m_offset; }
glm::ivec2 Widget::pos() const { return compute_position(); }
Anchor Widget::anchor() const { return m_anchor; }

bool Widget::is_visible() const { return m_visible; }

Widget* Widget::parent() const { return m_parent; }

const std::string& Widget::name() const { return m_name; }

std::span<const std::unique_ptr<Widget>> Widget::children() const {
    return m_children;
}

} // namespace serenkai
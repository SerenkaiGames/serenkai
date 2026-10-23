#include "serenkai/gui/ui_view.hpp"

#include "serenkai/application/event.hpp"

#include <utility>
#include <variant>

namespace serenkai {
UiView::UiView(const ResourceLocation& loc, std::unique_ptr<Widget> widget,
               std::unique_ptr<Script> script)
    : m_loc(loc), m_widget(std::move(widget)), m_script(std::move(script)) {}

void UiView::update(float dt) {
    if (m_widget) {
        m_widget->update(dt);
    }
}

void UiView::render(GuiContext* ctx) {
    if (m_widget) {
        m_widget->render(ctx);
    }
}

bool UiView::handle_event(const Event& e) {
    if (!m_widget) {
        return false;
    }

    return std::visit(
        Overloaded{
            [this](const KeyEvent& e) { return m_widget->handle_key_event(e); },
            [this](const MouseMoveEvent& e) {
                return m_widget->handle_mouse_move_event(e);
            },
            [](const auto&) { return false; }},
        e);
}

const ResourceLocation& UiView::loc() const { return m_loc; }
Widget* UiView::widget() const { return m_widget.get(); }
Script* UiView::script() const { return m_script.get(); }

} // namespace serenkai

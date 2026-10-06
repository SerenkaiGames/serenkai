#include "serenkai/scenes/title_scene.hpp"

#include "serenkai/gui/gui_context.hpp"
#include "serenkai/gui/widget_parser.hpp"
namespace serenkai {

void TitleScene::update(float dt) {
    if (m_root_widget) {
        m_root_widget->update(dt);
    }
}

void TitleScene::render(GuiContext* context) {
    if (m_root_widget) {
        m_root_widget->render(context);
    }
}

void TitleScene::on_enter(GuiContext* context) {
    m_root_widget =
        context->get_widget_parser()->parse("serenkai:ui/main_title.json");
}

} // namespace serenkai
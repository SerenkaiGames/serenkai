#include "serenkai/scenes/title_scene.hpp"

#include "serenkai/application/app_context.hpp"
#include "serenkai/gui/gui_context.hpp"
#include "serenkai/gui/widget_parser.hpp"
#include "serenkai/render/render_context.hpp"

#include <stdexcept>
namespace serenkai {

void TitleScene::update(float dt) {
    if (m_ui_view) {
        m_ui_view->update(dt);
    }
}

void TitleScene::render(RenderContext* ctx) {
    if (!ctx) {
        return;
    }
    if (m_ui_view) {
        m_ui_view->render(ctx->gui);
    }
}

void TitleScene::on_enter(AppContext* ctx) {
    if (!ctx) {
        throw std::runtime_error(
            "Failed to enter title scene: Application context is null");
    }
    if (ctx->gui) {
        m_ui_view =
            ctx->gui->get_widget_parser()->parse("serenkai:ui/main_title.json");
    }
}

bool TitleScene::handle_event(const Event& e) {
    if (m_ui_view) {
        return m_ui_view->handle_event(e);
    }
    return false;
}

} // namespace serenkai
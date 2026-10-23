#pragma once

#include "serenkai/application/event.hpp"
#include "serenkai/gui/gui_context.hpp"
#include "serenkai/gui/widget.hpp"
#include "serenkai/resource/resource_location.hpp"
#include "serenkai/script/script.hpp"

#include <memory>
namespace serenkai {

/// @brief View management class for UI
///
/// Includes UI components and script lifecycle.
class UiView {
public:
    UiView(const ResourceLocation& loc, std::unique_ptr<Widget> widget,
           std::unique_ptr<Script> script = nullptr);

    void update(float dt);

    void render(GuiContext* ctx);

    bool handle_event(const Event& e);

    const ResourceLocation& loc() const;
    Widget* widget() const;
    Script* script() const;

private:
    ResourceLocation m_loc;
    std::unique_ptr<Widget> m_widget;
    std::unique_ptr<Script> m_script;
};
} // namespace serenkai

#pragma once
#include "serenkai/gui/anchor.hpp"
#include "serenkai/gui/widget.hpp"

#include <glm/ext/vector_int2.hpp>
#include <string_view>
namespace serenkai {
/// @brief Layout class
///
/// Does not render itself; used only for layout.
class ColumnLayout : public Widget {
public:
    ColumnLayout(std::string_view name, Widget* parent);

    void set_spacing(int spacing);
    void set_child_anchor(ChildAnchor anchor);

    int spacing() const;
    ChildAnchor child_anchor() const;

    void layout();

private:
    int m_spacing = 0;
    ChildAnchor m_child_anchor = ChildAnchor::Left;

    void on_update(float dt) override;
    void set_size(glm::ivec2 size) override;
};
} // namespace serenkai
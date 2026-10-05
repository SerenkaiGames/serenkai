#pragma once

namespace serenkai {

enum class Anchor {
    TopLeft,
    TopCenter,
    TopRight,

    CenterLeft,
    Center,
    CenterRight,

    BottomLeft,
    BottomCenter,
    BottomRight,

};
// Unified anchor for layout widgets to control child components.
enum class ChildAnchor { Left, Center, Right };

} // namespace serenkai

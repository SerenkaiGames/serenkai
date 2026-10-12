#pragma once

#include <algorithm>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace serenkai {

/// @brief AABB bounding box
///
/// @note Assumes y points downward.
struct AABB {
    glm::vec2 position{0.0f}; // Top-left corner vertex.
    glm::vec2 size{
        0.0f}; // Width (x) and height (y), both should be non-negative.

    AABB() = default;
    AABB(const glm::vec2& pos, const glm::vec2& s) : position(pos), size(s) {
        normalize();
    }

    AABB(float x, float y, float w, float h) : position(x, y), size(w, h) {
        normalize();
    }

    explicit AABB(const glm::vec4& vec4)
        : position(vec4.x, vec4.y), size(vec4.z, vec4.w) {
        normalize();
    }

    static AABB from_center(const glm::vec2& center, const glm::vec2& size) {
        return AABB(center - size * 0.5f, size);
    }

    static AABB from_min_max(const glm::vec2& min_p, const glm::vec2& max_p) {
        return AABB(min_p, max_p - min_p);
    }

    float left() const { return position.x; }
    float top() const { return position.y; }
    float right() const { return position.x + size.x; }
    float bottom() const { return position.y + size.y; }

    float width() const { return size.x; }
    float height() const { return size.y; }

    glm::vec2 top_left() const { return position; }
    glm::vec2 top_right() const { return {right(), top()}; }
    glm::vec2 bottom_left() const { return {left(), bottom()}; }
    glm::vec2 bottom_right() const { return {right(), bottom()}; }
    glm::vec2 center() const { return position + size * 0.5f; }

    glm::vec2 min() const { return position; }
    glm::vec2 max() const { return position + size; }

    // Ensure size is non-negative (if negative, flip position and take the
    // absolute value).
    void normalize() {
        if (size.x < 0.0f) {
            position.x += size.x;
            size.x = -size.x;
        }
        if (size.y < 0.0f) {
            position.y += size.y;
            size.y = -size.y;
        }
    }

    void translate(const glm::vec2& offset) { position += offset; }
    void scale(const glm::vec2& factor) { size *= factor; }
    void set_center(const glm::vec2& c) { position = c - size * 0.5f; }

    bool contains(const glm::vec2& p) const {
        return p.x >= left() && p.x <= right() && p.y >= top() &&
               p.y <= bottom();
    }

    bool contains(const AABB& other) const {
        return other.left() >= left() && other.right() <= right() &&
               other.top() >= top() && other.bottom() <= bottom();
    }

    bool intersects(const AABB& other) const {
        return !(other.left() > right() || other.right() < left() ||
                 other.top() > bottom() || other.bottom() < top());
    }

    // Intersection area; returns a degenerate (zero-size) AABB when there is no
    // intersection.
    AABB intersection(const AABB& other) const {
        glm::vec2 mn(std::max(left(), other.left()),
                     std::max(top(), other.top()));
        glm::vec2 mx(std::min(right(), other.right()),
                     std::min(bottom(), other.bottom()));

        if (mx.x <= mn.x || mx.y <= mn.y)
            return AABB(glm::vec2(0.0f), glm::vec2(0.0f));

        return AABB::from_min_max(mn, mx);
    }

    // The smallest bounding box that contains both AABBs.
    AABB united(const AABB& other) const {
        glm::vec2 mn(std::min(left(), other.left()),
                     std::min(top(), other.top()));
        glm::vec2 mx(std::max(right(), other.right()),
                     std::max(bottom(), other.bottom()));
        return AABB::from_min_max(mn, mx);
    }

    // Expand outward by amount in all directions.
    AABB expanded(float amount) const {
        return AABB(position - glm::vec2(amount),
                    size + glm::vec2(amount * 2.0f));
    }

    AABB translated(const glm::vec2& offset) const {
        return AABB(position + offset, size);
    }

    float area() const { return size.x * size.y; }
};
} // namespace serenkai
#include "serenkai/script/bindings/bind_aabb.hpp"

#include "serenkai/base/aabb.hpp"
#include "serenkai/script/bindings/luau_glm.hpp"
#include "serenkai/script/lua_header.hpp"

namespace serenkai {
void register_lua_aabb(lua_State* L) {
    luabridge::getGlobalNamespace(L)
        .beginClass<AABB>("AABB")
        .addConstructor<void (*)(), void (*)(float, float, float, float),
                        void (*)(const glm::vec2&, const glm::vec2&)>()
        .addStaticFunction(
            "new", []() { return AABB(); },
            [](float x, float y, float w, float h) { return AABB(x, y, w, h); },
            [](const glm::vec2& pos, const glm::vec2& size) {
                return AABB(pos, size);
            })

        .addStaticFunction("from_center", &AABB::from_center)
        .addStaticFunction("from_min_max", &AABB::from_min_max)

        .addFunction("left", &AABB::left)
        .addFunction("top", &AABB::top)
        .addFunction("right", &AABB::right)
        .addFunction("bottom", &AABB::bottom)
        .addFunction("width", &AABB::width)
        .addFunction("height", &AABB::height)

        .addFunction("top_left", &AABB::top_left)
        .addFunction("top_right", &AABB::top_right)
        .addFunction("bottom_left", &AABB::bottom_left)
        .addFunction("bottom_right", &AABB::bottom_right)
        .addFunction("center", &AABB::center)

        .addFunction("min", &AABB::min)
        .addFunction("max", &AABB::max)

        .addFunction("normalize", &AABB::normalize)
        .addFunction("translate", &AABB::translate)
        .addFunction("scale", &AABB::scale)
        .addFunction("set_center", &AABB::set_center)

        .addFunction("contains",
                     luabridge::overload<const glm::vec2&>(&AABB::contains),
                     luabridge::overload<const AABB&>(&AABB::contains))

        .addFunction("intersects", &AABB::intersects)
        .addFunction("intersection", &AABB::intersection)
        .addFunction("united", &AABB::united)
        .addFunction("expanded", &AABB::expanded)
        .addFunction("translated", &AABB::translated)
        .addFunction("area", &AABB::area)
        .addFunction("is_empty", &AABB::is_empty)
        .endClass();
}
} // namespace serenkai
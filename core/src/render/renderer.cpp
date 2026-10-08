#include "serenkai/render/renderer.hpp"

#include "serenkai/gui/color.hpp"
#include "serenkai/render/text_renderer.hpp"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_vulkan.h>
#include <fmt/format.h>
#include <glm/ext/vector_int2.hpp>
#include <memory>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string_view>

#ifdef _WIN32
#include <d3d11.h>
#include <dxgi.h>
#endif

#ifdef __APPLE__
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#endif

namespace {
// NOLINTNEXTLINE
using PFN_vkVoidFunction = void (*)(void);
// NOLINTNEXTLINE
using PFN_vkGetInstanceProcAddr = PFN_vkVoidFunction (*)(VkInstance,
                                                         const char*);
// NOLINTNEXTLINE
using PFN_vkGetPhysicalDeviceProperties = void (*)(VkPhysicalDevice, void*);

struct MinimalVkProperties {
    uint32_t api_version;
    uint32_t driver_version;
    uint32_t vendor_id;
    uint32_t device_id;
    uint32_t device_type;
    char device_name[256];
    char padding[1024];
};
} // namespace

namespace serenkai {

std::optional<std::string>
Renderer::query_gpu_name(std::string_view name) const {

    SDL_PropertiesID renderer_props = SDL_GetRendererProperties(m_sdl_renderer);

    // SDL3 "gpu" backend
    if (name == "gpu" && renderer_props) {
        auto* gpu_device = static_cast<SDL_GPUDevice*>(SDL_GetPointerProperty(
            renderer_props, SDL_PROP_RENDERER_GPU_DEVICE_POINTER, nullptr));
        if (gpu_device) {
            SDL_PropertiesID gpu_props = SDL_GetGPUDeviceProperties(gpu_device);
            if (const char* gpu_name = SDL_GetStringProperty(
                    gpu_props, SDL_PROP_GPU_DEVICE_NAME_STRING, nullptr)) {
                return std::string(gpu_name);
            }
        }
    }

    // OpenGL / OpenGL ES backend
    if (name == "opengl" || name == "opengles2") {
        auto gl_get_string = reinterpret_cast<const char* (*)(unsigned int)>(
            SDL_GL_GetProcAddress("glGetString"));
        if (gl_get_string) {
            constexpr unsigned int GL_RENDERER = 0x1F01; // NOLINT
            if (const char* gpu_name = gl_get_string(GL_RENDERER)) {
                return std::string(gpu_name);
            }
        }
    }

    // Vulkan backend
    if (name == "vulkan" && renderer_props) {
        auto* instance = static_cast<VkInstance>(SDL_GetPointerProperty(
            renderer_props, SDL_PROP_RENDERER_VULKAN_INSTANCE_POINTER,
            nullptr));
        auto* phys_dev = static_cast<VkPhysicalDevice>(SDL_GetPointerProperty(
            renderer_props, SDL_PROP_RENDERER_VULKAN_PHYSICAL_DEVICE_POINTER,
            nullptr));
        if (instance && phys_dev) {
            auto get_proc_addr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
                SDL_Vulkan_GetVkGetInstanceProcAddr());
            if (get_proc_addr) {
                auto vk_get_props =
                    reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(
                        get_proc_addr(instance,
                                      "vkGetPhysicalDeviceProperties"));
                if (vk_get_props) {
                    MinimalVkProperties props{};
                    vk_get_props(phys_dev, &props);
                    return std::string(props.device_name);
                }
            }
        }
    }

#ifdef _WIN32
    // Direct3D 11 backend (Windows)
    if (name == "direct3d11" && renderer_props) {
        auto* d3d11_dev = static_cast<ID3D11Device*>(SDL_GetPointerProperty(
            renderer_props, SDL_PROP_RENDERER_D3D11_DEVICE_POINTER, nullptr));
        if (d3d11_dev) {
            IDXGIDevice* dxgi_dev = nullptr;
            if (SUCCEEDED(d3d11_dev->QueryInterface(
                    __uuidof(IDXGIDevice),
                    reinterpret_cast<void**>(&dxgi_dev)))) {
                IDXGIAdapter* adapter = nullptr;
                if (SUCCEEDED(dxgi_dev->GetAdapter(&adapter))) {
                    DXGI_ADAPTER_DESC desc{};
                    if (SUCCEEDED(adapter->GetDesc(&desc))) {
                        char name_buf[128];
                        wcstombs(name_buf, desc.Description, sizeof(name_buf));
                        adapter->Release();
                        dxgi_dev->Release();
                        return std::string(name_buf);
                    }
                    adapter->Release();
                }
                dxgi_dev->Release();
            }
        }
    }
#endif

#ifdef __APPLE__
    // Metal backend (macOS)
    if (name == "metal") {
        if (auto* layer = static_cast<CAMetalLayer*>(
                SDL_GetRenderMetalLayer(m_sdl_renderer))) {
            if (layer.device && layer.device.name) {
                return std::string([layer.device.name UTF8String]);
            }
        }
    }
#endif

    return std::nullopt;
}

Renderer::Renderer(const RendererConfig& config) : m_config(config) {
    m_sdl_renderer = SDL_CreateRenderer(config.window, nullptr);
    if (!m_sdl_renderer) {
        throw std::runtime_error(fmt::format(
            "Failed to initialize SDL_Renderer, error: {}", SDL_GetError()));
    }

    print_renderer_info();

    SDL_SetRenderVSync(m_sdl_renderer, static_cast<int>(config.v_sync));
    m_text_renderer = std::make_unique<TextRenderer>(m_sdl_renderer);
}
Renderer::~Renderer() {
    if (m_sdl_renderer) {
        SDL_DestroyRenderer(m_sdl_renderer);
    }
}

void Renderer::print_renderer_info() const {
    const char* renderer_name = SDL_GetRendererName(m_sdl_renderer);
    spdlog::info("Render backend: {}",
                 renderer_name ? renderer_name : "unknown");

    if (renderer_name) {
        if (auto gpu_name = query_gpu_name(renderer_name)) {
            spdlog::info("GPU: {}", *gpu_name);
        }
    }
}

SDL_Renderer* Renderer::get_sdl_renderer() const { return m_sdl_renderer; }

void Renderer::present() { SDL_RenderPresent(m_sdl_renderer); }

void Renderer::clear() {
    SDL_SetRenderDrawColor(m_sdl_renderer, m_clear_color.r, m_clear_color.g,
                           m_clear_color.b, m_clear_color.a);

    SDL_RenderClear(m_sdl_renderer);
}

void Renderer::draw_text(Font& font, std::string_view utf8, glm::ivec2 pos,
                         Color color, float scale) {
    m_text_renderer->draw_text(font, utf8, pos.x, pos.y, to_sdl_fcolor(color),
                               scale);
}

void Renderer::draw_rect(glm::ivec2 pos, glm::ivec2 size, Color color,
                         float alpha, float scale) {
    SDL_FRect dst{static_cast<float>(pos.x), static_cast<float>(pos.y),
                  static_cast<float>(size.x) * scale,
                  static_cast<float>(size.y) * scale};

    auto sc = to_sdl_fcolor(color);
    sc.a = alpha;
    SDL_SetRenderDrawColorFloat(m_sdl_renderer, sc.r, sc.g, sc.b, sc.a);
    SDL_SetRenderDrawBlendMode(m_sdl_renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderFillRect(m_sdl_renderer, &dst);
}

void Renderer::draw_image(SDL_Texture* texture, glm::ivec2 pos, glm::ivec2 size,
                          float scale) {
    if (!texture) {
        return;
    }
    SDL_FRect dst{static_cast<float>(pos.x), static_cast<float>(pos.y),
                  static_cast<float>(size.x) * scale,
                  static_cast<float>(size.y) * scale};
    SDL_RenderTexture(m_sdl_renderer, texture, nullptr, &dst);
}

} // namespace serenkai

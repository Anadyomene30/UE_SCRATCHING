// scratchvj — what OpenXR this machine actually offers, and whether bgfx and the
// runtime can be introduced to each other.
//
// This one is not a shader-to-reference check like its neighbours. It exists
// because every other question about VR depends on facts that cannot be guessed
// from documentation: which runtime is registered, which graphics binding it
// exposes, which renderer bgfx picked, and whether a headset is attached right
// now. Guessing any of those produces code that compiles and then fails on the
// only machine that matters.
//
// It is deliberately tolerant of having NO HEADSET. A missing headset is a fact
// to report, not a failure: the loader, the runtime and the binding can all be
// confirmed with the Quest in its box, and only the last step needs it awake.
// Exit code 0 means "the VR path is sound as far as the hardware present allows"
// and 1 means something is genuinely wrong.
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <bgfx/bgfx.h>

#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11
#define XR_USE_GRAPHICS_API_D3D12
#include <d3d11.h>
#include <d3d12.h>
#include <unknwn.h>

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

const char* renderer_name(bgfx::RendererType::Enum type) {
    return bgfx::getRendererName(type);
}

// The binding extension each bgfx backend needs. Named here rather than assumed
// because picking the wrong one fails at xrCreateSession, long after the point
// where the mistake was made.
//
// Only the two D3D bindings are COMPILED IN -- their headers are what
// XR_USE_GRAPHICS_API_D3D11/12 above pull in. The Vulkan and GL names are
// spelled out as literals so a machine whose bgfx picked one of those gets told
// what is missing instead of a blank refusal; wiring them is a separate job.
const char* binding_extension(bgfx::RendererType::Enum type) {
    switch (type) {
        case bgfx::RendererType::Direct3D11: return XR_KHR_D3D11_ENABLE_EXTENSION_NAME;
        case bgfx::RendererType::Direct3D12: return XR_KHR_D3D12_ENABLE_EXTENSION_NAME;
        case bgfx::RendererType::Vulkan: return "XR_KHR_vulkan_enable2";
        case bgfx::RendererType::OpenGL: return "XR_KHR_opengl_enable";
        default: return nullptr;
    }
}

}  // namespace

int main() {
    // bgfx first, so the renderer it CHOSE is reported rather than the one the
    // documentation says it would choose.
    SDL_SetMainReady();
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL indisponible\n");
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("xr_check", 128, 128, SDL_WINDOW_HIDDEN);
    if (window == nullptr) return 1;

    bgfx::renderFrame();
    bgfx::Init init;
    init.swapChain.nwh = SDL_GetPointerProperty(
        SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    init.swapChain.width = 128;
    init.swapChain.height = 128;
    if (!bgfx::init(init)) {
        std::fprintf(stderr, "bgfx indisponible\n");
        return 1;
    }
    const bgfx::RendererType::Enum renderer = bgfx::getRendererType();
    std::printf("bgfx          : %s\n", renderer_name(renderer));

    const char* wanted = binding_extension(renderer);
    if (wanted == nullptr) {
        std::fprintf(stderr, "aucun liant OpenXR pour le backend %s\n",
                     renderer_name(renderer));
        bgfx::shutdown();
        return 1;
    }
    std::printf("liant requis  : %s\n", wanted);

    // --- what the loader can see -------------------------------------------
    std::uint32_t extension_count = 0;
    if (XR_FAILED(xrEnumerateInstanceExtensionProperties(nullptr, 0, &extension_count,
                                                         nullptr))) {
        std::fprintf(stderr,
                     "le loader OpenXR ne repond pas : aucun runtime enregistre ?\n");
        bgfx::shutdown();
        return 1;
    }
    std::vector<XrExtensionProperties> extensions(
        extension_count, XrExtensionProperties{XR_TYPE_EXTENSION_PROPERTIES, nullptr, "", 0});
    xrEnumerateInstanceExtensionProperties(nullptr, extension_count, &extension_count,
                                           extensions.data());

    bool has_binding = false;
    for (const XrExtensionProperties& extension : extensions) {
        if (std::strcmp(extension.extensionName, wanted) == 0) has_binding = true;
    }
    std::printf("extensions    : %u exposees, liant %s\n", extension_count,
                has_binding ? "present" : "ABSENT");
    if (!has_binding) {
        std::fprintf(stderr,
                     "le runtime n'expose pas %s : bgfx et OpenXR ne peuvent pas "
                     "partager de device\n",
                     wanted);
        bgfx::shutdown();
        return 1;
    }

    // --- the instance -------------------------------------------------------
    const char* enabled[] = {wanted};
    XrInstanceCreateInfo instance_info{XR_TYPE_INSTANCE_CREATE_INFO};
    instance_info.enabledExtensionCount = 1;
    instance_info.enabledExtensionNames = enabled;
    std::strncpy(instance_info.applicationInfo.applicationName, "scratchvj",
                 XR_MAX_APPLICATION_NAME_SIZE - 1);
    instance_info.applicationInfo.applicationVersion = 1;
    instance_info.applicationInfo.apiVersion = XR_API_VERSION_1_0;

    XrInstance instance = XR_NULL_HANDLE;
    const XrResult created = xrCreateInstance(&instance_info, &instance);
    if (XR_FAILED(created)) {
        std::fprintf(stderr, "xrCreateInstance a echoue (%d)\n", static_cast<int>(created));
        bgfx::shutdown();
        return 1;
    }

    XrInstanceProperties properties{XR_TYPE_INSTANCE_PROPERTIES};
    xrGetInstanceProperties(instance, &properties);
    std::printf("runtime       : %s %u.%u.%u\n", properties.runtimeName,
                static_cast<unsigned>(XR_VERSION_MAJOR(properties.runtimeVersion)),
                static_cast<unsigned>(XR_VERSION_MINOR(properties.runtimeVersion)),
                static_cast<unsigned>(XR_VERSION_PATCH(properties.runtimeVersion)));

    // --- the headset, if one is awake ---------------------------------------
    XrSystemGetInfo system_info{XR_TYPE_SYSTEM_GET_INFO};
    system_info.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    XrSystemId system = XR_NULL_SYSTEM_ID;
    const XrResult got = xrGetSystem(instance, &system_info, &system);

    int status = 0;
    if (got == XR_ERROR_FORM_FACTOR_UNAVAILABLE) {
        // The expected answer with the Quest unplugged or asleep. Everything
        // above it is confirmed; only the display is missing.
        std::printf("casque        : aucun (runtime pret, casque non connecte)\n");
    } else if (XR_FAILED(got)) {
        std::fprintf(stderr, "xrGetSystem a echoue (%d)\n", static_cast<int>(got));
        status = 1;
    } else {
        XrSystemProperties system_properties{XR_TYPE_SYSTEM_PROPERTIES};
        xrGetSystemProperties(instance, system, &system_properties);
        std::printf("casque        : %s\n", system_properties.systemName);

        std::uint32_t view_count = 0;
        xrEnumerateViewConfigurationViews(instance, system,
                                          XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0,
                                          &view_count, nullptr);
        std::vector<XrViewConfigurationView> views(
            view_count, XrViewConfigurationView{XR_TYPE_VIEW_CONFIGURATION_VIEW});
        xrEnumerateViewConfigurationViews(instance, system,
                                          XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                                          view_count, &view_count, views.data());
        for (std::uint32_t i = 0; i < view_count; ++i) {
            std::printf("  oeil %u      : %ux%u conseille, %u echantillons\n", i,
                        views[i].recommendedImageRectWidth,
                        views[i].recommendedImageRectHeight,
                        views[i].recommendedSwapchainSampleCount);
        }
        if (view_count != 2) {
            std::fprintf(stderr, "configuration stereo attendue, %u vues recues\n",
                         view_count);
            status = 1;
        }
    }

    xrDestroyInstance(instance);
    bgfx::shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();

    if (status == 0) std::printf("chemin OpenXR praticable\n");
    return status;
}

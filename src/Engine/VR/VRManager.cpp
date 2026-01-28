#include "VRManager.h"
#include <glad/gl.h>
#include <SDL3/SDL.h>
#include <array>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <glm/gtc/matrix_transform.hpp>

#include "Engine/Graphics/Camera.h"
#include "Library/Logger/Logger.h"

static std::string_view xrResultName(XrResult result) {
    switch (result) {
        case XR_SUCCESS: return "XR_SUCCESS";
        case XR_TIMEOUT_EXPIRED: return "XR_TIMEOUT_EXPIRED";
        case XR_SESSION_LOSS_PENDING: return "XR_SESSION_LOSS_PENDING";
        case XR_EVENT_UNAVAILABLE: return "XR_EVENT_UNAVAILABLE";
        case XR_SPACE_BOUNDS_UNAVAILABLE: return "XR_SPACE_BOUNDS_UNAVAILABLE";
        case XR_SESSION_NOT_FOCUSED: return "XR_SESSION_NOT_FOCUSED";
        case XR_FRAME_DISCARDED: return "XR_FRAME_DISCARDED";
        case XR_ERROR_VALIDATION_FAILURE: return "XR_ERROR_VALIDATION_FAILURE";
        case XR_ERROR_RUNTIME_FAILURE: return "XR_ERROR_RUNTIME_FAILURE";
        case XR_ERROR_OUT_OF_MEMORY: return "XR_ERROR_OUT_OF_MEMORY";
        case XR_ERROR_API_VERSION_UNSUPPORTED: return "XR_ERROR_API_VERSION_UNSUPPORTED";
        case XR_ERROR_INITIALIZATION_FAILED: return "XR_ERROR_INITIALIZATION_FAILED";
        case XR_ERROR_FUNCTION_UNSUPPORTED: return "XR_ERROR_FUNCTION_UNSUPPORTED";
        case XR_ERROR_FEATURE_UNSUPPORTED: return "XR_ERROR_FEATURE_UNSUPPORTED";
        case XR_ERROR_EXTENSION_NOT_PRESENT: return "XR_ERROR_EXTENSION_NOT_PRESENT";
        case XR_ERROR_LIMIT_REACHED: return "XR_ERROR_LIMIT_REACHED";
        case XR_ERROR_SIZE_INSUFFICIENT: return "XR_ERROR_SIZE_INSUFFICIENT";
        case XR_ERROR_HANDLE_INVALID: return "XR_ERROR_HANDLE_INVALID";
        case XR_ERROR_INSTANCE_LOST: return "XR_ERROR_INSTANCE_LOST";
        case XR_ERROR_SESSION_RUNNING: return "XR_ERROR_SESSION_RUNNING";
        case XR_ERROR_SESSION_NOT_RUNNING: return "XR_ERROR_SESSION_NOT_RUNNING";
        case XR_ERROR_SESSION_LOST: return "XR_ERROR_SESSION_LOST";
        case XR_ERROR_SYSTEM_INVALID: return "XR_ERROR_SYSTEM_INVALID";
        case XR_ERROR_PATH_INVALID: return "XR_ERROR_PATH_INVALID";
        case XR_ERROR_PATH_COUNT_EXCEEDED: return "XR_ERROR_PATH_COUNT_EXCEEDED";
        case XR_ERROR_PATH_FORMAT_INVALID: return "XR_ERROR_PATH_FORMAT_INVALID";
        case XR_ERROR_PATH_UNSUPPORTED: return "XR_ERROR_PATH_UNSUPPORTED";
        case XR_ERROR_LAYER_INVALID: return "XR_ERROR_LAYER_INVALID";
        case XR_ERROR_LAYER_LIMIT_EXCEEDED: return "XR_ERROR_LAYER_LIMIT_EXCEEDED";
        case XR_ERROR_SWAPCHAIN_RECT_INVALID: return "XR_ERROR_SWAPCHAIN_RECT_INVALID";
        case XR_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED: return "XR_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED";
        case XR_ERROR_ACTION_TYPE_MISMATCH: return "XR_ERROR_ACTION_TYPE_MISMATCH";
        case XR_ERROR_SESSION_NOT_READY: return "XR_ERROR_SESSION_NOT_READY";
        case XR_ERROR_SESSION_NOT_STOPPING: return "XR_ERROR_SESSION_NOT_STOPPING";
        case XR_ERROR_TIME_INVALID: return "XR_ERROR_TIME_INVALID";
        case XR_ERROR_REFERENCE_SPACE_UNSUPPORTED: return "XR_ERROR_REFERENCE_SPACE_UNSUPPORTED";
        case XR_ERROR_FILE_ACCESS_ERROR: return "XR_ERROR_FILE_ACCESS_ERROR";
        case XR_ERROR_FILE_CONTENTS_INVALID: return "XR_ERROR_FILE_CONTENTS_INVALID";
        case XR_ERROR_FORM_FACTOR_UNSUPPORTED: return "XR_ERROR_FORM_FACTOR_UNSUPPORTED";
        case XR_ERROR_FORM_FACTOR_UNAVAILABLE: return "XR_ERROR_FORM_FACTOR_UNAVAILABLE";
        case XR_ERROR_API_LAYER_NOT_PRESENT: return "XR_ERROR_API_LAYER_NOT_PRESENT";
        case XR_ERROR_CALL_ORDER_INVALID: return "XR_ERROR_CALL_ORDER_INVALID";
        case XR_ERROR_GRAPHICS_DEVICE_INVALID: return "XR_ERROR_GRAPHICS_DEVICE_INVALID";
        case XR_ERROR_POSE_INVALID: return "XR_ERROR_POSE_INVALID";
        case XR_ERROR_INDEX_OUT_OF_RANGE: return "XR_ERROR_INDEX_OUT_OF_RANGE";
        case XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED: return "XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED";
        case XR_ERROR_ENVIRONMENT_BLEND_MODE_UNSUPPORTED: return "XR_ERROR_ENVIRONMENT_BLEND_MODE_UNSUPPORTED";
        case XR_ERROR_NAME_DUPLICATED: return "XR_ERROR_NAME_DUPLICATED";
        case XR_ERROR_NAME_INVALID: return "XR_ERROR_NAME_INVALID";
        case XR_ERROR_ACTIONSET_NOT_ATTACHED: return "XR_ERROR_ACTIONSET_NOT_ATTACHED";
        case XR_ERROR_ACTIONSETS_ALREADY_ATTACHED: return "XR_ERROR_ACTIONSETS_ALREADY_ATTACHED";
        case XR_ERROR_LOCALIZED_NAME_DUPLICATED: return "XR_ERROR_LOCALIZED_NAME_DUPLICATED";
        case XR_ERROR_LOCALIZED_NAME_INVALID: return "XR_ERROR_LOCALIZED_NAME_INVALID";
        case XR_ERROR_GRAPHICS_REQUIREMENTS_CALL_MISSING: return "XR_ERROR_GRAPHICS_REQUIREMENTS_CALL_MISSING";
        case XR_ERROR_RUNTIME_UNAVAILABLE: return "XR_ERROR_RUNTIME_UNAVAILABLE";
        default: return "XR_UNKNOWN_RESULT";
    }
}

static bool xrCheck(XrInstance instance, XrResult result, std::string_view call) {
    if (XR_SUCCEEDED(result))
        return true;

    if (logger) {
        if (instance != XR_NULL_HANDLE) {
            char resultString[XR_MAX_RESULT_STRING_SIZE] = {};
            if (XR_SUCCEEDED(xrResultToString(instance, result, resultString))) {
                logger->error("OpenXR {} failed: {} [{}] ({})", call, resultString, xrResultName(result), static_cast<int>(result));
            } else {
                logger->error("OpenXR {} failed: {} [{}]", call, static_cast<int>(result), xrResultName(result));
            }
        } else {
            logger->error("OpenXR {} failed: {} [{}]", call, static_cast<int>(result), xrResultName(result));
        }
    }

    return false;
}

static std::string getActiveRuntimePathWin32() {
#ifdef _WIN32
    HKEY hkey = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Khronos\\OpenXR\\1", 0, KEY_READ, &hkey) != ERROR_SUCCESS)
        return {};

    char buffer[4096] = {};
    DWORD bufferSize = sizeof(buffer);
    DWORD type = 0;
    LONG rc = RegGetValueA(hkey, nullptr, "ActiveRuntime", RRF_RT_REG_SZ, &type, buffer, &bufferSize);
    RegCloseKey(hkey);
    if (rc != ERROR_SUCCESS)
        return {};

    return std::string(buffer);
#else
    return {};
#endif
}

VRManager& VRManager::Get() {
    static VRManager instance;
    return instance;
}

VRManager::VRManager() {}
VRManager::~VRManager() { Shutdown(); }

bool VRManager::Initialize() {
    if (m_instance != XR_NULL_HANDLE) return true;

    std::string activeRuntimePath = getActiveRuntimePathWin32();
    if (logger && !activeRuntimePath.empty())
        logger->info("OpenXR ActiveRuntime: {}", activeRuntimePath);

    uint32_t extensionCount = 0;
    if (!xrCheck(XR_NULL_HANDLE, xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr), "xrEnumerateInstanceExtensionProperties"))
        return false;

    std::vector<XrExtensionProperties> extensions(extensionCount, {XR_TYPE_EXTENSION_PROPERTIES});
    if (!xrCheck(XR_NULL_HANDLE, xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, extensions.data()), "xrEnumerateInstanceExtensionProperties"))
        return false;

    bool hasOpenGLExtension = std::any_of(extensions.begin(), extensions.end(), [] (const XrExtensionProperties &p) {
        return std::string_view(p.extensionName) == "XR_KHR_opengl_enable";
    });

    if (!hasOpenGLExtension) {
        logger->error("OpenXR runtime doesn't support XR_KHR_opengl_enable");
        return false;
    }

    const char *enabledExtensions[] = { "XR_KHR_opengl_enable" };

    // 2. Create Instance
    XrInstanceCreateInfo createInfo = {XR_TYPE_INSTANCE_CREATE_INFO};
    strcpy(createInfo.applicationInfo.applicationName, "OpenEnroth VR");
    createInfo.applicationInfo.applicationVersion = 1;
    strcpy(createInfo.applicationInfo.engineName, "OpenEnroth");
    createInfo.applicationInfo.engineVersion = 1;
    createInfo.applicationInfo.apiVersion = XR_API_VERSION_1_0;
    createInfo.enabledExtensionCount = 1;
    createInfo.enabledExtensionNames = enabledExtensions;

    if (!xrCheck(XR_NULL_HANDLE, xrCreateInstance(&createInfo, &m_instance), "xrCreateInstance"))
        return false;

    // 3. Get System
    XrSystemGetInfo systemInfo = {XR_TYPE_SYSTEM_GET_INFO};
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    
    if (!xrCheck(m_instance, xrGetSystem(m_instance, &systemInfo, &m_systemId), "xrGetSystem"))
        return false;

    // 4. Requirements
    PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = nullptr;
    if (!xrCheck(m_instance, xrGetInstanceProcAddr(m_instance, "xrGetOpenGLGraphicsRequirementsKHR", (PFN_xrVoidFunction*)&pfnGetOpenGLGraphicsRequirementsKHR), "xrGetInstanceProcAddr(xrGetOpenGLGraphicsRequirementsKHR)"))
        return false;
    
    if (pfnGetOpenGLGraphicsRequirementsKHR) {
        m_graphicsRequirements = {XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR};
        if (!xrCheck(m_instance, pfnGetOpenGLGraphicsRequirementsKHR(m_instance, m_systemId, &m_graphicsRequirements), "xrGetOpenGLGraphicsRequirementsKHR"))
            return false;
    }

    logger->info("OpenXR initialized successfully.");
    return true;
}

bool VRManager::CreateSession(HDC hDC, HGLRC hGLRC) {
    if (m_instance == XR_NULL_HANDLE) return false;
    if (m_session != XR_NULL_HANDLE) return true;

    // Auto-detect using SDL3 if handles are missing
    if (!hDC || !hGLRC) {
        SDL_Window* window = SDL_GL_GetCurrentWindow();
        HGLRC context = (HGLRC)SDL_GL_GetCurrentContext();
        
        if (window && context) {
            hGLRC = context;
            #ifdef _WIN32
            SDL_PropertiesID props = SDL_GetWindowProperties(window);
            HWND hwnd = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
            if (hwnd) {
                hDC = GetDC(hwnd); // Note: Should probably ReleaseDC somewhere, but for main window it's fine
            }
            #endif
        }
    }

    if (!hDC || !hGLRC) {
        if (logger)
            logger->error("VRManager: Failed to obtain OpenGL context handles.");
        return false;
    }

    XrGraphicsBindingOpenGLWin32KHR graphicsBinding = {XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR};
    graphicsBinding.hDC = hDC;
    graphicsBinding.hGLRC = hGLRC;

    XrSessionCreateInfo sessionInfo = {XR_TYPE_SESSION_CREATE_INFO};
    sessionInfo.next = &graphicsBinding;
    sessionInfo.systemId = m_systemId;

    if (!xrCheck(m_instance, xrCreateSession(m_instance, &sessionInfo, &m_session), "xrCreateSession"))
        return false;

    // Create Reference Space
    XrReferenceSpaceCreateInfo spaceInfo = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL; // Headset relative to startup
    spaceInfo.poseInReferenceSpace.orientation.w = 1.0f;
    
    if (!xrCheck(m_instance, xrCreateReferenceSpace(m_session, &spaceInfo, &m_appSpace), "xrCreateReferenceSpace"))
        return false;

    if (!CreateSwapchains()) return false;

    m_sessionRunning = false;
    return true;
}

bool VRManager::CreateSwapchains() {
    // Get View Configuration
    uint32_t viewCount = 0;
    xrEnumerateViewConfigurationViews(m_instance, m_systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &viewCount, nullptr);
    std::vector<XrViewConfigurationView> configViews(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
    xrEnumerateViewConfigurationViews(m_instance, m_systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, viewCount, &viewCount, configViews.data());

    m_views.resize(viewCount);
    m_xrViews.resize(viewCount, {XR_TYPE_VIEW});
    m_projectionViews.resize(viewCount, {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW});
    m_swapchains.resize(viewCount);

    uint32_t blendModeCount = 0;
    if (xrCheck(m_instance, xrEnumerateEnvironmentBlendModes(m_instance, m_systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &blendModeCount, nullptr), "xrEnumerateEnvironmentBlendModes")) {
        std::vector<XrEnvironmentBlendMode> modes(blendModeCount);
        if (xrCheck(m_instance, xrEnumerateEnvironmentBlendModes(m_instance, m_systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, blendModeCount, &blendModeCount, modes.data()), "xrEnumerateEnvironmentBlendModes")) {
            XrEnvironmentBlendMode picked = modes.empty() ? XR_ENVIRONMENT_BLEND_MODE_OPAQUE : modes[0];
            auto hasMode = [&modes](XrEnvironmentBlendMode mode) {
                return std::find(modes.begin(), modes.end(), mode) != modes.end();
            };
            if (hasMode(XR_ENVIRONMENT_BLEND_MODE_OPAQUE)) picked = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
            else if (hasMode(XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND)) picked = XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND;
            else if (hasMode(XR_ENVIRONMENT_BLEND_MODE_ADDITIVE)) picked = XR_ENVIRONMENT_BLEND_MODE_ADDITIVE;
            m_environmentBlendMode = picked;
            if (logger) {
                logger->info("OpenXR blend mode: {}", static_cast<int>(m_environmentBlendMode));
            }
        }
    }

    for (uint32_t i = 0; i < viewCount; i++) {
        // Create Swapchain
        XrSwapchainCreateInfo swapchainInfo = {XR_TYPE_SWAPCHAIN_CREATE_INFO};
        swapchainInfo.arraySize = 1;
        swapchainInfo.format = GL_RGBA8; // Simple format
        swapchainInfo.width = configViews[i].recommendedImageRectWidth;
        swapchainInfo.height = configViews[i].recommendedImageRectHeight;
        swapchainInfo.mipCount = 1;
        swapchainInfo.faceCount = 1;
        swapchainInfo.sampleCount = 1; // configViews[i].recommendedSwapchainSampleCount;
        swapchainInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

        if (!xrCheck(m_instance, xrCreateSwapchain(m_session, &swapchainInfo, &m_swapchains[i].handle), "xrCreateSwapchain"))
            return false;
        m_swapchains[i].width = swapchainInfo.width;
        m_swapchains[i].height = swapchainInfo.height;
        if (logger) {
            logger->info("OpenXR view {} swapchain: {}x{}", i, m_swapchains[i].width, m_swapchains[i].height);
        }

        // Enumerate Images
        uint32_t imageCount = 0;
        xrEnumerateSwapchainImages(m_swapchains[i].handle, 0, &imageCount, nullptr);
        m_swapchains[i].images.resize(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR});
        xrEnumerateSwapchainImages(m_swapchains[i].handle, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)m_swapchains[i].images.data());

        // Setup View Info
        m_views[i].index = i;
        m_views[i].width = swapchainInfo.width;
        m_views[i].height = swapchainInfo.height;
        m_views[i].swapchain = m_swapchains[i].handle;
        m_views[i].framebufferId = 0;
    }

    return true;
}

void VRManager::PollEvents() {
    XrEventDataBuffer eventData = {XR_TYPE_EVENT_DATA_BUFFER};
    while (xrPollEvent(m_instance, &eventData) == XR_SUCCESS) {
        if (eventData.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
            auto* stateEvent = (XrEventDataSessionStateChanged*)&eventData;
            m_sessionState = stateEvent->state;
            if (m_sessionState == XR_SESSION_STATE_READY) {
                XrSessionBeginInfo beginInfo = {XR_TYPE_SESSION_BEGIN_INFO};
                beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
                xrBeginSession(m_session, &beginInfo);
                m_sessionRunning = true;
            } else if (m_sessionState == XR_SESSION_STATE_STOPPING) {
                xrEndSession(m_session);
                m_sessionRunning = false;
            }
        }
        eventData = {XR_TYPE_EVENT_DATA_BUFFER};
    }
}

bool VRManager::BeginFrame() {
    PollEvents();
    if (!m_sessionRunning) return false;

    XrFrameWaitInfo waitInfo = {XR_TYPE_FRAME_WAIT_INFO};
    m_frameState = {XR_TYPE_FRAME_STATE};
    if (XR_FAILED(xrWaitFrame(m_session, &waitInfo, &m_frameState))) return false;

    XrFrameBeginInfo beginInfo = {XR_TYPE_FRAME_BEGIN_INFO};
    if (XR_FAILED(xrBeginFrame(m_session, &beginInfo))) return false;

    if (!m_frameState.shouldRender) {
        return false; // Just end frame later
    }

    if (!m_savedScissorStateValid) {
        m_savedScissorEnabled = (glIsEnabled(GL_SCISSOR_TEST) == GL_TRUE);
        GLint scissorBox[4] = {0, 0, 0, 0};
        glGetIntegerv(GL_SCISSOR_BOX, scissorBox);
        m_savedScissorBox[0] = scissorBox[0];
        m_savedScissorBox[1] = scissorBox[1];
        m_savedScissorBox[2] = scissorBox[2];
        m_savedScissorBox[3] = scissorBox[3];
        m_savedScissorStateValid = true;
    }
    glDisable(GL_SCISSOR_TEST);

    // Locate Views
    XrViewLocateInfo locateInfo = {XR_TYPE_VIEW_LOCATE_INFO};
    locateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    locateInfo.displayTime = m_frameState.predictedDisplayTime;
    locateInfo.space = m_appSpace;

    XrViewState viewState = {XR_TYPE_VIEW_STATE};
    uint32_t viewCountOutput;
    if (XR_FAILED(xrLocateViews(m_session, &locateInfo, &viewState, (uint32_t)m_xrViews.size(), &viewCountOutput, m_xrViews.data()))) return false;

    // Update Views
    for (uint32_t i = 0; i < viewCountOutput; i++) {
        m_views[i].view = m_xrViews[i];
        
        // Convert Position/Orientation to GLM
        glm::quat orientation(m_xrViews[i].pose.orientation.w, m_xrViews[i].pose.orientation.x, m_xrViews[i].pose.orientation.y, m_xrViews[i].pose.orientation.z);
        glm::vec3 position(m_xrViews[i].pose.position.x, m_xrViews[i].pose.position.y, m_xrViews[i].pose.position.z);

        // View Matrix (Inverse of Pose)
        // OpenXR is Right-Handed. OpenGL is Right-Handed (usually).
        // MM7 might be Left-Handed or different.
        // For now, standard conversion.
        glm::mat4 rotation = glm::mat4_cast(orientation);
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 poseMat = translation * rotation;
        m_views[i].viewMatrix = glm::inverse(poseMat);

        // Projection Matrix
        float left = tan(m_xrViews[i].fov.angleLeft);
        float right = tan(m_xrViews[i].fov.angleRight);
        float down = tan(m_xrViews[i].fov.angleDown);
        float up = tan(m_xrViews[i].fov.angleUp);
        float nearZ = 4.0f; // Default near
        float farZ = 30000.0f; // Default far
        if (pCamera3D) {
            nearZ = pCamera3D->GetNearClip();
            farZ = pCamera3D->GetFarClip();
        }

        // Create standard projection matrix
        // This might need adjustment for OpenEnroth's coordinate system
        glm::mat4 proj(0.0f);
        proj[0][0] = 2.0f / (right - left);
        proj[1][1] = 2.0f / (up - down);
        proj[2][0] = (right + left) / (right - left);
        proj[2][1] = (up + down) / (up - down);
        proj[2][2] = -(farZ + nearZ) / (farZ - nearZ);
        proj[2][3] = -1.0f;
        proj[3][2] = -(2.0f * farZ * nearZ) / (farZ - nearZ);
        m_views[i].projectionMatrix = proj;

        m_projectionViews[i].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
        m_projectionViews[i].pose = m_xrViews[i].pose;
        m_projectionViews[i].fov = m_xrViews[i].fov;
        m_projectionViews[i].subImage.swapchain = m_swapchains[i].handle;
        m_projectionViews[i].subImage.imageRect.offset = {0, 0};
        m_projectionViews[i].subImage.imageRect.extent = {m_swapchains[i].width, m_swapchains[i].height};
    }

    return true;
}

void VRManager::EndFrame() {
    XrCompositionLayerProjection projectionLayer = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    projectionLayer.space = m_appSpace;
    projectionLayer.viewCount = (uint32_t)m_projectionViews.size();
    projectionLayer.views = m_projectionViews.data();

    const XrCompositionLayerBaseHeader* layers[] = { (const XrCompositionLayerBaseHeader*)&projectionLayer };

    XrFrameEndInfo endInfo = {XR_TYPE_FRAME_END_INFO};
    endInfo.displayTime = m_frameState.predictedDisplayTime;
    endInfo.environmentBlendMode = m_environmentBlendMode;
    endInfo.layerCount = 1;
    endInfo.layers = layers;

    xrCheck(m_instance, xrEndFrame(m_session, &endInfo), "xrEndFrame");

    if (m_savedScissorStateValid) {
        if (m_savedScissorEnabled) {
            glEnable(GL_SCISSOR_TEST);
        } else {
            glDisable(GL_SCISSOR_TEST);
        }
        glScissor(m_savedScissorBox[0], m_savedScissorBox[1], m_savedScissorBox[2], m_savedScissorBox[3]);
        m_savedScissorStateValid = false;
    }
}

unsigned int VRManager::AcquireSwapchainTexture(int viewIndex) {
    if (viewIndex < 0 || viewIndex >= m_swapchains.size()) return 0;

    uint32_t imageIndex;
    XrSwapchainImageAcquireInfo acquireInfo = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    if (XR_FAILED(xrAcquireSwapchainImage(m_swapchains[viewIndex].handle, &acquireInfo, &imageIndex))) return 0;

    XrSwapchainImageWaitInfo waitInfo = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
    waitInfo.timeout = XR_INFINITE_DURATION;
    if (XR_FAILED(xrWaitSwapchainImage(m_swapchains[viewIndex].handle, &waitInfo))) return 0;

    m_views[viewIndex].currentImageIndex = imageIndex;
    
    // Bind to FBO
    if (m_views[viewIndex].framebufferId == 0) {
        glGenFramebuffers(1, &m_views[viewIndex].framebufferId);
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, m_views[viewIndex].framebufferId);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_swapchains[viewIndex].images[imageIndex].image, 0);
    
    // Depth Buffer
    static std::vector<GLuint> depthBuffers;
    if (depthBuffers.size() <= viewIndex) depthBuffers.resize(viewIndex + 1, 0);
    
    if (depthBuffers[viewIndex] == 0) {
        glGenRenderbuffers(1, &depthBuffers[viewIndex]);
        glBindRenderbuffer(GL_RENDERBUFFER, depthBuffers[viewIndex]);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_swapchains[viewIndex].width, m_swapchains[viewIndex].height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffers[viewIndex]);

    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, m_swapchains[viewIndex].width, m_swapchains[viewIndex].height);

    GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
        if (logger) {
            logger->error("VR swapchain FBO incomplete (view {}): {}", viewIndex, static_cast<unsigned int>(fboStatus));
        }
    }

    if (std::getenv("OPENENROTH_VR_TEST_CLEAR") != nullptr) {
        if (viewIndex == 0) glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        else if (viewIndex == 1) glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
        else glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    return m_views[viewIndex].framebufferId;
}

void VRManager::ReleaseSwapchainTexture(int viewIndex) {
    if (viewIndex < 0 || viewIndex >= m_swapchains.size()) return;

    XrSwapchainImageReleaseInfo releaseInfo = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
    xrReleaseSwapchainImage(m_swapchains[viewIndex].handle, &releaseInfo);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void VRManager::Shutdown() {
    if (m_session != XR_NULL_HANDLE) {
        xrDestroySession(m_session);
        m_session = XR_NULL_HANDLE;
    }
    if (m_instance != XR_NULL_HANDLE) {
        xrDestroyInstance(m_instance);
        m_instance = XR_NULL_HANDLE;
    }
}

glm::mat4 VRManager::GetCurrentViewMatrix(const glm::vec3& worldOrigin, float yawRad, float pitchRad) {
    if (m_currentViewIndex < 0 || m_currentViewIndex >= m_views.size()) return glm::mat4(1.0f);
    
    // VR View Matrix (Headset -> Local Y-Up)
    glm::mat4 vrView = m_views[m_currentViewIndex].viewMatrix;
    
    // Conversion from Z-Up (OpenEnroth) to Y-Up (OpenXR)
    // Rotate -90 degrees around X axis
    // Z (0,0,1) -> Y (0,1,0)
    // Y (0,1,0) -> -Z (0,0,-1)
    glm::mat4 coordConvert = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1, 0, 0));
    
    // Game Rotation (Yaw)
    // We rotate the world around Z by -Yaw to match the camera's orientation.
    // We intentionally ignore pitch/roll from the game as the HMD provides those.
    // Note: We might need to adjust the phase of yaw (e.g. + PI/2) depending on 0-angle convention.
    // For now, assuming standard counter-clockwise Z rotation.
    glm::mat4 gameRotation = glm::rotate(glm::mat4(1.0f), -yawRad, glm::vec3(0, 0, 1));
    
    // Translation to World Origin (Party Position)
    // We invert the world origin to make it the camera position relative to origin
    glm::mat4 worldTrans = glm::translate(glm::mat4(1.0f), -worldOrigin);
    
    return vrView * coordConvert * gameRotation * worldTrans;
}

glm::mat4 VRManager::GetCurrentProjectionMatrix() {
    if (m_currentViewIndex < 0 || m_currentViewIndex >= m_views.size()) return glm::mat4(1.0f);
    return m_views[m_currentViewIndex].projectionMatrix;
}

void VRManager::BindSwapchainFramebuffer(int viewIndex) {
    if (viewIndex >= 0 && viewIndex < m_views.size()) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_views[viewIndex].framebufferId);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glDisable(GL_SCISSOR_TEST);
        glScissor(0, 0, m_views[viewIndex].width, m_views[viewIndex].height);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glViewport(0, 0, m_views[viewIndex].width, m_views[viewIndex].height);

        if (std::getenv("OPENENROTH_VR_GLSTATE_LOG") != nullptr) {
            GLint viewport[4] = {0, 0, 0, 0};
            glGetIntegerv(GL_VIEWPORT, viewport);
            GLboolean scissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
            GLint scissorBox[4] = {0, 0, 0, 0};
            glGetIntegerv(GL_SCISSOR_BOX, scissorBox);
            logger->info("VR GL state (view {}): viewport {} {} {} {}, scissor {} box {} {} {} {}",
                         viewIndex,
                         viewport[0], viewport[1], viewport[2], viewport[3],
                         scissorEnabled ? 1 : 0,
                         scissorBox[0], scissorBox[1], scissorBox[2], scissorBox[3]);
        }
    }
}

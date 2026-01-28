#include "VRManager.h"
#include <glad/gl.h>
#include <SDL3/SDL.h>
#include <iostream>
#include <array>
#include <vector>
#include <string>
#include <glm/gtc/matrix_transform.hpp>

// Helper para verificar erros OpenXR
#define XR_CHECK(x) { \
    XrResult result = (x); \
    if (XR_FAILED(result)) { \
        char resultString[XR_MAX_RESULT_STRING_SIZE]; \
        xrResultToString(m_instance, result, resultString); \
        std::cerr << "OpenXR Error: " << resultString << " (" << result << ") at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return false; \
    } \
}

VRManager& VRManager::Get() {
    static VRManager instance;
    return instance;
}

VRManager::VRManager() {}
VRManager::~VRManager() { Shutdown(); }

bool VRManager::Initialize() {
    if (m_instance != XR_NULL_HANDLE) return true;

    // 1. Extensions
    const char* extensions[] = {
        "XR_KHR_opengl_enable" 
    };

    // 2. Create Instance
    XrInstanceCreateInfo createInfo = {XR_TYPE_INSTANCE_CREATE_INFO};
    strcpy(createInfo.applicationInfo.applicationName, "OpenEnroth VR");
    createInfo.applicationInfo.applicationVersion = 1;
    strcpy(createInfo.applicationInfo.engineName, "OpenEnroth");
    createInfo.applicationInfo.engineVersion = 1;
    createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
    createInfo.enabledExtensionCount = 1;
    createInfo.enabledExtensionNames = extensions;

    XR_CHECK(xrCreateInstance(&createInfo, &m_instance));

    // 3. Get System
    XrSystemGetInfo systemInfo = {XR_TYPE_SYSTEM_GET_INFO};
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    
    XR_CHECK(xrGetSystem(m_instance, &systemInfo, &m_systemId));

    // 4. Requirements
    PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = nullptr;
    xrGetInstanceProcAddr(m_instance, "xrGetOpenGLGraphicsRequirementsKHR", (PFN_xrVoidFunction*)&pfnGetOpenGLGraphicsRequirementsKHR);
    
    if (pfnGetOpenGLGraphicsRequirementsKHR) {
        m_graphicsRequirements = {XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR};
        pfnGetOpenGLGraphicsRequirementsKHR(m_instance, m_systemId, &m_graphicsRequirements);
    }

    std::cout << "OpenXR Initialized successfully." << std::endl;
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
        std::cerr << "VRManager: Failed to obtain OpenGL context handles." << std::endl;
        return false;
    }

    XrGraphicsBindingOpenGLWin32KHR graphicsBinding = {XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR};
    graphicsBinding.hDC = hDC;
    graphicsBinding.hGLRC = hGLRC;

    XrSessionCreateInfo sessionInfo = {XR_TYPE_SESSION_CREATE_INFO};
    sessionInfo.next = &graphicsBinding;
    sessionInfo.systemId = m_systemId;

    XR_CHECK(xrCreateSession(m_instance, &sessionInfo, &m_session));

    // Create Reference Space
    XrReferenceSpaceCreateInfo spaceInfo = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL; // Headset relative to startup
    spaceInfo.poseInReferenceSpace.orientation.w = 1.0f;
    
    XR_CHECK(xrCreateReferenceSpace(m_session, &spaceInfo, &m_appSpace));

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

        XR_CHECK(xrCreateSwapchain(m_session, &swapchainInfo, &m_swapchains[i].handle));
        m_swapchains[i].width = swapchainInfo.width;
        m_swapchains[i].height = swapchainInfo.height;

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
        float nearZ = 0.1f; // TODO: Get from engine
        float farZ = 1000.0f; // TODO: Get from engine

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
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    endInfo.layerCount = 1;
    endInfo.layers = layers;

    xrEndFrame(m_session, &endInfo);
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

glm::mat4 VRManager::GetCurrentViewMatrix(const glm::vec3& worldOrigin) {
    if (m_currentViewIndex < 0 || m_currentViewIndex >= m_views.size()) return glm::mat4(1.0f);
    
    // VR View Matrix (Headset -> Local Y-Up)
    glm::mat4 vrView = m_views[m_currentViewIndex].viewMatrix;
    
    // Conversion from Z-Up (OpenEnroth) to Y-Up (OpenXR)
    // Rotate -90 degrees around X axis
    // Z (0,0,1) -> Y (0,1,0)
    // Y (0,1,0) -> -Z (0,0,-1)
    glm::mat4 coordConvert = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1, 0, 0));
    
    // Translation to World Origin (Party Position)
    // We invert the world origin to make it the camera position relative to origin
    glm::mat4 worldTrans = glm::translate(glm::mat4(1.0f), -worldOrigin);
    
    return vrView * coordConvert * worldTrans;
}

glm::mat4 VRManager::GetCurrentProjectionMatrix() {
    if (m_currentViewIndex < 0 || m_currentViewIndex >= m_views.size()) return glm::mat4(1.0f);
    return m_views[m_currentViewIndex].projectionMatrix;
}

void VRManager::BindSwapchainFramebuffer(int viewIndex) {
    if (viewIndex >= 0 && viewIndex < m_views.size()) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_views[viewIndex].framebufferId);
        glViewport(0, 0, m_views[viewIndex].width, m_views[viewIndex].height);
    }
}

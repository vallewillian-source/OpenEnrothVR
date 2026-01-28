#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <unknwn.h>
#ifdef DrawText
#undef DrawText
#endif
#endif

#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_OPENGL
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class VRManager {
public:
    static VRManager& Get();

    // Inicializa OpenXR Instance e System
    bool Initialize();
    
    // Inicializa Sessão OpenXR usando contexto OpenGL existente
    bool CreateSession(HDC hDC, HGLRC hGLRC);
    
    void Shutdown();
    void PollEvents();

    // Ciclo de Render
    bool BeginFrame(); // Retorna true se deve renderizar
    void EndFrame();

    struct VRView {
        int index;
        XrView view;
        glm::mat4 viewMatrix;
        glm::mat4 projectionMatrix;
        int width;
        int height;
        
        // Swapchain
        XrSwapchain swapchain;
        unsigned int currentImageIndex;
        unsigned int framebufferId; // Se usarmos FBO interno, ou apenas a textura ID
    };

    const std::vector<VRView>& GetViews() const { return m_views; }
    
    // Funções auxiliares para Swapchain
    unsigned int AcquireSwapchainTexture(int viewIndex);
    void ReleaseSwapchainTexture(int viewIndex);

    bool IsInitialized() const { return m_instance != XR_NULL_HANDLE; }
    bool IsSessionRunning() const { return m_sessionRunning; }
    
    void SetIsRenderingVREye(bool value) { m_isRenderingVR = value; }
    bool IsRenderingVREye() const { return m_isRenderingVR; }

    void SetCurrentViewIndex(int index) { m_currentViewIndex = index; }
    glm::mat4 GetCurrentViewMatrix(const glm::vec3& worldOrigin, float yawRad = 0.0f, float pitchRad = 0.0f);
    glm::mat4 GetCurrentProjectionMatrix();

    // Binds the framebuffer for the current swapchain image of the specified view
    void BindSwapchainFramebuffer(int viewIndex);

private:
    VRManager();
    ~VRManager();

    bool CreateInstance();
    bool GetSystem();
    bool CreateSwapchains();
    void DestroySwapchains();

    XrInstance m_instance = XR_NULL_HANDLE;
    XrSystemId m_systemId = XR_NULL_SYSTEM_ID;
    XrSession m_session = XR_NULL_HANDLE;
    XrSpace m_appSpace = XR_NULL_HANDLE;
    
    bool m_sessionRunning = false;
    XrSessionState m_sessionState = XR_SESSION_STATE_UNKNOWN;

    // Graphics requirements
    XrGraphicsRequirementsOpenGLKHR m_graphicsRequirements = {XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR};

    // Swapchain management
    struct SwapchainInfo {
        XrSwapchain handle;
        int width;
        int height;
        std::vector<XrSwapchainImageOpenGLKHR> images;
    };
    std::vector<SwapchainInfo> m_swapchains;
    
    // Frame State
    XrFrameState m_frameState = {XR_TYPE_FRAME_STATE};
    std::vector<VRView> m_views;
    std::vector<XrView> m_xrViews; // Raw views from OpenXR
    std::vector<XrCompositionLayerProjectionView> m_projectionViews;
    XrEnvironmentBlendMode m_environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    
    bool m_isRenderingVR = false;
    int m_currentViewIndex = 0;

    bool m_savedScissorStateValid = false;
    bool m_savedScissorEnabled = false;
    int m_savedScissorBox[4] = {0, 0, 0, 0};
};

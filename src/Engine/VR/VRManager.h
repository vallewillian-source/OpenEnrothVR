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
    int GetCurrentViewIndex() const { return m_currentViewIndex; }
    void GetViewSize(int viewIndex, int& w, int& h) const;
    glm::mat4 GetCurrentViewMatrix(const glm::vec3& worldOrigin, float yawRad = 0.0f, float pitchRad = 0.0f);
    glm::mat4 GetCurrentProjectionMatrix();
    void GetViewTangents(int viewIndex, float& l, float& r, float& u, float& d);

    // Binds the framebuffer for the current swapchain image of the specified view
    void BindSwapchainFramebuffer(int viewIndex);

    // Overlay / Virtual Screen methods
    void InitOverlay(int width, int height);
    void BeginOverlayRender();
    void EndOverlayRender();
    void CaptureScreenToOverlay(int srcWidth, int srcHeight);
    void RenderOverlay3D();

    // Overlay layer (OpenXR quad layer) methods
    void InitOverlayLayer(int width, int height);
    void CaptureScreenToOverlayLayer(int srcWidth, int srcHeight);
    void SetOverlayLayerEnabled(bool enabled);

    bool GetMenuMouseState(int menuWidth, int menuHeight, int& outX, int& outY, bool& outClickPressed);

    struct VRInputState {
        glm::vec2 move = {0.0f, 0.0f};
        glm::vec2 turn = {0.0f, 0.0f};
        bool jump = false;
        bool attack = false;
        bool castReady = false;
        bool interact = false;
        bool yell = false;
    };
    
    VRInputState GetVRInputState();

private:
    VRManager();
    ~VRManager();

    bool CreateInstance();
    bool GetSystem();
    bool CreateSwapchains();
    void DestroySwapchains();
    
    // Overlay resources
    unsigned int m_overlayFBO = 0;
    unsigned int m_overlayTexture = 0;
    unsigned int m_overlayDepthBuffer = 0; // Might not need depth for 2D overlay, but good practice
    int m_overlayWidth = 0;
    int m_overlayHeight = 0;
    
    // Overlay Quad resources
    unsigned int m_quadVAO = 0;
    unsigned int m_quadVBO = 0;
    unsigned int m_quadShader = 0;
    void InitOverlayQuad();
    void DrawOverlayQuad();

    XrInstance m_instance = XR_NULL_HANDLE;
    XrSystemId m_systemId = XR_NULL_SYSTEM_ID;
    XrSession m_session = XR_NULL_HANDLE;
    XrSpace m_appSpace = XR_NULL_HANDLE;
    XrSpace m_viewSpace = XR_NULL_HANDLE;
    
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

    // Overlay quad layer swapchain
    XrSwapchain m_overlayLayerSwapchain = XR_NULL_HANDLE;
    std::vector<XrSwapchainImageOpenGLKHR> m_overlayLayerImages;
    uint32_t m_overlayLayerImageIndex = 0;
    unsigned int m_overlayLayerFBO = 0;
    int m_overlayLayerWidth = 0;
    int m_overlayLayerHeight = 0;
    bool m_overlayLayerEnabled = false;
    bool m_overlayLayerHasFrame = false;
    bool m_overlayLayerAnchorPoseValid = false;
    XrPosef m_overlayLayerAnchorPose = {};

    XrActionSet m_menuActionSet = XR_NULL_HANDLE;
    XrAction m_menuAimPoseAction = XR_NULL_HANDLE;
    XrAction m_menuSelectClickAction = XR_NULL_HANDLE;
    XrAction m_menuSelectValueAction = XR_NULL_HANDLE;
    
    // Gameplay Actions
    XrActionSet m_gameplayActionSet = XR_NULL_HANDLE;
    XrAction m_actionMove = XR_NULL_HANDLE;
    XrAction m_actionTurn = XR_NULL_HANDLE;
    XrAction m_actionAttack = XR_NULL_HANDLE;
    XrAction m_actionCastReady = XR_NULL_HANDLE;
    XrAction m_actionInteract = XR_NULL_HANDLE; // Trigger Right
    XrAction m_actionYell = XR_NULL_HANDLE; // Trigger Left
    // Jump is derived from Turn (Right Thumbstick Up)

    XrSpace m_menuAimSpaceRight = XR_NULL_HANDLE;
    XrPath m_handLeftPath = XR_NULL_PATH;
    XrPath m_handRightPath = XR_NULL_PATH;
    bool m_menuSelectPressedPrev = false;
    
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

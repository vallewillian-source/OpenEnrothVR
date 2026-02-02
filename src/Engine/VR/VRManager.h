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
#include "VRInputState.h"

class VRManager {
public:
    using VRInputState = ::VRInputState;
    static VRManager& Get();

    // Inicializa OpenXR Instance e System
    bool Initialize();

    // Inicializa Sessão OpenXR usando contexto OpenGL existente
    bool CreateSession(HDC hDC, HGLRC hGLRC);

    void Shutdown();
    void PollEvents();

    // Ciclo de Render
    bool BeginFrame(); // Retorna true se deve renderizar
    bool ShouldRenderFrame() const { return m_shouldRenderThisFrame; }
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
    bool IsOverlayRenderActive() const { return m_isRenderingOverlay; }

    // Overlay layer (OpenXR quad layer) methods
    void InitOverlayLayer(int width, int height);
    void CaptureScreenToOverlayLayer(int srcWidth, int srcHeight);
    void SetOverlayLayerEnabled(bool enabled);

    bool GetMenuMouseState(int menuWidth, int menuHeight, int& outX, int& outY, bool& outClickPressed);

    VRInputState GetVRInputState();

    void SetDebugHouseIndicator(bool enabled);
    bool GetDebugHouseIndicator() const { return m_debugHouseIndicator; }

    void SetShowGuiBillboard(bool enabled);
    bool GetShowGuiBillboard() const { return m_showGuiBillboard; }

    // Dialogue Text HUD
    void SetDialogueText(const std::string& text);
    const std::string& GetDialogueText() const { return m_dialogueText; }
    void ClearDialogueText();

    struct DialogueOption {
        std::string text;
        int id;
        int msg; // UIMSG_SelectNPCDialogueOption etc.

        bool operator==(const DialogueOption& other) const {
            return id == other.id && msg == other.msg && text == other.text;
        }
    };
    void SetDialogueOptions(const std::vector<DialogueOption>& options);
    void ClearDialogueOptions();

    void RenderDialogueHUD();
    void RenderDialogueMenu();

private:
    void UpdateDialogueTexture();
    void UpdateDialogueMenuTexture();
    VRManager();
    ~VRManager();

    // World-locked house overlay state
    bool m_debugHouseIndicator = false;
    bool m_showGuiBillboard = false;
    bool m_housePoseInitialized = false;
    glm::vec3 m_houseOverlayWorldPos = glm::vec3(0.0f);
    glm::quat m_houseOverlayWorldRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    int m_houseOverlayScreenW = 0;
    int m_houseOverlayScreenH = 0;
    float m_houseOverlayWidthMeters = 0.0f;
    float m_houseOverlayHeightMeters = 0.0f;

    // World-locked gui billboard state
    bool m_guiBillboardPoseInitialized = false;
    glm::vec3 m_guiBillboardWorldPos = glm::vec3(0.0f);
    glm::quat m_guiBillboardWorldRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

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
    void InitDebugResources();
    void RenderDebugCircle(const glm::vec3& position, float radius = 0.05f, const glm::vec4& color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
    void RenderDebugController();
    void RenderLeftHouseRay();
    void InitRayResources();
    void UpdateLeftRay();

    unsigned int m_rayVAO = 0;
    unsigned int m_rayVBO = 0;
    unsigned int m_rayShader = 0;
    
    unsigned int m_debugVAO = 0;
    unsigned int m_debugVBO = 0;
    unsigned int m_debugShader = 0;
    int m_debugVertexCount = 0;
    
    float m_leftRayLength = 5.0f;
    bool m_leftRayValid = false;
    bool m_leftRayHitHouse = false;
    bool m_leftRayHitGUI = false;
    glm::vec3 m_leftRayHitPos = glm::vec3(0.0f);
    float m_houseHitX = 0.0f;
    float m_houseHitY = 0.0f;
    glm::vec3 m_leftRayOrigin = glm::vec3(0.0f);
    glm::vec3 m_leftRayDirection = glm::vec3(0.0f, 0.0f, -1.0f);

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
    // Removed old raycasting actions

    // Gameplay Actions
    XrActionSet m_gameplayActionSet = XR_NULL_HANDLE;
    XrAction m_actionMove = XR_NULL_HANDLE;
    XrAction m_actionTurn = XR_NULL_HANDLE;
    XrAction m_actionAttack = XR_NULL_HANDLE;
    XrAction m_actionCastReady = XR_NULL_HANDLE;
    XrAction m_actionInteract = XR_NULL_HANDLE; // Trigger Right
    XrAction m_actionEsc = XR_NULL_HANDLE; // Trigger Left
    XrAction m_actionCombat = XR_NULL_HANDLE; // Y Button
    XrAction m_actionCast = XR_NULL_HANDLE; // X Button
    XrAction m_actionFlyUp = XR_NULL_HANDLE; // Grip Right
    XrAction m_actionFlyDown = XR_NULL_HANDLE; // Grip Left
    XrAction m_actionQuest = XR_NULL_HANDLE; // Left Stick Click
    XrAction m_actionPass = XR_NULL_HANDLE; // Right Stick Click
    XrAction m_actionLeftAimPose = XR_NULL_HANDLE;
    // Jump is derived from Turn (Right Thumbstick Up)

    XrPath m_handLeftPath = XR_NULL_PATH;
    XrPath m_handRightPath = XR_NULL_PATH;
    XrSpace m_leftAimSpace = XR_NULL_HANDLE;

    // New Menu Cursor State
    float m_menuCursorX = 0.0f;
    float m_menuCursorY = 0.0f;
    float m_menuCursorSpeed = 4.0f; // Pixels per frame factor (adjust as needed)
    bool m_menuCursorInitialized = false;
    bool m_menuSelectPressedPrev = false;
    bool m_guiBillboardButtonPressedPrev = false;
    bool m_waitForTriggerRelease = false;

    // Dialogue HUD
    std::string m_dialogueText;
    unsigned int m_dialogueFontTexture = 0; // Future use if we want a dedicated font
    int m_dialogueTextureWidth = 0;
    int m_dialogueTextureHeight = 0;

    // Dialogue Menu
    std::vector<DialogueOption> m_dialogueOptions;
    int m_selectedOptionIndex = 0;
    unsigned int m_dialogueMenuTexture = 0;
    int m_menuTextureWidth = 0;
    int m_menuTextureHeight = 0;
    bool m_menuInputCooldown = false;
    uint32_t m_lastMenuInputTime = 0;

    // Frame State
    XrFrameState m_frameState = {XR_TYPE_FRAME_STATE};
    std::vector<VRView> m_views;
    std::vector<XrView> m_xrViews; // Raw views from OpenXR
    std::vector<XrCompositionLayerProjectionView> m_projectionViews;
    XrEnvironmentBlendMode m_environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;

    bool m_frameBegun = false;
    bool m_shouldRenderThisFrame = false;
    bool m_isRenderingVR = false;
    int m_currentViewIndex = 0;
    bool m_savedScissorStateValid = false;
    bool m_savedScissorEnabled = false;
    int m_savedScissorBox[4] = {0, 0, 0, 0};
    bool m_isRenderingOverlay = false;
};

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
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <iostream>
#include <sstream>

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/AssetsManager.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Graphics/Image.h"
#include "Engine/Graphics/TurnBasedOverlay.h"
#include "GUI/GUIFont.h"
#include "GUI/GUIEnums.h"
#include "Engine/Graphics/Camera.h"
#include "GUI/GUIMessageQueue.h"
#include "Library/Logger/Logger.h"
#include "GUI/UI/UIGame.h"
#include "Engine/Party.h"

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

static int64_t SelectColorSwapchainFormat(XrSession session) {
    uint32_t formatCount = 0;
    xrEnumerateSwapchainFormats(session, 0, &formatCount, nullptr);
    std::vector<int64_t> formats(formatCount);
    xrEnumerateSwapchainFormats(session, formatCount, &formatCount, formats.data());

    int64_t selectedFormat = formats[0]; // Fallback

    // Prefer SRGB to avoid washed out colors (double gamma correction issues)
    // If the game renders in sRGB space (which it likely does), we should tell OpenXR that.
    for (int64_t format : formats) {
        if (format == GL_SRGB8_ALPHA8) {
            return format;
        }
    }
    
    // Fallback to RGBA8 if SRGB not found
    for (int64_t format : formats) {
        if (format == GL_RGBA8) {
            return format;
        }
    }

    return selectedFormat;
}

void VRManager::SetDebugHouseIndicator(bool enabled) {
    if (enabled && !m_debugHouseIndicator) {
        // Reset anchor initialization when entering a house
        m_housePoseInitialized = false;
        m_waitForTriggerRelease = true; // Prevent accidental selection when entering
        if (logger) {
            logger->info("VRManager: DebugHouseIndicator ENABLED (Entering house)");
        }
    } else if (!enabled && m_debugHouseIndicator) {
        if (logger) {
            logger->info("VRManager: DebugHouseIndicator DISABLED (Leaving house)");
        }
    }
    m_debugHouseIndicator = enabled;
}

void VRManager::SetShowGuiBillboard(bool enabled) {
    if (enabled && !m_showGuiBillboard) {
        m_guiBillboardPoseInitialized = false;
        if (logger) {
            logger->info("VRManager: ShowGuiBillboard ENABLED (MVP Character Screen)");
        }
    }
    m_showGuiBillboard = enabled;
}

void VRManager::UpdateDialogueTexture() {
    if (m_dialogueText.empty()) return;

    std::string textToRender = m_dialogueText;
    
    // Get font - we use the same font as GUIWindow::DrawDialoguePanel
    if (!assets || !assets->pFontArrus) {
        if (logger) logger->error("VRManager: Assets ou pFontArrus não encontrados!");
        return;
    }
    GUIFont* font = assets->pFontArrus.get();

    int maxWidth = 600; // Fixed width for VR HUD panel
    int indent = 10;
    
    // Check if we need a smaller font (like in GUIWindow)
    int textHeight = font->CalcTextHeight(textToRender, maxWidth, indent) + 20;
    if (textHeight > 400) { // arbitrary limit for VR panel
        if (assets->pFontCreate) {
            font = assets->pFontCreate.get();
            textHeight = font->CalcTextHeight(textToRender, maxWidth, indent) + 20;
        }
    }

    std::string wrapped = font->WrapText(textToRender, maxWidth, indent);
    
    m_dialogueTextureWidth = maxWidth;
    m_dialogueTextureHeight = textHeight;

    if (logger) {
        logger->info("VRManager: Atualizando textura de diálogo. Dimensões: {}x{}", m_dialogueTextureWidth, m_dialogueTextureHeight);
    }

    // Create buffer
    std::vector<Color> buffer(m_dialogueTextureWidth * m_dialogueTextureHeight, Color(0, 0, 0, 180)); // Fundo preto semi-transparente direto no buffer

    // Render lines
    std::istringstream stream(wrapped);
    std::string line;
    int y = 10;
    while (std::getline(stream, line)) {
        if (y + font->GetHeight() > m_dialogueTextureHeight) break;
        font->DrawTextLineToBuff(colorTable.White, colorTable.Black, &buffer[y * m_dialogueTextureWidth + indent], line, m_dialogueTextureWidth);
        y += font->GetHeight() - 3; // Match CalcTextHeight logic
    }

    // Upload to texture
    if (m_dialogueFontTexture == 0) {
        glGenTextures(1, &m_dialogueFontTexture);
    }
    glBindTexture(GL_TEXTURE_2D, m_dialogueFontTexture);

    // Flip buffer vertically for OpenGL (which expects bottom-to-top)
    std::vector<Color> flippedBuffer(m_dialogueTextureWidth * m_dialogueTextureHeight);
    for (int row = 0; row < m_dialogueTextureHeight; ++row) {
        std::memcpy(&flippedBuffer[(m_dialogueTextureHeight - 1 - row) * m_dialogueTextureWidth],
                    &buffer[row * m_dialogueTextureWidth],
                    m_dialogueTextureWidth * sizeof(Color));
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_dialogueTextureWidth, m_dialogueTextureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, flippedBuffer.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void VRManager::ClearDialogueText() {
    if (!m_dialogueText.empty()) {
        m_dialogueText.clear();
        UpdateDialogueTexture();
    }
}

void VRManager::SetDialogueOptions(const std::vector<DialogueOption>& options) {
    if (m_dialogueOptions == options) {
        return;
    }

    if (logger) {
        logger->debug("VRManager: SetDialogueOptions mudou ({} opções)", options.size());
        for (size_t i = 0; i < options.size(); ++i) {
            logger->debug("  Opção {}: '{}' (ID: {}, Msg: {})", i, options[i].text, options[i].id, options[i].msg);
        }
    }
    m_dialogueOptions = options;
    m_selectedOptionIndex = 0;
    m_waitForTriggerRelease = true; // Prevent accidental selection when options change
    UpdateDialogueMenuTexture();
}

void VRManager::ClearDialogueOptions() {
    if (!m_dialogueOptions.empty() && logger) {
        logger->debug("VRManager: ClearDialogueOptions chamada (limpando {} opções)", m_dialogueOptions.size());
    }
    m_dialogueOptions.clear();
    m_selectedOptionIndex = 0;
}

void VRManager::UpdateDialogueMenuTexture() {
    if (m_dialogueOptions.empty()) {
        if (logger) logger->info("VRManager: UpdateDialogueMenuTexture ignorada (sem opções)");
        return;
    }

    if (!assets || !assets->pFontArrus) {
        if (logger) logger->error("VRManager: UpdateDialogueMenuTexture falhou: assets ou pFontArrus nulo");
        return;
    }
    GUIFont* font = assets->pFontArrus.get();

    int maxWidth = 400; // Menu is narrower than text HUD
    int indent = 10;
    int itemHeight = font->GetHeight() + 5;
    int totalHeight = (int)m_dialogueOptions.size() * itemHeight + 20;

    m_menuTextureWidth = maxWidth;
    m_menuTextureHeight = totalHeight;

    if (logger) {
        logger->debug("VRManager: Atualizando textura do menu. Dimensões: {}x{}, Itens: {}", 
                     m_menuTextureWidth, m_menuTextureHeight, m_dialogueOptions.size());
    }

    if (m_dialogueMenuTexture == 0) {
        glGenTextures(1, &m_dialogueMenuTexture);
        if (logger) logger->debug("VRManager: Gerada nova textura para menu: {}", m_dialogueMenuTexture);
    }

    glBindTexture(GL_TEXTURE_2D, m_dialogueMenuTexture);

    std::vector<Color> buffer(m_menuTextureWidth * m_menuTextureHeight, Color(0, 0, 0, 180));

    for (size_t i = 0; i < m_dialogueOptions.size(); ++i) {
        Color textColor = (i == (size_t)m_selectedOptionIndex) ? colorTable.Sunflower : colorTable.White;
        Color shadowColor = colorTable.Black;
        
        int y = 10 + (int)i * itemHeight;
        font->DrawTextLineToBuff(textColor, shadowColor, &buffer[y * m_menuTextureWidth + indent], m_dialogueOptions[i].text, m_menuTextureWidth);
    }

    // Flip buffer vertically for OpenGL
    std::vector<Color> flippedBuffer(m_menuTextureWidth * m_menuTextureHeight);
    for (int row = 0; row < m_menuTextureHeight; ++row) {
        std::memcpy(&flippedBuffer[(m_menuTextureHeight - 1 - row) * m_menuTextureWidth],
                    &buffer[row * m_menuTextureWidth],
                    m_menuTextureWidth * sizeof(Color));
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_menuTextureWidth, m_menuTextureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, flippedBuffer.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void VRManager::RenderDialogueMenu() {
    if (m_dialogueOptions.empty()) return;

    GLboolean prevBlend, prevDepthTest, prevCullFace, prevScissor;
    glGetBooleanv(GL_BLEND, &prevBlend);
    glGetBooleanv(GL_DEPTH_TEST, &prevDepthTest);
    glGetBooleanv(GL_CULL_FACE, &prevCullFace);
    glGetBooleanv(GL_SCISSOR_TEST, &prevScissor);

    if (m_dialogueMenuTexture == 0) return;

    glViewport(0, 0, m_views[m_currentViewIndex].width, m_views[m_currentViewIndex].height);
    glm::mat4 view = m_views[m_currentViewIndex].viewMatrix;

    // Use same local projection as HUD
    float left = tan(m_xrViews[m_currentViewIndex].fov.angleLeft);
    float right = tan(m_xrViews[m_currentViewIndex].fov.angleRight);
    float down = tan(m_xrViews[m_currentViewIndex].fov.angleDown);
    float up = tan(m_xrViews[m_currentViewIndex].fov.angleUp);
    float nearZ = 0.05f;
    float farZ = 100.0f;
    
    glm::mat4 localProjection(0.0f);
    localProjection[0][0] = 2.0f / (right - left);
    localProjection[1][1] = 2.0f / (up - down);
    localProjection[2][0] = (right + left) / (right - left);
    localProjection[2][1] = (up + down) / (up - down);
    localProjection[2][2] = -(farZ + nearZ) / (farZ - nearZ);
    localProjection[2][3] = -1.0f;
    localProjection[3][2] = -(2.0f * farZ * nearZ) / (farZ - nearZ);

    glm::mat4 invView = glm::inverse(view);
    glm::vec3 camPos = glm::vec3(invView[3]);
    glm::vec3 camFwd = glm::vec3(invView * glm::vec4(0, 0, -1, 0));
    glm::vec3 camUp = glm::vec3(invView * glm::vec4(0, 1, 0, 0));
    glm::vec3 camRight = glm::vec3(invView * glm::vec4(1, 0, 0, 0));

    // Menu position: 1.5m front, 0.2m down, 0.8m to the right of main text HUD
    glm::vec3 menuPos = camPos + camFwd * 1.5f - camUp * 0.2f + camRight * 0.85f;

    float aspect = (float)m_menuTextureWidth / (float)m_menuTextureHeight;
    float panelWidth = 0.6f;
    float panelHeight = panelWidth / aspect;

    glm::mat4 model = invView;
    model[3] = glm::vec4(menuPos, 1.0f);

    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // 1. Background
    InitDebugResources();
    if (m_debugShader != 0 && m_quadVAO != 0) {
        glUseProgram(m_debugShader);
        glUniformMatrix4fv(glGetUniformLocation(m_debugShader, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(m_debugShader, "projection"), 1, GL_FALSE, &localProjection[0][0]);
        
        glm::mat4 backgroundModel = glm::scale(model, glm::vec3(panelWidth + 0.02f, panelHeight + 0.02f, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(m_debugShader, "model"), 1, GL_FALSE, &backgroundModel[0][0]);
        glUniform4f(glGetUniformLocation(m_debugShader, "color"), 0.1f, 0.1f, 0.1f, 0.9f); 

        glBindVertexArray(m_quadVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    // 2. Menu Texture
    InitOverlayQuad();
    if (m_quadShader != 0 && m_quadVAO != 0) {
        glUseProgram(m_quadShader);
        glUniformMatrix4fv(glGetUniformLocation(m_quadShader, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(m_quadShader, "projection"), 1, GL_FALSE, &localProjection[0][0]);
        
        glm::mat4 menuModel = glm::scale(model, glm::vec3(panelWidth, panelHeight, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(m_quadShader, "model"), 1, GL_FALSE, &menuModel[0][0]);
        glUniform4f(glGetUniformLocation(m_quadShader, "color"), 1.0f, 1.0f, 1.0f, 1.0f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_dialogueMenuTexture);
        glUniform1i(glGetUniformLocation(m_quadShader, "screenTexture"), 0);

        glBindVertexArray(m_quadVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    if (prevScissor) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    if (prevCullFace) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (prevDepthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (prevBlend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
}

void VRManager::RenderMinimalCharacterHUD() {
    if (!pParty || current_screen_type != SCREEN_GAME || m_showGuiBillboard || !m_showMinimalHUD) return;

    GLboolean prevBlend, prevDepthTest, prevCullFace, prevScissor;
    glGetBooleanv(GL_BLEND, &prevBlend);
    glGetBooleanv(GL_DEPTH_TEST, &prevDepthTest);
    glGetBooleanv(GL_CULL_FACE, &prevCullFace);
    glGetBooleanv(GL_SCISSOR_TEST, &prevScissor);

    glViewport(0, 0, m_views[m_currentViewIndex].width, m_views[m_currentViewIndex].height);
    glm::mat4 view = m_views[m_currentViewIndex].viewMatrix;

    float left = tan(m_xrViews[m_currentViewIndex].fov.angleLeft);
    float right = tan(m_xrViews[m_currentViewIndex].fov.angleRight);
    float down = tan(m_xrViews[m_currentViewIndex].fov.angleDown);
    float up = tan(m_xrViews[m_currentViewIndex].fov.angleUp);
    float nearZ = 0.05f;
    float farZ = 100.0f;
    
    glm::mat4 hudProjection(0.0f);
    hudProjection[0][0] = 2.0f / (right - left);
    hudProjection[1][1] = 2.0f / (up - down);
    hudProjection[2][0] = (right + left) / (right - left);
    hudProjection[2][1] = (up + down) / (up - down);
    hudProjection[2][2] = -(farZ + nearZ) / (farZ - nearZ);
    hudProjection[2][3] = -1.0f;
    hudProjection[3][2] = -(2.0f * farZ * nearZ) / (farZ - nearZ);

    glm::mat4 invView = glm::inverse(view);
    glm::vec3 camPos = glm::vec3(invView[3]);
    glm::vec3 camFwd = glm::vec3(invView * glm::vec4(0, 0, -1, 0));
    glm::vec3 camUp = glm::vec3(invView * glm::vec4(0, 1, 0, 0));
    glm::vec3 camRight = glm::vec3(invView * glm::vec4(1, 0, 0, 0));

    // Base position: 1.5m away, Bottom Right
    // Adjusted to align bottom right (approximate values, can be tuned)
    glm::vec3 basePos = camPos + camFwd * 1.5f + camRight * 0.8f - camUp * 0.5f; 
    
    // Closer offsets (approx 0.15 spacing - Reverted size requires more space than 0.11 but less than original 0.25)
    float offsets[] = {-0.225f, -0.075f, 0.075f, 0.225f};
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    
    InitOverlayQuad();
    if (m_quadShader == 0 || m_quadVAO == 0) return;
    
    glUseProgram(m_quadShader);
    glUniformMatrix4fv(glGetUniformLocation(m_quadShader, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(m_quadShader, "projection"), 1, GL_FALSE, &hudProjection[0][0]);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(m_quadShader, "screenTexture"), 0);
    // 80% Opacity
    glUniform4f(glGetUniformLocation(m_quadShader, "color"), 1.0f, 1.0f, 1.0f, 0.8f);

    glBindVertexArray(m_quadVAO);

    for (int i = 0; i < 4; ++i) {
        if (i >= pParty->pCharacters.size()) break;
        Character* pPlayer = &pParty->pCharacters[i];
        GraphicsImage* pPortrait = nullptr;
        
        if (pPlayer->IsEradicated()) {
            pPortrait = game_ui_player_face_eradicated;
        } else if (pPlayer->IsDead()) {
            pPortrait = game_ui_player_face_dead;
        } else {
            int idx = pPlayer->portraitImageIndex;
            if (idx >= 0 && idx < 56) {
                 pPortrait = game_ui_player_faces[i][idx];
            }
        }
        
        if (!pPortrait || !pPortrait->renderId().isValid()) continue;
        
        glm::vec3 charPos = basePos + camRight * offsets[i];
        
        // Draw Face - Reverted size
        float faceScale = 0.0015f; 
        float w = pPortrait->width() * faceScale;
        float h = pPortrait->height() * faceScale;
        
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, charPos);
        model = model * glm::mat4(glm::mat3(invView)); // Rotation
        model = glm::scale(model, glm::vec3(1.0f, -1.0f, 1.0f)); // Flip Y
        
        glm::mat4 faceModel = glm::scale(model, glm::vec3(w, h, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(m_quadShader, "model"), 1, GL_FALSE, &faceModel[0][0]);
        glBindTexture(GL_TEXTURE_2D, (unsigned int)pPortrait->renderId().value());
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        // Draw HP Bar (Left of face)
        if (pPlayer->health > 0) {
            double hpRatio = (double)pPlayer->health / (double)pPlayer->GetMaxHealth();
            if (hpRatio > 1.0) hpRatio = 1.0;
            
            GraphicsImage* pBarTex = game_ui_bar_green;
            if (hpRatio <= 0.25) pBarTex = game_ui_bar_red;
            else if (hpRatio <= 0.5) pBarTex = game_ui_bar_yellow;
            
            if (pBarTex && pBarTex->renderId().isValid()) {
                 float barW = 0.015f; // Reverted width
                 float barH = h; 
                 
                 glm::vec3 barPos = charPos - camRight * (w/2.0f + barW/2.0f + 0.005f);
                 
                 // Scale height by ratio
                 float currentH = barH * (float)hpRatio;
                 
                 // Adjust Y to anchor at bottom
                 glm::vec3 adjustedBarPos = barPos - camUp * (h/2.0f - currentH/2.0f);
                 
                 glm::mat4 barModel = glm::mat4(1.0f);
                 barModel = glm::translate(barModel, adjustedBarPos);
                 barModel = barModel * glm::mat4(glm::mat3(invView));
                 barModel = glm::scale(barModel, glm::vec3(1.0f, -1.0f, 1.0f));
                 barModel = glm::scale(barModel, glm::vec3(barW, currentH, 1.0f));
                 
                 glUniformMatrix4fv(glGetUniformLocation(m_quadShader, "model"), 1, GL_FALSE, &barModel[0][0]);
                 glBindTexture(GL_TEXTURE_2D, (unsigned int)pBarTex->renderId().value());
                 glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }
        }
        
        // Draw Mana Bar (Right of face)
        if (pPlayer->mana > 0) {
            double mpRatio = (double)pPlayer->mana / (double)pPlayer->GetMaxMana();
            if (mpRatio > 1.0) mpRatio = 1.0;
            
            GraphicsImage* pBarTex = game_ui_bar_blue;
            if (pBarTex && pBarTex->renderId().isValid()) {
                 float barW = 0.015f; // Reverted width
                 float barH = h;
                 
                 glm::vec3 barPos = charPos + camRight * (w/2.0f + barW/2.0f + 0.005f);
                 float currentH = barH * (float)mpRatio;
                 glm::vec3 adjustedBarPos = barPos - camUp * (h/2.0f - currentH/2.0f);
                 
                 glm::mat4 barModel = glm::mat4(1.0f);
                 barModel = glm::translate(barModel, adjustedBarPos);
                 barModel = barModel * glm::mat4(glm::mat3(invView));
                 barModel = glm::scale(barModel, glm::vec3(1.0f, -1.0f, 1.0f));
                 barModel = glm::scale(barModel, glm::vec3(barW, currentH, 1.0f));
                 
                 glUniformMatrix4fv(glGetUniformLocation(m_quadShader, "model"), 1, GL_FALSE, &barModel[0][0]);
                 glBindTexture(GL_TEXTURE_2D, (unsigned int)pBarTex->renderId().value());
                 glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }
        }
    }
    
    if (prevScissor) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    if (prevCullFace) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (prevDepthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (prevBlend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
}

void VRManager::RenderTurnBasedHUD() {
    static bool loggedTurnBasedActive = false;

    if (!turnBasedOverlay.IsActive()) {
        if (loggedTurnBasedActive && logger) {
             logger->info("VRManager: TurnBasedOverlay deactivated.");
             loggedTurnBasedActive = false;
        }
        return;
    }
    
    if (!loggedTurnBasedActive && logger) {
        logger->info("VRManager: TurnBasedOverlay active. Attempting to render HUD.");
        loggedTurnBasedActive = true;
    }

    GraphicsImage* icon = turnBasedOverlay.currentIcon();
    if (!icon) {
        if (logger) logger->warning("VRManager: TurnBasedOverlay active but no icon!");
        return;
    }
    
    unsigned int textureId = (unsigned int)icon->renderId().value();
    if (textureId == 0) {
        static bool loggedInvalidTex = false;
        if (!loggedInvalidTex && logger) {
            logger->warning("VRManager: TurnBasedOverlay icon has invalid texture ID!");
            loggedInvalidTex = true;
        }
        return;
    }

    GLboolean prevBlend, prevDepthTest, prevCullFace, prevScissor;
    glGetBooleanv(GL_BLEND, &prevBlend);
    glGetBooleanv(GL_DEPTH_TEST, &prevDepthTest);
    glGetBooleanv(GL_CULL_FACE, &prevCullFace);
    glGetBooleanv(GL_SCISSOR_TEST, &prevScissor);

    glViewport(0, 0, m_views[m_currentViewIndex].width, m_views[m_currentViewIndex].height);
    glm::mat4 view = m_views[m_currentViewIndex].viewMatrix;

    float left = tan(m_xrViews[m_currentViewIndex].fov.angleLeft);
    float right = tan(m_xrViews[m_currentViewIndex].fov.angleRight);
    float down = tan(m_xrViews[m_currentViewIndex].fov.angleDown);
    float up = tan(m_xrViews[m_currentViewIndex].fov.angleUp);
    float nearZ = 0.05f;
    float farZ = 100.0f;
    
    glm::mat4 hudProjection(0.0f);
    hudProjection[0][0] = 2.0f / (right - left);
    hudProjection[1][1] = 2.0f / (up - down);
    hudProjection[2][0] = (right + left) / (right - left);
    hudProjection[2][1] = (up + down) / (up - down);
    hudProjection[2][2] = -(farZ + nearZ) / (farZ - nearZ);
    hudProjection[2][3] = -1.0f;
    hudProjection[3][2] = -(2.0f * farZ * nearZ) / (farZ - nearZ);

    glm::mat4 invView = glm::inverse(view);
    glm::vec3 camPos = glm::vec3(invView[3]);
    glm::vec3 camFwd = glm::vec3(invView * glm::vec4(0, 0, -1, 0));
    glm::vec3 camUp = glm::vec3(invView * glm::vec4(0, 1, 0, 0));
    glm::vec3 camRight = glm::vec3(invView * glm::vec4(1, 0, 0, 0));

    // Position: Top Right relative to view
    // 1.5m away
    // 0.4m right
    // 0.3m up
    glm::vec3 hudPos = camPos + camFwd * 1.5f + camRight * 0.4f + camUp * 0.3f;

    // Scale
    float scale = 0.002f; // Adjust size
    float w = icon->width() * scale;
    float h = icon->height() * scale;

    // Billboard rotation (facing camera)
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, hudPos);
    
    // Extract rotation from view matrix inverse (camera rotation)
    // We want the quad to face the camera, so it should have the same rotation as the camera
    // (Camera looks down -Z, Quad faces +Z? No, Quad usually faces +Z or -Z defined in InitOverlayQuad)
    // InitOverlayQuad defines vertices at Z=0.
    // If we apply camera rotation to it, it will be aligned with camera view plane.
    
    // Construct rotation matrix from camera basis vectors
    // camRight, camUp, -camFwd are the basis vectors of the camera in world space.
    // We want the quad X to align with camRight, Y with camUp, Z with -camFwd (so it faces +Z which is towards camera? Wait).
    // Standard Quad (InitOverlayQuad) is usually drawn in XY plane.
    // If we rotate it to match camera orientation, it will face the camera (if we orient it correctly).
    // invView rotation part does exactly that.
    
    glm::mat4 rotation = glm::mat4(glm::mat3(invView));
    model = model * rotation;

    // Flip Y scale to correct upside-down icon
    model = glm::scale(model, glm::vec3(1.0f, -1.0f, 1.0f));

    static bool loggedHudPos = false;
    if (!loggedHudPos && logger) {
        glm::vec4 clipPos = hudProjection * view * glm::vec4(hudPos, 1.0f);
        glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;
        logger->info("VRManager: RenderTurnBasedHUD iconId={}, size={}x{} (scaled {}x{}), HUD NDC=({}, {}, {}), w={}",
                     textureId, icon->width(), icon->height(), w, h, ndc.x, ndc.y, ndc.z, clipPos.w);
        loggedHudPos = true;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    InitOverlayQuad(); 
    if (m_quadShader != 0 && m_quadVAO != 0) {
        glUseProgram(m_quadShader);
        glUniformMatrix4fv(glGetUniformLocation(m_quadShader, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(m_quadShader, "projection"), 1, GL_FALSE, &hudProjection[0][0]);
        
        glm::mat4 quadModel = glm::scale(model, glm::vec3(w, h, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(m_quadShader, "model"), 1, GL_FALSE, &quadModel[0][0]);
        glUniform4f(glGetUniformLocation(m_quadShader, "color"), 1.0f, 1.0f, 1.0f, 1.0f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glUniform1i(glGetUniformLocation(m_quadShader, "screenTexture"), 0);

        glBindVertexArray(m_quadVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        GLenum err = glGetError();
        if (err != GL_NO_ERROR && logger) {
            logger->error("VRManager: RenderTurnBasedHUD gl error: 0x{:x}", err);
        }
    }

    if (prevScissor) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    if (prevCullFace) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (prevDepthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (prevBlend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
}

void VRManager::SetDialogueText(const std::string& text) {
    if (m_dialogueText == text) return;
    
    if (logger) {
        logger->info("VRManager: Recebendo novo texto de diálogo: '{}'", text);
    }

    m_dialogueText = text;
    UpdateDialogueTexture();
}

VRManager& VRManager::Get() {
    static VRManager instance;
    return instance;
}

VRManager::VRManager() {}
VRManager::~VRManager() { Shutdown(); }

void VRManager::SetOverlayLayerEnabled(bool enabled) {
    if (enabled == m_overlayLayerEnabled) return;
    m_overlayLayerEnabled = enabled;
    m_overlayLayerHasFrame = false;
    m_overlayLayerAnchorPoseValid = false;
    m_menuSelectPressedPrev = false;
    m_menuCursorInitialized = false; // Reset cursor initialization so it re-centers next time
}

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

// ------------------------------------------------------------------------------------------------
// Overlay / Virtual Screen Implementation
// ------------------------------------------------------------------------------------------------

void VRManager::InitOverlay(int width, int height) {
    if (m_overlayFBO != 0) return; // Already initialized

    m_overlayWidth = width;
    m_overlayHeight = height;

    // Create FBO
    glGenFramebuffers(1, &m_overlayFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_overlayFBO);

    // Create Texture
    glGenTextures(1, &m_overlayTexture);
    glBindTexture(GL_TEXTURE_2D, m_overlayTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_overlayTexture, 0);

    // Create Depth/Stencil RBO (Optional, but good for safety if UI draws depth)
    // glGenRenderbuffers(1, &m_overlayDepthBuffer);
    // glBindRenderbuffer(GL_RENDERBUFFER, m_overlayDepthBuffer);
    // glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    // glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_overlayDepthBuffer);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        if (logger) logger->error("VR Overlay Framebuffer is not complete!");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    InitOverlayQuad();
}

void VRManager::BeginOverlayRender() {
    if (m_overlayFBO == 0) return;

    m_isRenderingOverlay = true;
    glBindFramebuffer(GL_FRAMEBUFFER, m_overlayFBO);
    glViewport(0, 0, m_overlayWidth, m_overlayHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Transparent background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void VRManager::EndOverlayRender() {
    m_isRenderingOverlay = false;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void VRManager::CaptureScreenToOverlay(int srcWidth, int srcHeight) {
    if (m_overlayFBO == 0) return;

    GLint oldReadFBO = 0, oldDrawFBO = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &oldReadFBO);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldDrawFBO);

    // glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_overlayFBO);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    glBlitFramebuffer(0, 0, srcWidth, srcHeight,
                      0, 0, m_overlayWidth, m_overlayHeight,
                      GL_COLOR_BUFFER_BIT, GL_LINEAR);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, oldReadFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, oldDrawFBO);
}

void VRManager::InitOverlayLayer(int width, int height) {
    if (m_session == XR_NULL_HANDLE) return;
    if (m_overlayLayerSwapchain != XR_NULL_HANDLE && m_overlayLayerWidth == width && m_overlayLayerHeight == height) return;

    if (m_overlayLayerSwapchain != XR_NULL_HANDLE) {
        xrDestroySwapchain(m_overlayLayerSwapchain);
        m_overlayLayerSwapchain = XR_NULL_HANDLE;
        m_overlayLayerImages.clear();
        m_overlayLayerHasFrame = false;
    }

    if (m_overlayLayerFBO != 0) {
        glDeleteFramebuffers(1, &m_overlayLayerFBO);
        m_overlayLayerFBO = 0;
    }

    int64_t swapchainFormat = SelectColorSwapchainFormat(m_session);
    if (logger) {
        logger->info("VRManager: Overlay Swapchain format selected: {}", swapchainFormat);
    }

    XrSwapchainCreateInfo swapchainInfo = {XR_TYPE_SWAPCHAIN_CREATE_INFO};
    swapchainInfo.arraySize = 1;
    swapchainInfo.format = swapchainFormat;
    swapchainInfo.width = width;
    swapchainInfo.height = height;
    swapchainInfo.mipCount = 1;
    swapchainInfo.faceCount = 1;
    swapchainInfo.sampleCount = 1;
    swapchainInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_SAMPLED_BIT;

    if (!xrCheck(m_instance, xrCreateSwapchain(m_session, &swapchainInfo, &m_overlayLayerSwapchain), "xrCreateSwapchain(overlayLayer)"))
        return;

    uint32_t imageCount = 0;
    xrEnumerateSwapchainImages(m_overlayLayerSwapchain, 0, &imageCount, nullptr);
    m_overlayLayerImages.resize(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR});
    xrEnumerateSwapchainImages(m_overlayLayerSwapchain, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)m_overlayLayerImages.data());

    glGenFramebuffers(1, &m_overlayLayerFBO);

    m_overlayLayerWidth = width;
    m_overlayLayerHeight = height;
    m_overlayLayerHasFrame = false;
}

void VRManager::CaptureScreenToOverlayLayer(int srcWidth, int srcHeight) {
    if (m_overlayLayerSwapchain == XR_NULL_HANDLE) {
        InitOverlayLayer(srcWidth, srcHeight);
    }
    if (m_overlayLayerSwapchain == XR_NULL_HANDLE) return;

    if (logger && std::getenv("OPENENROTH_VR_MENU_OVERLAY_LOG") != nullptr) {
        logger->info("VR menu overlay layer capture: src {}x{}, dst {}x{}",
                     srcWidth, srcHeight, m_overlayLayerWidth, m_overlayLayerHeight);
    }

    uint32_t imageIndex = 0;
    XrSwapchainImageAcquireInfo acquireInfo = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    if (!xrCheck(m_instance, xrAcquireSwapchainImage(m_overlayLayerSwapchain, &acquireInfo, &imageIndex), "xrAcquireSwapchainImage(overlayLayer)"))
        return;

    XrSwapchainImageWaitInfo waitInfo = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
    waitInfo.timeout = XR_INFINITE_DURATION;
    if (!xrCheck(m_instance, xrWaitSwapchainImage(m_overlayLayerSwapchain, &waitInfo), "xrWaitSwapchainImage(overlayLayer)"))
        return;

    m_overlayLayerImageIndex = imageIndex;

    GLint oldReadFBO = 0, oldDrawFBO = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &oldReadFBO);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldDrawFBO);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_overlayLayerFBO);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_overlayLayerImages[imageIndex].image, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    GLenum fboStatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
    if (fboStatus == GL_FRAMEBUFFER_COMPLETE) {
        glDisable(GL_SCISSOR_TEST);
        glViewport(0, 0, m_overlayLayerWidth, m_overlayLayerHeight);
        
        // Disable sRGB conversion to avoid washing out colors when writing to sRGB swapchain
        // The source is already sRGB (treated as linear by GL), and we want to copy raw values.
        glDisable(GL_FRAMEBUFFER_SRGB); 
        
        glBlitFramebuffer(0, 0, srcWidth, srcHeight,
                          0, 0, m_overlayLayerWidth, m_overlayLayerHeight,
                          GL_COLOR_BUFFER_BIT, GL_LINEAR);
    } else if (logger) {
        logger->error("VR overlay layer FBO incomplete: {}", static_cast<unsigned int>(fboStatus));
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, oldReadFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, oldDrawFBO);

    XrSwapchainImageReleaseInfo releaseInfo = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
    xrCheck(m_instance, xrReleaseSwapchainImage(m_overlayLayerSwapchain, &releaseInfo), "xrReleaseSwapchainImage(overlayLayer)");

    m_overlayLayerHasFrame = true;
}

// Simple Shader for Overlay
static const char* overlayVertSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

static const char* overlayFragSrc = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D screenTexture;
uniform vec4 color;

void main() {
    vec4 col = texture(screenTexture, TexCoord);
    // Simple alpha threshold if needed, or rely on blending
    // if (col.a < 0.01) discard;
    FragColor = col * color;
}
)";

static const char* rayVertSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 view;
uniform mat4 projection;
void main() {
    gl_Position = projection * view * vec4(aPos, 1.0);
}
)";

static const char* rayFragSrc = R"(
#version 330 core
out vec4 FragColor;
uniform vec4 color;
void main() {
    FragColor = color;
}
)";

static const char* debugVertSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

static const char* debugFragSrc = R"(
#version 330 core
out vec4 FragColor;
uniform vec4 color;

void main() {
    FragColor = color;
}
)";

void VRManager::InitOverlayQuad() {
    // Quad vertices (centered at 0,0, facing Z?)
    // 1.5m away means we place it at Z = -1.5 (in OpenXR space)
    // But we will use Model matrix to position it.
    // So let's define it at origin, 1x1 size (from -0.5 to 0.5)

    // X, Y, Z, U, V
    float vertices[] = {
        // positions          // texture coords
         0.5f,  0.375f, 0.0f,   1.0f, 1.0f, // Top Right (4:3 Aspect Ratio approx)
         0.5f, -0.375f, 0.0f,   1.0f, 0.0f, // Bottom Right
        -0.5f, -0.375f, 0.0f,   0.0f, 0.0f, // Bottom Left
        -0.5f,  0.375f, 0.0f,   0.0f, 1.0f  // Top Left
    };
    // Aspect Ratio 4:3 -> 1.0 width, 0.75 height.

    unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    unsigned int EBO;
    glGenBuffers(1, &EBO);

    glBindVertexArray(m_quadVAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Compile Shaders
    unsigned int vertex, fragment;
    int success;
    char infoLog[512];

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &overlayVertSrc, NULL);
    glCompileShader(vertex);
    // Check errors...
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        if (logger) logger->error("VR Overlay Vertex Shader Compilation Failed: {}", infoLog);
    }

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &overlayFragSrc, NULL);
    glCompileShader(fragment);
    // Check errors...
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        if (logger) logger->error("VR Overlay Fragment Shader Compilation Failed: {}", infoLog);
    }

    m_quadShader = glCreateProgram();
    glAttachShader(m_quadShader, vertex);
    glAttachShader(m_quadShader, fragment);
    glLinkProgram(m_quadShader);
    // Check errors...
    glGetProgramiv(m_quadShader, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(m_quadShader, 512, NULL, infoLog);
        if (logger) logger->error("VR Overlay Shader Linking Failed: {}", infoLog);
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void VRManager::InitRayResources() {
    if (m_rayVAO != 0) return;

    if (logger) logger->info("VRManager: Initializing Ray Resources...");

    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &rayVertSrc, NULL);
    glCompileShader(vertex);
    GLint success = 0;
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        if (logger) logger->error("VR Ray Vertex Shader Compilation Failed: {}", infoLog);
    } else if (logger) {
        logger->info("VRManager: Ray Vertex Shader Compiled successfully.");
    }

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &rayFragSrc, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        if (logger) logger->error("VR Ray Fragment Shader Compilation Failed: {}", infoLog);
    } else if (logger) {
        logger->info("VRManager: Ray Fragment Shader Compiled successfully.");
    }

    m_rayShader = glCreateProgram();
    glAttachShader(m_rayShader, vertex);
    glAttachShader(m_rayShader, fragment);
    glLinkProgram(m_rayShader);
    glGetProgramiv(m_rayShader, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(m_rayShader, 512, NULL, infoLog);
        if (logger) logger->error("VR Ray Shader Linking Failed: {}", infoLog);
    } else if (logger) {
        logger->info("VRManager: Ray Shader Program linked successfully.");
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    glGenVertexArrays(1, &m_rayVAO);
    glGenBuffers(1, &m_rayVBO);
    glBindVertexArray(m_rayVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_rayVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    if (logger) logger->info("VRManager: Ray Resources initialized (VAO: {}, VBO: {}, Shader: {})", m_rayVAO, m_rayVBO, m_rayShader);
}

void VRManager::InitDebugResources() {
    if (m_debugVAO != 0) return;

    if (logger) logger->info("VRManager: Initializing Debug Resources...");

    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &debugVertSrc, NULL);
    glCompileShader(vertex);
    GLint success = 0;
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        if (logger) logger->error("VR Debug Vertex Shader Compilation Failed: {}", infoLog);
    } else if (logger) {
        logger->info("VRManager: Debug Vertex Shader Compiled successfully.");
    }

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &debugFragSrc, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        if (logger) logger->error("VR Debug Fragment Shader Compilation Failed: {}", infoLog);
    } else if (logger) {
        logger->info("VRManager: Debug Fragment Shader Compiled successfully.");
    }

    m_debugShader = glCreateProgram();
    glAttachShader(m_debugShader, vertex);
    glAttachShader(m_debugShader, fragment);
    glLinkProgram(m_debugShader);
    glGetProgramiv(m_debugShader, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(m_debugShader, 512, NULL, infoLog);
        if (logger) logger->error("VR Debug Shader Linking Failed: {}", infoLog);
    } else if (logger) {
        logger->info("VRManager: Debug Shader Program linked successfully.");
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    // Generate geometry for a simple circle (triangle fan)
    std::vector<float> circleVertices;
    const int segments = 32;
    const float radius = 1.0f;
    
    // Center vertex
    circleVertices.push_back(0.0f);
    circleVertices.push_back(0.0f);
    circleVertices.push_back(0.0f);
    
    // Outer vertices
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        circleVertices.push_back(cos(angle) * radius);
        circleVertices.push_back(sin(angle) * radius);
        circleVertices.push_back(0.0f);
    }

    glGenVertexArrays(1, &m_debugVAO);
    glGenBuffers(1, &m_debugVBO);
    glBindVertexArray(m_debugVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_debugVBO);
    glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(float), circleVertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    m_debugVertexCount = circleVertices.size() / 3;

    if (logger) logger->info("VRManager: Debug Resources initialized (VAO: {}, VBO: {}, Shader: {}, Vertices: {})", 
                             m_debugVAO, m_debugVBO, m_debugShader, (int)m_debugVertexCount);
}

void VRManager::UpdateLeftRay() {
    m_leftRayValid = false;
    if (m_session == XR_NULL_HANDLE || !m_sessionRunning) return;
    if (m_leftAimSpace == XR_NULL_HANDLE) {
        static bool logged = false;
        if (!logged && logger) {
            logger->warning("VRManager: m_leftAimSpace is NULL");
            logged = true;
        }
        return;
    }

    XrSpaceLocation location = {XR_TYPE_SPACE_LOCATION};
    XrResult res = xrLocateSpace(m_leftAimSpace, m_appSpace, m_frameState.predictedDisplayTime, &location);
    if (XR_FAILED(res)) {
        static bool loggedErr = false;
        if (!loggedErr && logger) {
            logger->warning("VRManager: xrLocateSpace failed with {}", (int)res);
            loggedErr = true;
        }
        return;
    }

    if ((location.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) == 0 ||
        (location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) == 0) {
        static bool loggedFlags = false;
        if (!loggedFlags && logger) {
            logger->warning("VRManager: Left ray location flags invalid: {}", (int)location.locationFlags);
            loggedFlags = true;
        }
        return;
    }

    static bool loggedSuccess = false;
    if (!loggedSuccess && logger) {
        logger->info("VRManager: Left controller pose located successfully!");
        loggedSuccess = true;
    }

    glm::quat orientation(
        location.pose.orientation.w,
        location.pose.orientation.x,
        location.pose.orientation.y,
        location.pose.orientation.z
    );
    glm::vec3 position(
        location.pose.position.x,
        location.pose.position.y,
        location.pose.position.z
    );

    m_leftRayOrigin = position;
    // Aim pose forward is usually -Z in OpenXR
    m_leftRayDirection = glm::normalize(orientation * glm::vec3(0.0f, 0.0f, -1.0f));
    m_leftRayValid = true;

    // Check intersection with House Overlay
    m_leftRayHitHouse = false;
    m_leftRayHitGUI = false;
    m_leftRayLength = 10.0f; // Reset length

    if (m_housePoseInitialized && m_debugHouseIndicator) {
        glm::vec3 planeNormal = m_houseOverlayWorldRot * glm::vec3(0.0f, 0.0f, 1.0f); // Face towards camera (Z+)
        float denom = glm::dot(m_leftRayDirection, planeNormal);
        
        // Ray should hit the front face (denom < 0)
        if (denom < -0.0001f) {
            float t = glm::dot(m_houseOverlayWorldPos - m_leftRayOrigin, planeNormal) / denom;
            if (t > 0.0f) {
                glm::vec3 hitPoint = m_leftRayOrigin + m_leftRayDirection * t;
                
                // Transform to local space of the plane to check bounds
                glm::vec3 localHit = glm::inverse(m_houseOverlayWorldRot) * (hitPoint - m_houseOverlayWorldPos);
                
                // Check bounds (assuming plane is centered at m_houseOverlayWorldPos)
                // Note: The screen rendering centers it.
                float halfW = m_houseOverlayWidthMeters * 0.5f;
                float halfH = m_houseOverlayHeightMeters * 0.5f;
                
                if (localHit.x >= -halfW && localHit.x <= halfW &&
                        localHit.y >= -halfH && localHit.y <= halfH) {
                    
                        m_leftRayHitHouse = true;
                        m_leftRayHitPos = hitPoint;
                        m_leftRayLength = t; // Stop ray at the screen
                        
                        // Calculate normalized screen coordinates (0..1)
                        // X: -halfW..halfW -> 0..1
                        m_houseHitX = (localHit.x / m_houseOverlayWidthMeters) + 0.5f;
                        // Y: halfH..-halfH -> 0..1 (Screen Y is down, 3D Y is up)
                        m_houseHitY = 0.5f - (localHit.y / m_houseOverlayHeightMeters);

                        // Optional: Log hit once
                    static bool loggedHit = false;
                    if (!loggedHit && logger) {
                        logger->info("VRManager: Ray Hit House Overlay at dist {}, local({},{})", t, localHit.x, localHit.y);
                        loggedHit = true;
                    }
                }
            }
        }
    }

    // Check intersection with GUI Billboard
    if (m_showGuiBillboard && m_guiBillboardPoseInitialized) {
        glm::vec3 planeNormal = m_guiBillboardWorldRot * glm::vec3(0.0f, 0.0f, 1.0f); // Face towards camera (Z+)
        float denom = glm::dot(m_leftRayDirection, planeNormal);
        
        // Ray should hit the front face (denom < 0)
        if (denom < -0.0001f) {
            float t = glm::dot(m_guiBillboardWorldPos - m_leftRayOrigin, planeNormal) / denom;
            if (t > 0.0f && t < m_leftRayLength) { // Only if closer than previous hit (house)
                glm::vec3 hitPoint = m_leftRayOrigin + m_leftRayDirection * t;
                
                // Transform to local space of the plane to check bounds
                glm::vec3 localHit = glm::inverse(m_guiBillboardWorldRot) * (hitPoint - m_guiBillboardWorldPos);
                
                // Check bounds (1.2m width, 0.9m height)
                float width = 1.2f;
                float height = 0.9f;
                float halfW = width * 0.5f;
                float halfH = height * 0.5f;
                
                if (localHit.x >= -halfW && localHit.x <= halfW &&
                        localHit.y >= -halfH && localHit.y <= halfH) {
                    
                        m_leftRayHitGUI = true;
                        m_leftRayHitHouse = false; // Override house hit
                        m_leftRayHitPos = hitPoint;
                        m_leftRayLength = t;
                        
                        // Calculate normalized screen coordinates (0..1)
                        m_guiHitX = (localHit.x / width) + 0.5f;
                        m_guiHitY = 0.5f - (localHit.y / height);
                        
                        // Optional: Log hit once
                    static bool loggedGuiHit = false;
                    if (!loggedGuiHit && logger) {
                        logger->info("VRManager: Ray Hit GUI Billboard at dist {}, local({},{})", t, localHit.x, localHit.y);
                        loggedGuiHit = true;
                    }
                }
            }
        }
    }
}

void VRManager::RenderLeftHouseRay() {
    if (m_currentViewIndex < 0 || m_currentViewIndex >= static_cast<int>(m_views.size())) return;

    InitRayResources();
    if (m_rayVAO == 0 || m_rayShader == 0) return;

    UpdateLeftRay();
    if (!m_leftRayValid) return;

    static bool loggedRender = false;
    if (!loggedRender && logger) {
        logger->info("VRManager: Rendering Left House Ray...");
        loggedRender = true;
    }

    glDisable(GL_DEPTH_TEST);
    glLineWidth(3.0f);

    glm::vec3 endPos = m_leftRayOrigin + m_leftRayDirection * m_leftRayLength;
    float vertices[6] = {
        m_leftRayOrigin.x, m_leftRayOrigin.y, m_leftRayOrigin.z,
        endPos.x, endPos.y, endPos.z
    };

    glBindBuffer(GL_ARRAY_BUFFER, m_rayVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glUseProgram(m_rayShader);
    const glm::mat4& view = m_views[m_currentViewIndex].viewMatrix;
    
    // Create a local projection matrix with Near=0.05f to ensure the ray is visible
    // This avoids messing with the global projection matrix that the game engine relies on
    float left = tan(m_xrViews[m_currentViewIndex].fov.angleLeft);
    float right = tan(m_xrViews[m_currentViewIndex].fov.angleRight);
    float down = tan(m_xrViews[m_currentViewIndex].fov.angleDown);
    float up = tan(m_xrViews[m_currentViewIndex].fov.angleUp);
    float nearZ = 0.05f;
    float farZ = 100.0f; 
    
    glm::mat4 localProjection(0.0f);
    localProjection[0][0] = 2.0f / (right - left);
    localProjection[1][1] = 2.0f / (up - down);
    localProjection[2][0] = (right + left) / (right - left);
    localProjection[2][1] = (up + down) / (up - down);
    localProjection[2][2] = -(farZ + nearZ) / (farZ - nearZ);
    localProjection[2][3] = -1.0f;
    localProjection[3][2] = -(2.0f * farZ * nearZ) / (farZ - nearZ);

    glUniformMatrix4fv(glGetUniformLocation(m_rayShader, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(m_rayShader, "projection"), 1, GL_FALSE, &localProjection[0][0]);
    glUniform4f(glGetUniformLocation(m_rayShader, "color"), 1.0f, 0.0f, 0.0f, 1.0f);

    glBindVertexArray(m_rayVAO);
    glDrawArrays(GL_LINES, 0, 2);
    GLenum err = glGetError();
    if (err != GL_NO_ERROR && logger) {
        logger->error("VRManager: RenderLeftHouseRay glDrawArrays error: 0x{:x}", err);
    }
    glBindVertexArray(0);

    glLineWidth(1.0f);
    glEnable(GL_DEPTH_TEST);

    // If hit, draw a cursor at the intersection point
    if (m_leftRayHitHouse || m_leftRayHitGUI) {
        RenderDebugCircle(m_leftRayHitPos, 0.015f, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)); // Red cursor
    }
}

void VRManager::RenderDebugCircle(const glm::vec3& position, float radius, const glm::vec4& color) {
    if (m_currentViewIndex < 0 || m_currentViewIndex >= static_cast<int>(m_views.size())) return;

    InitDebugResources();
    if (m_debugVAO == 0 || m_debugShader == 0) return;

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(m_debugShader);
    const glm::mat4& view = m_views[m_currentViewIndex].viewMatrix;
    
    // Create a local projection matrix with Near=0.05f to ensure the controller is visible
    float left = tan(m_xrViews[m_currentViewIndex].fov.angleLeft);
    float right = tan(m_xrViews[m_currentViewIndex].fov.angleRight);
    float down = tan(m_xrViews[m_currentViewIndex].fov.angleDown);
    float up = tan(m_xrViews[m_currentViewIndex].fov.angleUp);
    float nearZ = 0.05f;
    float farZ = 100.0f;
    
    glm::mat4 localProjection(0.0f);
    localProjection[0][0] = 2.0f / (right - left);
    localProjection[1][1] = 2.0f / (up - down);
    localProjection[2][0] = (right + left) / (right - left);
    localProjection[2][1] = (up + down) / (up - down);
    localProjection[2][2] = -(farZ + nearZ) / (farZ - nearZ);
    localProjection[2][3] = -1.0f;
    localProjection[3][2] = -(2.0f * farZ * nearZ) / (farZ - nearZ);
    
    // Create model matrix: translate to position and scale by radius
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, glm::vec3(radius));

    glUniformMatrix4fv(glGetUniformLocation(m_debugShader, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(m_debugShader, "projection"), 1, GL_FALSE, &localProjection[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(m_debugShader, "model"), 1, GL_FALSE, &model[0][0]);
    glUniform4f(glGetUniformLocation(m_debugShader, "color"), color.r, color.g, color.b, color.a);

    glBindVertexArray(m_debugVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, m_debugVertexCount);
    GLenum err = glGetError();
    if (err != GL_NO_ERROR && logger) {
        logger->error("VRManager: RenderDebugCircle glDrawArrays error: 0x{:x}", err);
    }
    glBindVertexArray(0);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void VRManager::RenderDebugController() {
    if (m_currentViewIndex < 0 || m_currentViewIndex >= static_cast<int>(m_views.size())) {
        return;
    }

    static bool loggedRender = false;
    if (!loggedRender && logger) {
        logger->info("VRManager: Rendering Debug Controller...");
        loggedRender = true;
    }

    if (m_leftAimSpace == XR_NULL_HANDLE) {
        static bool loggedError = false;
        if (!loggedError && logger) {
            logger->error("VRManager: Cannot render debug controller - m_leftAimSpace is NULL");
            loggedError = true;
        }
        return;
    }

    XrSpaceLocation location = {XR_TYPE_SPACE_LOCATION};
    XrResult res = xrLocateSpace(m_leftAimSpace, m_appSpace, m_frameState.predictedDisplayTime, &location);
    
    if (XR_SUCCEEDED(res)) {
        if (location.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) {
            glm::vec3 pos = {location.pose.position.x, location.pose.position.y, location.pose.position.z};
            RenderDebugCircle(pos, 0.05f, glm::vec4(0.0f, 1.0f, 0.0f, 0.8f));
            
            static bool loggedPose = false;
            if (!loggedPose && logger) {
                // Calculate NDC to verify visibility
                const glm::mat4& view = m_views[m_currentViewIndex].viewMatrix;
                const glm::mat4& projection = m_views[m_currentViewIndex].projectionMatrix;
                glm::vec4 clipPos = projection * view * glm::vec4(pos, 1.0f);
                glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;
                
                logger->info("VRManager: Debug controller pose located at ({}, {}, {})", pos.x, pos.y, pos.z);
                logger->info("VRManager: Controller NDC Position: ({}, {}, {}), w={}", ndc.x, ndc.y, ndc.z, clipPos.w);
                if (ndc.x < -1 || ndc.x > 1 || ndc.y < -1 || ndc.y > 1) {
                    logger->warning("VRManager: Controller is OFF-SCREEN!");
                } else {
                    logger->info("VRManager: Controller is ON-SCREEN (Visible Range).");
                }
                if (ndc.z < -1 || ndc.z > 1) {
                    logger->warning("VRManager: Controller is CLIPPED (Z-range)!");
                } else {
                    logger->info("VRManager: Controller Z is VALID.");
                }
                loggedPose = true;
            }
        }
    } else {
        static bool loggedPoseError = false;
        if (!loggedPoseError && logger) {
            logger->error("VRManager: Failed to locate left controller pose for debug rendering (Result: {})", static_cast<int>(res));
            loggedPoseError = true;
        }
    }
}

void VRManager::RenderDialogueHUD() {
    if (m_dialogueText.empty()) return;

    static bool loggedHud = false;

    GLboolean prevBlend = GL_FALSE;
    GLboolean prevDepthTest = GL_FALSE;
    GLboolean prevCullFace = GL_FALSE;
    GLboolean prevScissor = GL_FALSE;
    glGetBooleanv(GL_BLEND, &prevBlend);
    glGetBooleanv(GL_DEPTH_TEST, &prevDepthTest);
    glGetBooleanv(GL_CULL_FACE, &prevCullFace);
    glGetBooleanv(GL_SCISSOR_TEST, &prevScissor);

    if (m_dialogueFontTexture == 0) return;
    
    // Use current view parameters
    glViewport(0, 0, m_views[m_currentViewIndex].width, m_views[m_currentViewIndex].height);
    glm::mat4 view = m_views[m_currentViewIndex].viewMatrix;

    float left = tan(m_xrViews[m_currentViewIndex].fov.angleLeft);
    float right = tan(m_xrViews[m_currentViewIndex].fov.angleRight);
    float down = tan(m_xrViews[m_currentViewIndex].fov.angleDown);
    float up = tan(m_xrViews[m_currentViewIndex].fov.angleUp);
    float nearZ = 0.05f;
    float farZ = 100.0f;
    
    glm::mat4 hudProjection(0.0f);
    hudProjection[0][0] = 2.0f / (right - left);
    hudProjection[1][1] = 2.0f / (up - down);
    hudProjection[2][0] = (right + left) / (right - left);
    hudProjection[2][1] = (up + down) / (up - down);
    hudProjection[2][2] = -(farZ + nearZ) / (farZ - nearZ);
    hudProjection[2][3] = -1.0f;
    hudProjection[3][2] = -(2.0f * farZ * nearZ) / (farZ - nearZ);

    // Head-locked position: 1.5m in front (matching standard overlay depth)
    glm::mat4 invView = glm::inverse(view);
    glm::vec3 camPos = glm::vec3(invView[3]);
    glm::vec3 camFwd = glm::vec3(invView * glm::vec4(0, 0, -1, 0));
    glm::vec3 camUp = glm::vec3(invView * glm::vec4(0, 1, 0, 0));
    
    // HUD Pos: 1.5m distance (same as game menu), slightly up (0.12m) to sit right above the dialogue options
    // "Lado a lado" interpreted as same depth plane, vertically stacked but close
    glm::vec3 hudPos = camPos + camFwd * 1.5f + camUp * 0.12f; 

    if (!loggedHud && logger) {
        glm::vec4 clipPos = hudProjection * view * glm::vec4(hudPos, 1.0f);
        glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;
        logger->info("VRManager: RenderDialogueHUD ativo. viewIndex={}, texId={}, texSize={}x{}, HUD NDC=({}, {}, {}), w={}",
                     m_currentViewIndex,
                     (unsigned int)m_dialogueFontTexture,
                     m_dialogueTextureWidth, m_dialogueTextureHeight,
                     ndc.x, ndc.y, ndc.z, clipPos.w);
        loggedHud = true;
    }

    // Background panel scale based on texture aspect ratio
    float aspect = (float)m_dialogueTextureWidth / (float)m_dialogueTextureHeight;
    float panelWidth = 0.9f; // Reduzido de 1.2f para ser mais discreto
    float panelHeight = panelWidth / aspect;

    glm::mat4 model = invView;
    model[3] = glm::vec4(hudPos, 1.0f);
    
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE); // Garantir que não estamos vendo o verso

    // 1. Render background (semi-transparent black) - usando o shader de debug
    InitDebugResources();
    if (m_debugShader != 0 && m_quadVAO != 0) {
        glUseProgram(m_debugShader);
        glUniformMatrix4fv(glGetUniformLocation(m_debugShader, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(m_debugShader, "projection"), 1, GL_FALSE, &hudProjection[0][0]);
        
        // Match rotation and position of text for the background
        glm::mat4 backgroundModel = glm::scale(model, glm::vec3(panelWidth + 0.02f, panelHeight + 0.02f, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(m_debugShader, "model"), 1, GL_FALSE, &backgroundModel[0][0]);
        glUniform4f(glGetUniformLocation(m_debugShader, "color"), 0.1f, 0.1f, 0.1f, 0.9f); 

        glBindVertexArray(m_quadVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        GLenum err = glGetError();
        if (err != GL_NO_ERROR && logger) {
            logger->error("VRManager: RenderDialogueHUD background gl error: 0x{:x}", err);
        }
    }

    // 2. Render text texture - usando o quad shader de overlay
    InitOverlayQuad(); 
    if (m_quadShader != 0 && m_quadVAO != 0) {
        glUseProgram(m_quadShader);
        glUniformMatrix4fv(glGetUniformLocation(m_quadShader, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(m_quadShader, "projection"), 1, GL_FALSE, &hudProjection[0][0]);
        
        glm::mat4 textModel = glm::scale(model, glm::vec3(panelWidth, panelHeight, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(m_quadShader, "model"), 1, GL_FALSE, &textModel[0][0]);
        glUniform4f(glGetUniformLocation(m_quadShader, "color"), 1.0f, 1.0f, 1.0f, 1.0f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_dialogueFontTexture);
        glUniform1i(glGetUniformLocation(m_quadShader, "screenTexture"), 0);

        glBindVertexArray(m_quadVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        GLenum err = glGetError();
        if (err != GL_NO_ERROR && logger) {
            logger->error("VRManager: RenderDialogueHUD text gl error: 0x{:x}", err);
        }
    }

    if (prevScissor) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    if (prevCullFace) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (prevDepthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (prevBlend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
}

void VRManager::RenderOverlay3D() {
    if (m_quadShader == 0 || m_overlayTexture == 0) return;

    // Save current states
    GLboolean prevBlend = glIsEnabled(GL_BLEND);
    GLboolean prevDepthTest = glIsEnabled(GL_DEPTH_TEST);
    GLboolean prevCullFace = glIsEnabled(GL_CULL_FACE);
    GLboolean prevScissor = glIsEnabled(GL_SCISSOR_TEST);
    GLint prevPolygonMode[2];
    glGetIntegerv(GL_POLYGON_MODE, prevPolygonMode);
    GLint prevActiveTexture;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &prevActiveTexture);
    GLint prevTextureBinding;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTextureBinding);
    GLint prevProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
    GLint prevVAO;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVAO);
    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    // Save current global projection matrix if we are going to modify it
    // Actually, we use localProjection in most places, but the game engine 
    // relies on the projection matrix set in OpenGLRenderer.

    if (m_showGuiBillboard) {
        // Use current view parameters
        // In RenderOverlay3D, we are inside the VR Render Loop, so correct framebuffer is bound.
        
        // Set Viewport to match VR View Size
        glViewport(0, 0, m_views[m_currentViewIndex].width, m_views[m_currentViewIndex].height);

        // Get View from current VR View
        glm::mat4 view = m_views[m_currentViewIndex].viewMatrix;
        
        // Create a local projection matrix with Near=0.05f to ensure the billboard is visible
        // This avoids using the global projection matrix that might have a high Near plane
        float l = tan(m_xrViews[m_currentViewIndex].fov.angleLeft);
        float r = tan(m_xrViews[m_currentViewIndex].fov.angleRight);
        float d = tan(m_xrViews[m_currentViewIndex].fov.angleDown);
        float u = tan(m_xrViews[m_currentViewIndex].fov.angleUp);
        float nearZ = 0.05f;
        float farZ = 100.0f; 
        
        glm::mat4 localProjection(0.0f);
        localProjection[0][0] = 2.0f / (r - l);
        localProjection[1][1] = 2.0f / (u - d);
        localProjection[2][0] = (r + l) / (r - l);
        localProjection[2][1] = (u + d) / (u - d);
        localProjection[2][2] = -(farZ + nearZ) / (farZ - nearZ);
        localProjection[2][3] = -1.0f;
        localProjection[3][2] = -(2.0f * farZ * nearZ) / (farZ - nearZ);

        // Calculate Model Matrix - World Locked
        if (!m_guiBillboardPoseInitialized) {
            // Capture head position ONCE when the billboard is first shown
            glm::mat4 invView = glm::inverse(view);
            glm::vec3 camPos = glm::vec3(invView[3]);
            glm::vec3 camFwd = glm::vec3(invView * glm::vec4(0, 0, -1, 0));
            
            m_guiBillboardWorldPos = camPos + camFwd * 1.5f;
            m_guiBillboardWorldRot = glm::quat_cast(invView);
            m_guiBillboardPoseInitialized = true;
            
            if (logger) {
                logger->info("VRManager: GUI Billboard Position Initialized at ({}, {}, {})", 
                             m_guiBillboardWorldPos.x, m_guiBillboardWorldPos.y, m_guiBillboardWorldPos.z);
            }
        }

        // Construct Model Matrix from saved world pose
        glm::mat4 model = glm::mat4_cast(m_guiBillboardWorldRot);
        model[3] = glm::vec4(m_guiBillboardWorldPos, 1.0f);
        // Scaling to match a 4:3 aspect ratio (1.2m width, 0.9m height)
        // Note: Quad vertices are +/- 0.5 (width 1.0) and +/- 0.375 (height 0.75)
        // To get 1.2m width: scale by 1.2. To get 0.9m height: 0.75 * 1.2 = 0.9.
        model = glm::scale(model, glm::vec3(1.2f, 1.2f, 1.0f));

        // Render State Setup
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE); // Important: Ensure we see the quad regardless of winding

        glUseProgram(m_quadShader);

        glUniformMatrix4fv(glGetUniformLocation(m_quadShader, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(m_quadShader, "projection"), 1, GL_FALSE, &localProjection[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(m_quadShader, "model"), 1, GL_FALSE, &model[0][0]);
        glUniform4f(glGetUniformLocation(m_quadShader, "color"), 1.0f, 1.0f, 1.0f, 1.0f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_overlayTexture);
        glUniform1i(glGetUniformLocation(m_quadShader, "screenTexture"), 0);

        glBindVertexArray(m_quadVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        GLenum err = glGetError();
        if (err != GL_NO_ERROR && logger) {
            logger->error("VRManager: RenderOverlay3D billboard gl error: 0x{:x}", err);
        }
    }

    // Debug House Indicator: Draw the monitor screen anchored in the world
    if (m_debugHouseIndicator) {
        int w = m_views[m_currentViewIndex].width;
        int h = m_views[m_currentViewIndex].height;

        if (!m_housePoseInitialized) {
            // Capture head position ONCE when entering the house
            glm::mat4 invView = glm::inverse(m_views[m_currentViewIndex].viewMatrix);
            glm::vec3 camPos = glm::vec3(invView[3]);
            glm::vec3 camFwd = glm::vec3(invView * glm::vec4(0, 0, -1, 0));
            
            m_houseOverlayWorldPos = camPos + camFwd * 2.5f;
            m_houseOverlayWorldRot = glm::quat_cast(invView);
            
            m_houseOverlayScreenW = (int)(w / 2.2f);
            m_houseOverlayScreenH = (int)(m_houseOverlayScreenW / 1.333f);

            // Calculate Physical Dimensions at 2.5m distance
            // Total Frustum Width at Distance D = D * (tan(Right) - tan(Left))
            float dist = 2.5f;
            float tanLeft = tan(m_xrViews[m_currentViewIndex].fov.angleLeft);
            float tanRight = tan(m_xrViews[m_currentViewIndex].fov.angleRight);
            float tanDown = tan(m_xrViews[m_currentViewIndex].fov.angleDown);
            float tanUp = tan(m_xrViews[m_currentViewIndex].fov.angleUp);
            
            float frustumWidthAtDist = dist * (tanRight - tanLeft);
            float frustumHeightAtDist = dist * (tanUp - tanDown);
            
            m_houseOverlayWidthMeters = frustumWidthAtDist * ((float)m_houseOverlayScreenW / (float)w);
            m_houseOverlayHeightMeters = frustumHeightAtDist * ((float)m_houseOverlayScreenH / (float)h);
            
            if (logger) {
                 logger->info("VRManager: House Overlay Init. Size in Meters: {} x {}", m_houseOverlayWidthMeters, m_houseOverlayHeightMeters);
            }

            m_housePoseInitialized = true;
        }

        // Project the world position to the current eye's screen space
        glm::mat4 view = m_views[m_currentViewIndex].viewMatrix;
        glm::mat4 projection = m_views[m_currentViewIndex].projectionMatrix;
        
        glm::vec4 eyePos = view * glm::vec4(m_houseOverlayWorldPos, 1.0f);
        
        // If the object is behind the camera, don't render it
        if (eyePos.z < 0.0f) {
            glm::vec4 ndcPos = projection * eyePos;
            ndcPos /= ndcPos.w;

            // Convert NDC to pixel coordinates
            int pixelCenterH = (int)((ndcPos.x + 1.0f) * 0.5f * w);
            int pixelCenterV = (int)((ndcPos.y + 1.0f) * 0.5f * h);

            glEnable(GL_SCISSOR_TEST);
            int screenW = m_houseOverlayScreenW;
            int screenH = m_houseOverlayScreenH;

            glScissor(pixelCenterH - screenW / 2, pixelCenterV - screenH / 2, screenW, screenH);
            
            if (m_quadShader != 0 && m_overlayTexture != 0) {
                glDisable(GL_DEPTH_TEST);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glUseProgram(m_quadShader);
                
                glm::mat4 identity = glm::mat4(1.0f);
                glm::mat4 ortho = glm::ortho(0.0f, (float)w, 0.0f, (float)h);
                glm::mat4 model = glm::translate(identity, glm::vec3((float)pixelCenterH, (float)pixelCenterV, 0.0f));
                model = glm::scale(model, glm::vec3((float)screenW, (float)screenH, 1.0f));
                
                glUniformMatrix4fv(glGetUniformLocation(m_quadShader, "view"), 1, GL_FALSE, &identity[0][0]);
                glUniformMatrix4fv(glGetUniformLocation(m_quadShader, "projection"), 1, GL_FALSE, &ortho[0][0]);
                glUniformMatrix4fv(glGetUniformLocation(m_quadShader, "model"), 1, GL_FALSE, &model[0][0]);
                
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, m_overlayTexture);
                glUniform1i(glGetUniformLocation(m_quadShader, "screenTexture"), 0);
                
                glBindVertexArray(m_quadVAO);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            } else {
                glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
            }

            glDisable(GL_SCISSOR_TEST);
        }
    }

    if (m_debugHouseIndicator || m_showGuiBillboard) {
        static bool loggedRayRender = false;
        if (!loggedRayRender && logger) {
            logger->info("VRManager: Ray active (House: {}, GUI: {}). Calling Ray and Controller renderers.", 
                         m_debugHouseIndicator, m_showGuiBillboard);
            loggedRayRender = true;
        }
        RenderLeftHouseRay();
        RenderDebugController();
    }

    // Restore State
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    glPolygonMode(GL_FRONT_AND_BACK, prevPolygonMode[0]);
    glActiveTexture(prevActiveTexture);
    glBindTexture(GL_TEXTURE_2D, prevTextureBinding);
    glUseProgram(prevProgram);
    glBindVertexArray(prevVAO);

    if (prevScissor) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    if (prevCullFace) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (prevDepthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (prevBlend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
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

    XrReferenceSpaceCreateInfo viewSpaceInfo = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    viewSpaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    viewSpaceInfo.poseInReferenceSpace.orientation.w = 1.0f;
    if (!xrCheck(m_instance, xrCreateReferenceSpace(m_session, &viewSpaceInfo, &m_viewSpace), "xrCreateReferenceSpace(XR_REFERENCE_SPACE_TYPE_VIEW)"))
        return false;

    if (m_menuActionSet == XR_NULL_HANDLE) {
        xrStringToPath(m_instance, "/user/hand/left", &m_handLeftPath);
        xrStringToPath(m_instance, "/user/hand/right", &m_handRightPath);

        // We can keep the menu action set if we want to add specific menu actions later,
        // but for now we will rely on gameplay actions (move/trigger) even in menu.
        // Or we can remove it entirely if unused.
        // Let's keep it but empty for future use, or just remove the creation if it has no actions.

        // Actually, let's just skip creating m_menuActionSet actions since we removed them.

        // Gameplay Action Set
        XrActionSetCreateInfo gameplaySetInfo = {XR_TYPE_ACTION_SET_CREATE_INFO};
        std::strncpy(gameplaySetInfo.actionSetName, "gameplay", XR_MAX_ACTION_SET_NAME_SIZE - 1);
        std::strncpy(gameplaySetInfo.localizedActionSetName, "Gameplay", XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE - 1);
        gameplaySetInfo.priority = 0;
        xrCheck(m_instance, xrCreateActionSet(m_instance, &gameplaySetInfo, &m_gameplayActionSet), "xrCreateActionSet(gameplay)");

        if (m_gameplayActionSet != XR_NULL_HANDLE) {
            auto createAction = [&](const char* name, const char* locName, XrActionType type, XrAction* action) {
                XrActionCreateInfo info = {XR_TYPE_ACTION_CREATE_INFO};
                info.actionType = type;
                std::strncpy(info.actionName, name, XR_MAX_ACTION_NAME_SIZE - 1);
                std::strncpy(info.localizedActionName, locName, XR_MAX_LOCALIZED_ACTION_NAME_SIZE - 1);
                xrCheck(m_instance, xrCreateAction(m_gameplayActionSet, &info, action), name);
            };

            createAction("move", "Move", XR_ACTION_TYPE_VECTOR2F_INPUT, &m_actionMove);
            createAction("turn", "Turn", XR_ACTION_TYPE_VECTOR2F_INPUT, &m_actionTurn);
            createAction("attack", "Attack", XR_ACTION_TYPE_BOOLEAN_INPUT, &m_actionAttack);
            createAction("cast_ready", "Cast Ready", XR_ACTION_TYPE_BOOLEAN_INPUT, &m_actionCastReady);
            createAction("interact", "Interact", XR_ACTION_TYPE_FLOAT_INPUT, &m_actionInteract);
            createAction("esc", "Escape", XR_ACTION_TYPE_FLOAT_INPUT, &m_actionEsc);
            createAction("combat", "Combat", XR_ACTION_TYPE_BOOLEAN_INPUT, &m_actionCombat);
            createAction("cast", "Cast", XR_ACTION_TYPE_BOOLEAN_INPUT, &m_actionCast);
            createAction("fly_up", "Fly Up", XR_ACTION_TYPE_FLOAT_INPUT, &m_actionFlyUp);
            createAction("fly_down", "Fly Down", XR_ACTION_TYPE_FLOAT_INPUT, &m_actionFlyDown);
            createAction("quest", "Quest", XR_ACTION_TYPE_BOOLEAN_INPUT, &m_actionQuest);
            createAction("pass", "Pass", XR_ACTION_TYPE_BOOLEAN_INPUT, &m_actionPass);
            createAction("left_aim_pose", "Left Aim Pose", XR_ACTION_TYPE_POSE_INPUT, &m_actionLeftAimPose);

            if (m_actionLeftAimPose != XR_NULL_HANDLE) {
                XrActionSpaceCreateInfo actionSpaceInfo = {XR_TYPE_ACTION_SPACE_CREATE_INFO};
                actionSpaceInfo.action = m_actionLeftAimPose;
                actionSpaceInfo.subactionPath = m_handLeftPath;
                actionSpaceInfo.poseInActionSpace.orientation.w = 1.0f;
                xrCheck(m_instance, xrCreateActionSpace(m_session, &actionSpaceInfo, &m_leftAimSpace), "xrCreateActionSpace(left_aim_pose)");
                if (m_leftAimSpace != XR_NULL_HANDLE && logger) {
                    logger->info("VRManager: Created m_leftAimSpace for left controller.");
                } else if (logger) {
                    logger->error("VRManager: FAILED to create m_leftAimSpace!");
                }
            }

            // Bindings
            auto suggest = [&](const char* profile, std::vector<std::pair<XrAction, const char*>> bindings) {
                XrPath profilePath = XR_NULL_PATH;
                if (XR_FAILED(xrStringToPath(m_instance, profile, &profilePath))) return;

                std::vector<XrActionSuggestedBinding> suggestedBindings;
                for (auto& b : bindings) {
                    XrPath path = XR_NULL_PATH;
                    if (XR_FAILED(xrStringToPath(m_instance, b.second, &path))) continue;
                    suggestedBindings.push_back({b.first, path});
                }

                if (suggestedBindings.empty()) return;

                XrInteractionProfileSuggestedBinding suggested = {XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
                suggested.interactionProfile = profilePath;
                suggested.suggestedBindings = suggestedBindings.data();
                suggested.countSuggestedBindings = (uint32_t)suggestedBindings.size();
                xrSuggestInteractionProfileBindings(m_instance, &suggested);
            };

            // Oculus Touch (Quest 3)
            suggest("/interaction_profiles/oculus/touch_controller", {
                {m_actionMove, "/user/hand/left/input/thumbstick"},
                {m_actionTurn, "/user/hand/right/input/thumbstick"},
                {m_actionAttack, "/user/hand/right/input/b/click"},
                {m_actionCastReady, "/user/hand/right/input/a/click"},
                {m_actionInteract, "/user/hand/right/input/trigger/value"}, // Using value as bool (threshold)
                {m_actionEsc, "/user/hand/left/input/trigger/value"},
                {m_actionCombat, "/user/hand/left/input/y/click"},
                {m_actionCast, "/user/hand/left/input/x/click"},
                {m_actionFlyUp, "/user/hand/right/input/squeeze/value"},
                {m_actionFlyDown, "/user/hand/left/input/squeeze/value"},
                {m_actionQuest, "/user/hand/left/input/thumbstick/click"},
                {m_actionPass, "/user/hand/right/input/thumbstick/click"},
                {m_actionLeftAimPose, "/user/hand/left/input/aim/pose"}
            });
        }

        std::vector<XrActionSet> actionSets;
        if (m_menuActionSet != XR_NULL_HANDLE) actionSets.push_back(m_menuActionSet);
        if (m_gameplayActionSet != XR_NULL_HANDLE) actionSets.push_back(m_gameplayActionSet);

        if (!actionSets.empty()) {
            XrSessionActionSetsAttachInfo attachInfo = {XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
            attachInfo.countActionSets = (uint32_t)actionSets.size();
            attachInfo.actionSets = actionSets.data();
            xrCheck(m_instance, xrAttachSessionActionSets(m_session, &attachInfo), "xrAttachSessionActionSets");
        }

        if (m_menuActionSet != XR_NULL_HANDLE) {
            // No action spaces to create for menu currently
        }
    }

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

    int64_t swapchainFormat = SelectColorSwapchainFormat(m_session);
    if (logger) {
        logger->info("VRManager: Main Swapchain format selected: {}", swapchainFormat);
    }

    for (uint32_t i = 0; i < viewCount; i++) {
        // Create Swapchain
        XrSwapchainCreateInfo swapchainInfo = {XR_TYPE_SWAPCHAIN_CREATE_INFO};
        swapchainInfo.arraySize = 1;
        swapchainInfo.format = swapchainFormat; // Simple format
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

    // Process Menu Input
    // Note: Dialogue menu navigation is now handled in GetMenuMouseState to avoid duplication
    /*
    if (!m_dialogueOptions.empty()) {
        // ... removed duplicate logic ...
    }
    */
}

bool VRManager::BeginFrame() {
    PollEvents();
    if (!m_sessionRunning) return false;

    XrFrameWaitInfo waitInfo = {XR_TYPE_FRAME_WAIT_INFO};
    m_frameState = {XR_TYPE_FRAME_STATE};
    if (XR_FAILED(xrWaitFrame(m_session, &waitInfo, &m_frameState))) return false;

    XrFrameBeginInfo beginInfo = {XR_TYPE_FRAME_BEGIN_INFO};
    if (XR_FAILED(xrBeginFrame(m_session, &beginInfo))) return false;

    m_frameBegun = true;
    m_shouldRenderThisFrame = m_frameState.shouldRender;

    if (m_sessionState == XR_SESSION_STATE_FOCUSED || m_sessionState == XR_SESSION_STATE_VISIBLE) {
        std::vector<XrActiveActionSet> activeSets;

        // We removed the menu action set for now, relying on gameplay actions.
        /*
        if (m_menuActionSet != XR_NULL_HANDLE) {
            XrActiveActionSet activeSet = {m_menuActionSet, XR_NULL_PATH};
            activeSets.push_back(activeSet);
        }
        */

        if (m_gameplayActionSet != XR_NULL_HANDLE) {
            XrActiveActionSet activeSet = {m_gameplayActionSet, XR_NULL_PATH};
            activeSets.push_back(activeSet);
        }

        if (!activeSets.empty()) {
            XrActionsSyncInfo syncInfo = {XR_TYPE_ACTIONS_SYNC_INFO};
            syncInfo.countActiveActionSets = (uint32_t)activeSets.size();
            syncInfo.activeActionSets = activeSets.data();
            xrSyncActions(m_session, &syncInfo);
        }

        // Toggle GUI Billboard with X button
        if (m_actionCast != XR_NULL_HANDLE) {
            XrActionStateGetInfo getInfo = {XR_TYPE_ACTION_STATE_GET_INFO};
            getInfo.action = m_actionCast;
            XrActionStateBoolean boolState = {XR_TYPE_ACTION_STATE_BOOLEAN};
            if (XR_SUCCEEDED(xrGetActionStateBoolean(m_session, &getInfo, &boolState))) {
                if (boolState.isActive && boolState.currentState) {
                    if (!m_guiBillboardButtonPressedPrev) {
                        m_showGuiBillboard = !m_showGuiBillboard;
                        if (m_showGuiBillboard) {
                            m_waitForTriggerRelease = true; // Prevent accidental clicks when opening
                        }
                        if (logger) {
                            logger->info("VRManager: GUI Billboard toggled to {}", m_showGuiBillboard);
                        }
                    }
                    m_guiBillboardButtonPressedPrev = true;
                } else {
                    m_guiBillboardButtonPressedPrev = false;
                }
            }
        }

        // Toggle Minimal HUD with BOTH Triggers (Left + Right)
        if (m_actionEsc != XR_NULL_HANDLE && m_actionInteract != XR_NULL_HANDLE) {
            bool leftPressed = false;
            bool rightPressed = false;

            // Check Left Trigger (Esc)
            XrActionStateGetInfo getInfoL = {XR_TYPE_ACTION_STATE_GET_INFO};
            getInfoL.action = m_actionEsc;
            XrActionStateFloat floatStateL = {XR_TYPE_ACTION_STATE_FLOAT};
            if (XR_SUCCEEDED(xrGetActionStateFloat(m_session, &getInfoL, &floatStateL))) {
                if (floatStateL.isActive && floatStateL.currentState > 0.8f) { // High threshold for deliberate press
                    leftPressed = true;
                }
            }

            // Check Right Trigger (Interact)
            XrActionStateGetInfo getInfoR = {XR_TYPE_ACTION_STATE_GET_INFO};
            getInfoR.action = m_actionInteract;
            XrActionStateFloat floatStateR = {XR_TYPE_ACTION_STATE_FLOAT};
            if (XR_SUCCEEDED(xrGetActionStateFloat(m_session, &getInfoR, &floatStateR))) {
                if (floatStateR.isActive && floatStateR.currentState > 0.8f) {
                    rightPressed = true;
                }
            }

            if (leftPressed && rightPressed) {
                if (!m_minimalHUDTogglePressedPrev) {
                    m_showMinimalHUD = !m_showMinimalHUD;
                    if (logger) {
                        logger->info("VRManager: Minimal HUD toggled to {}", m_showMinimalHUD);
                    }
                }
                m_minimalHUDTogglePressedPrev = true;
            } else {
                m_minimalHUDTogglePressedPrev = false;
            }
        }
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
    if (XR_FAILED(xrLocateViews(m_session, &locateInfo, &viewState, (uint32_t)m_xrViews.size(), &viewCountOutput, m_xrViews.data()))) {
        m_shouldRenderThisFrame = false;
        EndFrame();
        return false;
    }

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

    if (m_overlayLayerEnabled && !m_overlayLayerAnchorPoseValid && viewCountOutput > 0) {
        glm::vec3 p0(m_xrViews[0].pose.position.x, m_xrViews[0].pose.position.y, m_xrViews[0].pose.position.z);
        glm::vec3 center = p0;
        if (viewCountOutput > 1) {
            glm::vec3 p1(m_xrViews[1].pose.position.x, m_xrViews[1].pose.position.y, m_xrViews[1].pose.position.z);
            center = (p0 + p1) * 0.5f;
        }

        glm::quat ori(m_xrViews[0].pose.orientation.w, m_xrViews[0].pose.orientation.x, m_xrViews[0].pose.orientation.y, m_xrViews[0].pose.orientation.z);
        glm::vec3 forward = glm::normalize(ori * glm::vec3(0.0f, 0.0f, -1.0f));
        glm::vec3 quadPos = center + forward * 1.5f;

        m_overlayLayerAnchorPose.orientation = m_xrViews[0].pose.orientation;
        m_overlayLayerAnchorPose.position.x = quadPos.x;
        m_overlayLayerAnchorPose.position.y = quadPos.y;
        m_overlayLayerAnchorPose.position.z = quadPos.z;
        m_overlayLayerAnchorPoseValid = true;
    }

    return true;
}

void VRManager::EndFrame() {
    if (!m_frameBegun) return;

    XrCompositionLayerProjection projectionLayer = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    projectionLayer.space = m_appSpace;
    projectionLayer.viewCount = (uint32_t)m_projectionViews.size();
    projectionLayer.views = m_projectionViews.data();

    XrCompositionLayerQuad quadLayer = {XR_TYPE_COMPOSITION_LAYER_QUAD};
    std::array<const XrCompositionLayerBaseHeader*, 2> layers = { (const XrCompositionLayerBaseHeader*)&projectionLayer, nullptr };
    uint32_t layerCount = 1;
    if (!m_shouldRenderThisFrame) {
        layerCount = 0;
    } else if (m_overlayLayerEnabled && m_overlayLayerHasFrame && m_overlayLayerSwapchain != XR_NULL_HANDLE) {
        quadLayer.layerFlags = 0;
        quadLayer.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
        if (m_overlayLayerAnchorPoseValid && m_appSpace != XR_NULL_HANDLE) {
            quadLayer.space = m_appSpace;
            quadLayer.pose = m_overlayLayerAnchorPose;
        } else if (m_viewSpace != XR_NULL_HANDLE) {
            quadLayer.space = m_viewSpace;
            quadLayer.pose.orientation.w = 1.0f;
            quadLayer.pose.position.x = 0.0f;
            quadLayer.pose.position.y = 0.0f;
            quadLayer.pose.position.z = -1.5f;
        } else {
            quadLayer.space = m_appSpace;
            quadLayer.pose.orientation.w = 1.0f;
            quadLayer.pose.position.x = 0.0f;
            quadLayer.pose.position.y = 0.0f;
            quadLayer.pose.position.z = 0.0f;
        }
        quadLayer.size.width = 1.0f;
        quadLayer.size.height = 0.75f;
        quadLayer.subImage.swapchain = m_overlayLayerSwapchain;
        quadLayer.subImage.imageRect.offset = {0, 0};
        quadLayer.subImage.imageRect.extent = {m_overlayLayerWidth, m_overlayLayerHeight};
        quadLayer.subImage.imageArrayIndex = 0;
        layers[layerCount++] = (const XrCompositionLayerBaseHeader*)&quadLayer;
    }

    XrFrameEndInfo endInfo = {XR_TYPE_FRAME_END_INFO};
    endInfo.displayTime = m_frameState.predictedDisplayTime;
    endInfo.environmentBlendMode = m_environmentBlendMode;
    endInfo.layerCount = layerCount;
    endInfo.layers = (layerCount > 0) ? layers.data() : nullptr;

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

    m_frameBegun = false;
    m_shouldRenderThisFrame = false;
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

    // Disable sRGB conversion for writing to swapchain (we write raw sRGB values)
    glDisable(GL_FRAMEBUFFER_SRGB);

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
    /*
    if (m_menuAimSpaceRight != XR_NULL_HANDLE) {
        xrDestroySpace(m_menuAimSpaceRight);
        m_menuAimSpaceRight = XR_NULL_HANDLE;
    }
    if (m_menuActionSet != XR_NULL_HANDLE) {
        xrDestroyActionSet(m_menuActionSet);
        m_menuActionSet = XR_NULL_HANDLE;
        m_menuAimPoseAction = XR_NULL_HANDLE;
        m_menuSelectClickAction = XR_NULL_HANDLE;
        m_menuSelectValueAction = XR_NULL_HANDLE;
    }
    */

    if (m_gameplayActionSet != XR_NULL_HANDLE) {
        xrDestroyActionSet(m_gameplayActionSet);
        m_gameplayActionSet = XR_NULL_HANDLE;
        m_actionMove = XR_NULL_HANDLE;
        m_actionTurn = XR_NULL_HANDLE;
        m_actionAttack = XR_NULL_HANDLE;
        m_actionCastReady = XR_NULL_HANDLE;
        m_actionInteract = XR_NULL_HANDLE;
        m_actionEsc = XR_NULL_HANDLE;
        m_actionCombat = XR_NULL_HANDLE;
        m_actionCast = XR_NULL_HANDLE;
        m_actionFlyUp = XR_NULL_HANDLE;
        m_actionFlyDown = XR_NULL_HANDLE;
        m_actionQuest = XR_NULL_HANDLE;
        m_actionPass = XR_NULL_HANDLE;
    }

    if (m_overlayLayerSwapchain != XR_NULL_HANDLE) {
        xrDestroySwapchain(m_overlayLayerSwapchain);
        m_overlayLayerSwapchain = XR_NULL_HANDLE;
    }
    m_overlayLayerImages.clear();
    m_overlayLayerHasFrame = false;
    m_overlayLayerAnchorPoseValid = false;
    if (m_overlayLayerFBO != 0) {
        glDeleteFramebuffers(1, &m_overlayLayerFBO);
        m_overlayLayerFBO = 0;
    }

    if (m_leftAimSpace != XR_NULL_HANDLE) {
        xrDestroySpace(m_leftAimSpace);
        m_leftAimSpace = XR_NULL_HANDLE;
    }
    if (m_viewSpace != XR_NULL_HANDLE) {
        xrDestroySpace(m_viewSpace);
        m_viewSpace = XR_NULL_HANDLE;
    }
    if (m_appSpace != XR_NULL_HANDLE) {
        xrDestroySpace(m_appSpace);
        m_appSpace = XR_NULL_HANDLE;
    }
    if (m_session != XR_NULL_HANDLE) {
        xrDestroySession(m_session);
        m_session = XR_NULL_HANDLE;
    }
    if (m_instance != XR_NULL_HANDLE) {
        xrDestroyInstance(m_instance);
        m_instance = XR_NULL_HANDLE;
    }

    if (m_dialogueFontTexture != 0) {
        glDeleteTextures(1, &m_dialogueFontTexture);
        m_dialogueFontTexture = 0;
    }
}

bool VRManager::GetMenuMouseState(int menuWidth, int menuHeight, int& outX, int& outY, bool& outLeftDown, bool& outRightDown) {
    static bool loggedCall = false;
    if (!loggedCall && logger) {
        logger->info("VRManager: GetMenuMouseState called.");
        loggedCall = true;
    }

    outX = 0;
    outY = 0;
    outLeftDown = false;
    outRightDown = false;

    const bool canUseOverlayLayer = m_overlayLayerEnabled && m_overlayLayerHasFrame;
    const bool canUseHouseOverlay = m_debugHouseIndicator;
    const bool canUseGuiBillboard = m_showGuiBillboard;
    const bool hasDialogue = !m_dialogueOptions.empty();

    if (!canUseOverlayLayer && !canUseHouseOverlay && !hasDialogue && !canUseGuiBillboard) {
        return false;
    }
    if (m_session == XR_NULL_HANDLE || !m_sessionRunning)
    {
        return false;
    }
    
    if (menuWidth <= 0 || menuHeight <= 0)
    {
        return false;
    }

    // Initialize cursor to center if first time
    if (!m_menuCursorInitialized) {
        m_menuCursorX = (float)menuWidth * 0.5f;
        m_menuCursorY = (float)menuHeight * 0.5f;
        m_menuCursorInitialized = true;
    }

    if (m_sessionState == XR_SESSION_STATE_FOCUSED || m_sessionState == XR_SESSION_STATE_VISIBLE) {
        if (m_gameplayActionSet != XR_NULL_HANDLE) {
            XrActiveActionSet activeSet = {m_gameplayActionSet, XR_NULL_PATH};
            XrActionsSyncInfo syncInfo = {XR_TYPE_ACTIONS_SYNC_INFO};
            syncInfo.countActiveActionSets = 1;
            syncInfo.activeActionSets = &activeSet;
            xrSyncActions(m_session, &syncInfo);
        }
    }

    // Get input from Left Stick (m_actionMove) and Left/Right Triggers (m_actionEsc/m_actionInteract)
    XrActionStateVector2f moveState = {XR_TYPE_ACTION_STATE_VECTOR2F};
    if (m_actionMove != XR_NULL_HANDLE) {
        XrActionStateGetInfo getInfo = {XR_TYPE_ACTION_STATE_GET_INFO};
        getInfo.action = m_actionMove;
        getInfo.subactionPath = XR_NULL_PATH;
        xrGetActionStateVector2f(m_session, &getInfo, &moveState);
    }

    XrActionStateFloat triggerLeftState = {XR_TYPE_ACTION_STATE_FLOAT};
    if (m_actionEsc != XR_NULL_HANDLE) {
        XrActionStateGetInfo getInfo = {XR_TYPE_ACTION_STATE_GET_INFO};
        getInfo.action = m_actionEsc;
        getInfo.subactionPath = XR_NULL_PATH;
        xrGetActionStateFloat(m_session, &getInfo, &triggerLeftState);
    }

    XrActionStateFloat triggerRightState = {XR_TYPE_ACTION_STATE_FLOAT};
    if (m_actionInteract != XR_NULL_HANDLE) {
        XrActionStateGetInfo getInfo = {XR_TYPE_ACTION_STATE_GET_INFO};
        getInfo.action = m_actionInteract;
        getInfo.subactionPath = XR_NULL_PATH;
        xrGetActionStateFloat(m_session, &getInfo, &triggerRightState);
    }

    // Determine Button States
    bool leftDown = (triggerLeftState.isActive && triggerLeftState.currentState > 0.5f);
    bool rightDown = (triggerRightState.isActive && triggerRightState.currentState > 0.5f);
    
    // Output states
    outLeftDown = leftDown;
    outRightDown = rightDown;

    // --- GUI Billboard Mode (House/Shop 2D Overlay) ---
    if (m_showGuiBillboard) {
        // In this mode, we enforce Raycast for Cursor Position and map Triggers directly to Mouse Buttons.
        // We explicitly IGNORE list navigation logic (Stick Up/Down) as per user request.
        
        if (m_leftRayHitGUI) {
             m_menuCursorX = m_guiHitX * menuWidth;
             m_menuCursorY = m_guiHitY * menuHeight;
             
             if (logger) {
                static bool loggedGuiRayHit = false;
                if (!loggedGuiRayHit) {
                    logger->info("VRManager: Ray hit GUI Billboard. Cursor set to X: {}, Y: {}", m_menuCursorX, m_menuCursorY);
                    loggedGuiRayHit = true;
                }
            }
        }
        
        // Output coordinates
        outX = static_cast<int>(m_menuCursorX);
        outY = static_cast<int>(m_menuCursorY);
        return true;
    }

    // --- Standard Dialogue/Menu Mode ---

    // Update Cursor Position (Standard)
    if (m_leftRayHitHouse) {
        m_menuCursorX = m_houseHitX * menuWidth;
        m_menuCursorY = m_houseHitY * menuHeight;
    } else if (moveState.isActive) {
        // Move X
        m_menuCursorX += moveState.currentState.x * m_menuCursorSpeed;

        // Move Y (Invert Stick Y because screen Y is down)
        m_menuCursorY -= moveState.currentState.y * m_menuCursorSpeed;

        // Clamp
        m_menuCursorX = std::clamp(m_menuCursorX, 0.0f, (float)menuWidth - 1.0f);
        m_menuCursorY = std::clamp(m_menuCursorY, 0.0f, (float)menuHeight - 1.0f);
    }

    if (logger) {
        static int logCounter = 0;
        logCounter++;
        if (logCounter > 300) { // Log every ~5 seconds at 60fps
            logger->info("VRManager: Trigger states - Left: {} (Active: {}), Right: {} (Active: {})", 
                triggerLeftState.currentState, triggerLeftState.isActive, 
                triggerRightState.currentState, triggerRightState.isActive);
            logCounter = 0;
        }
    }

    // Handle directional selection for dialogue menu (Legacy/NPC Dialogue)
    if (!m_dialogueOptions.empty()) {
        static bool stickReset = true;
        float stickThreshold = 0.5f;

        // Try getting turn action state as well (Right Stick)
        XrActionStateVector2f turnState = {XR_TYPE_ACTION_STATE_VECTOR2F};
        if (m_actionTurn != XR_NULL_HANDLE) {
            XrActionStateGetInfo getInfo = {XR_TYPE_ACTION_STATE_GET_INFO};
            getInfo.action = m_actionTurn;
            getInfo.subactionPath = XR_NULL_PATH;
            xrGetActionStateVector2f(m_session, &getInfo, &turnState);
        }
        
        float inputY = 0.0f;
        if (moveState.isActive && std::abs(moveState.currentState.y) > 0.1f) {
            inputY = moveState.currentState.y;
        } else if (turnState.isActive && std::abs(turnState.currentState.y) > 0.1f) {
            inputY = turnState.currentState.y;
        }

        if (inputY != 0.0f) {
            if (stickReset) {
                if (inputY > stickThreshold) { // Up (Stick Up is positive Y in OpenXR)
                    int oldIndex = m_selectedOptionIndex;
                    m_selectedOptionIndex = (m_selectedOptionIndex > 0) ? m_selectedOptionIndex - 1 : (int)m_dialogueOptions.size() - 1;
                    UpdateDialogueMenuTexture();
                    stickReset = false;
                } else if (inputY < -stickThreshold) { // Down (Stick Down is negative Y in OpenXR)
                    int oldIndex = m_selectedOptionIndex;
                    m_selectedOptionIndex = (m_selectedOptionIndex < (int)m_dialogueOptions.size() - 1) ? m_selectedOptionIndex + 1 : 0;
                    UpdateDialogueMenuTexture();
                    stickReset = false;
                }
            } else {
                if (std::abs(inputY) < 0.25f) {
                    stickReset = true;
                }
            }
        } else {
            stickReset = true;
        }

        // Selection via Trigger (Pulse logic for dialogue)
        // Detect Click Pulse (Rising Edge) locally for Dialogue selection
        bool pressedNow = leftDown || rightDown; // Any trigger selects in dialogue? Or just left? 
        // Original code used both.
        
        bool clickPulse = pressedNow && !m_menuSelectPressedPrev;
        m_menuSelectPressedPrev = pressedNow;

        if (clickPulse) {
            if (m_selectedOptionIndex >= 0 && m_selectedOptionIndex < (int)m_dialogueOptions.size()) {
                const auto& opt = m_dialogueOptions[m_selectedOptionIndex];
                if (engine && engine->_messageQueue) {
                    engine->_messageQueue->addMessageCurrentFrame(static_cast<UIMessageType>(opt.msg), opt.id, 0);
                    if (logger) logger->info("VRManager: Option selected via directional/click: '{}' (ID: {}, Msg: {})", opt.text, opt.id, opt.msg);
                    
                    // Consume the click to prevent Mouse.cpp from triggering generic click
                    outLeftDown = false; 
                    outRightDown = false;
                }
            }
        }
    } else {
        // Update prev state if not in dialogue (to keep state consistent)
         bool pressedNow = leftDown || rightDown;
         m_menuSelectPressedPrev = pressedNow;
    }

    outX = static_cast<int>(m_menuCursorX);
    outY = static_cast<int>(m_menuCursorY);

    return true;
}

glm::mat4 VRManager::GetCurrentViewMatrix(const glm::vec3& worldOrigin, float yawRad, float pitchRad) {
    if (m_currentViewIndex < 0 || m_currentViewIndex >= m_views.size()) return glm::mat4(1.0f);
    
    // VR View Matrix (Headset -> Local Y-Up)
    // We reconstruct it here to apply world scale to the position
    const auto& pose = m_views[m_currentViewIndex].view.pose;
    glm::quat orientation(pose.orientation.w, pose.orientation.x, pose.orientation.y, pose.orientation.z);
    
    // Scale position: 1 meter (OpenXR) = 100 units (MM7)
    // We assume the game uses ~1 unit = 1 cm scale based on eyeLevel=160 (1.6m)
    constexpr float WORLD_SCALE = 100.0f;
    glm::vec3 position(pose.position.x * WORLD_SCALE, pose.position.y * WORLD_SCALE, pose.position.z * WORLD_SCALE);

    glm::mat4 rotation = glm::mat4_cast(orientation);
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 poseMat = translation * rotation;
    glm::mat4 vrView = glm::inverse(poseMat);
    
    // Conversion from Z-Up (OpenEnroth) to Y-Up (OpenXR)
    // Rotate -90 degrees around X axis
    // Z (0,0,1) -> Y (0,1,0)
    // Y (0,1,0) -> -Z (0,0,-1)
    glm::mat4 coordConvert = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1, 0, 0));
    
    // Game Rotation (Yaw)
    // We rotate the world around Z by -Yaw to match the camera's orientation.
    // We intentionally ignore pitch/roll from the game as the HMD provides those.
    // Note: We might need to adjust the phase of yaw (e.g. + PI/2) depending on 0-angle convention.
    // MM7 Yaw 0 looks at -X. OpenXR Forward is -Z.
    // Previous attempt with -PI/2 resulted in 180 degree flip (looking backward).
    // So we need to flip it by 180 degrees.
    // -PI/2 + PI = +PI/2.
    glm::mat4 gameRotation = glm::rotate(glm::mat4(1.0f), -yawRad + glm::half_pi<float>(), glm::vec3(0, 0, 1));
    
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

void VRManager::GetViewTangents(int viewIndex, float& l, float& r, float& u, float& d) {
    if (viewIndex >= 0 && viewIndex < m_xrViews.size()) {
        l = std::tan(m_xrViews[viewIndex].fov.angleLeft);
        r = std::tan(m_xrViews[viewIndex].fov.angleRight);
        d = std::tan(m_xrViews[viewIndex].fov.angleDown);
        u = std::tan(m_xrViews[viewIndex].fov.angleUp);
    } else {
        l = -1.0f; r = 1.0f; d = -1.0f; u = 1.0f; // Default dummy
    }
}

void VRManager::GetViewSize(int viewIndex, int& w, int& h) const {
    if (viewIndex >= 0 && viewIndex < m_views.size()) {
        w = m_views[viewIndex].width;
        h = m_views[viewIndex].height;
    } else {
        w = 0; h = 0;
    }
}

VRManager::VRInputState VRManager::GetVRInputState() {
    VRInputState state;
    if (m_session == XR_NULL_HANDLE || (m_sessionState != XR_SESSION_STATE_FOCUSED && m_sessionState != XR_SESSION_STATE_VISIBLE)) return state;

    if (m_gameplayActionSet != XR_NULL_HANDLE) {
        XrActiveActionSet activeSet = {m_gameplayActionSet, XR_NULL_PATH};
        XrActionsSyncInfo syncInfo = {XR_TYPE_ACTIONS_SYNC_INFO};
        syncInfo.countActiveActionSets = 1;
        syncInfo.activeActionSets = &activeSet;
        xrSyncActions(m_session, &syncInfo);
    }

    auto getActionBool = [&](XrAction action) {
        if (action == XR_NULL_HANDLE) return false;
        XrActionStateGetInfo getInfo = {XR_TYPE_ACTION_STATE_GET_INFO};
        getInfo.action = action;
        XrActionStateBoolean boolState = {XR_TYPE_ACTION_STATE_BOOLEAN};
        if (XR_FAILED(xrGetActionStateBoolean(m_session, &getInfo, &boolState))) return false;
        return boolState.isActive && (bool)boolState.currentState;
    };

    auto getActionVec2 = [&](XrAction action) {
        if (action == XR_NULL_HANDLE) return glm::vec2(0.0f);
        XrActionStateGetInfo getInfo = {XR_TYPE_ACTION_STATE_GET_INFO};
        getInfo.action = action;
        XrActionStateVector2f vec2State = {XR_TYPE_ACTION_STATE_VECTOR2F};
        if (XR_FAILED(xrGetActionStateVector2f(m_session, &getInfo, &vec2State))) return glm::vec2(0.0f);
        if (!vec2State.isActive) return glm::vec2(0.0f);
        return glm::vec2(vec2State.currentState.x, vec2State.currentState.y);
    };
    
    auto getActionFloatAsBool = [&](XrAction action, float threshold = 0.5f) {
        if (action == XR_NULL_HANDLE) return false;
        XrActionStateGetInfo getInfo = {XR_TYPE_ACTION_STATE_GET_INFO};
        getInfo.action = action;
        XrActionStateFloat floatState = {XR_TYPE_ACTION_STATE_FLOAT};
        if (XR_FAILED(xrGetActionStateFloat(m_session, &getInfo, &floatState))) return false;
        return floatState.isActive && floatState.currentState > threshold;
    };

    glm::vec2 move = getActionVec2(m_actionMove);
    state.moveX = move.x;
    state.moveY = move.y;
    glm::vec2 turn = getActionVec2(m_actionTurn);
    state.turnX = turn.x;
    state.turnY = turn.y;
    state.attack = getActionBool(m_actionAttack);
    state.castReady = getActionBool(m_actionCastReady);
    state.interact = getActionFloatAsBool(m_actionInteract, 0.5f);
    state.esc = getActionBool(m_actionCombat);
    state.combat = false;
    state.cast = getActionBool(m_actionCast);
    state.flyUp = getActionFloatAsBool(m_actionFlyUp, 0.5f);
    state.flyDown = getActionFloatAsBool(m_actionFlyDown, 0.5f);
    
    // Check for simultaneous grips to simulate "Enter" key
    if (state.flyUp && state.flyDown) {
        state.simultaneousGrips = true;
        // Suppress flying when both are pressed to avoid conflicting movement
        state.flyUp = false;
        state.flyDown = false;
    } else {
        state.simultaneousGrips = false;
    }

    state.quest = getActionBool(m_actionQuest);
    state.pass = getActionBool(m_actionPass);
    
    // Jump: Right Stick Up (Turn Y > 0.5)
    state.jump = state.turnY > 0.5f;

    // Calculate Head Pitch from first view (Left Eye/Head)
    if (!m_xrViews.empty()) {
        const auto& orient = m_xrViews[0].pose.orientation;
        glm::quat q(orient.w, orient.x, orient.y, orient.z);
        // OpenXR: Forward is -Z.
        // Get forward vector direction
        glm::vec3 forward = q * glm::vec3(0.0f, 0.0f, -1.0f);
        // Pitch is the angle of forward vector with the horizontal plane (XZ).
        // Since Y is Up, pitch is asin(forward.y).
        // forward.y > 0 means looking up (positive pitch).
        // forward.y < 0 means looking down (negative pitch).
        state.headPitch = std::asin(std::clamp(forward.y, -1.0f, 1.0f));
    } else {
        state.headPitch = 0.0f;
    }

    return state;
}

#include "Mouse.h"

#include <cstdlib>
#include <list>
#include <memory>

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/EngineIocContainer.h"
#include "Engine/Graphics/Viewport.h"
#include "Engine/Graphics/Vis.h"
#include "Engine/Graphics/Image.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Party.h"
#include "Engine/TurnEngine/TurnEngine.h"
#include "Engine/AssetsManager.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"
#include "GUI/GUIMessageQueue.h"
#include "GUI/UI/UIBranchlessDialogue.h"
#include "GUI/UI/UISpell.h"
#include "GUI/UI/UIGameOver.h"

#include "Media/Audio/AudioPlayer.h"

#include "Library/Logger/Logger.h"
#include "Engine/VR/VRManager.h"

std::shared_ptr<Io::Mouse> mouse = nullptr;

Pointi Io::Mouse::position() const {
    return _position;
}

void Io::Mouse::setPosition(Pointi position) {
        _position = position;
}

void Io::Mouse::SetCursorBitmapFromItemID(ItemId uItemID) {
    SetCursorImage(pItemTable->items[uItemID].iconName);
}

void Io::Mouse::SetCurrentCursorBitmap() { SetCursorImage(this->cursor_name); }

void Io::Mouse::SetCursorImage(std::string_view name) {
    if (!this->bInitialized) {
        return;
    }

    if (this->cursor_name != name)
        this->cursor_name = name;

    ClearCursor();
    if (name == "MICON1" && !engine->config->graphics.AlwaysCustomCursor.value()) { // Arrow cursor.
        this->_arrowCursor = true;
        platform->setCursorShown(true);
        this->cursor_img = nullptr;
    } else { // Cursor is item or another bitmap.
        this->cursor_img = assets->getImage_ColorKey(name, colorTable.Black /*colorTable.TealMask*/);
        this->AllocCursorSystemMem();
        this->_arrowCursor = false;
    }
}

void Io::Mouse::ClearCursor() {
    free(this->pCursorBitmap_sysmem);
    this->pCursorBitmap_sysmem = nullptr;
    free(this->pCursorBitmap2_sysmem);
    this->pCursorBitmap2_sysmem = nullptr;
}

void Io::Mouse::AllocCursorSystemMem() {
    if (!pCursorBitmap_sysmem)
        pCursorBitmap_sysmem = (uint16_t *)DoAllocCursorMem();
    if (!pCursorBitmap2_sysmem)
        pCursorBitmap2_sysmem = (uint8_t *)DoAllocCursorMem();
}

void *Io::Mouse::DoAllocCursorMem() { return nullptr; }

void Io::Mouse::Initialize() {
    this->bInitialized = true;

    // this->field_8 = 0;//Ritor1: result incorrect uMouseX,
    // this->uMouseY in _469AE4()

    this->pCursorBitmapPos.x = 0;
    this->pCursorBitmapPos.y = 0;
    this->pCursorBitmap_sysmem = nullptr;
    this->pCursorBitmap2_sysmem = nullptr;

    SetCursorImage("MICON3");
    SetCursorImage("MICON2");
    SetCursorImage("MICON1");
}

void Io::Mouse::DrawCursor() {
    if (VRManager::Get().IsInitialized()) {
        const auto dims = render->GetRenderDimensions();
        int vrX = 0, vrY = 0;
        bool vrClick = false;
        
        // Check if we are in VR Menu Mode
        if (VRManager::Get().GetMenuMouseState(dims.w, dims.h, vrX, vrY, vrClick)) {
             // Update Mouse Position for Game Logic
             this->setPosition(Pointi(vrX, vrY));
             
             // Handle Click
             if (vrClick) {
                 this->UI_OnMouseLeftClick();
             }

             // Hide OS cursor
             platform->setCursorShown(false);

             // Draw a "circle" using 2 rectangles (approximating 6x6 circle)
             // Vertical-ish rect (4x6)
             Recti r1;
             r1.w = 4;
             r1.h = 6;
             r1.x = vrX - r1.w / 2;
             r1.y = vrY - r1.h / 2;
             render->FillRect(r1, colorTable.Blue);

             // Horizontal-ish rect (6x4)
             Recti r2;
             r2.w = 6;
             r2.h = 4;
             r2.x = vrX - r2.w / 2;
             r2.y = vrY - r2.h / 2;
             render->FillRect(r2, colorTable.Blue);
             
             // Draw Picked Item if any (on top of cursor)
             if (pParty->pPickedItem.itemId != ITEM_NULL) {
                DrawPickedItem();
             }
             
             return; // Skip standard cursor logic
        }
    }

    // get mouse pos
    Pointi pos = this->position();

    // for party held item
    if (pParty->pPickedItem.itemId != ITEM_NULL) {
        DrawPickedItem();
        // TODO(pskelton): consider this in future - hide cursor when holding item
        //platform->setCursorShown(false);
    } else {
        // for other cursor img ie target mouse
        if (this->cursor_img) {
            platform->setCursorShown(false);
            // draw image - needs centering
            pos.x -= (this->cursor_img->width()) / 2;
            pos.y -= (this->cursor_img->height()) / 2;

            render->DrawQuad2D(this->cursor_img, pos);
        } else if (_mouseLook) {
            platform->setCursorShown(false);
            auto pointer = assets->getImage_ColorKey("MICON2", colorTable.Black /*colorTable.TealMask*/);
            render->DrawQuad2D(pointer, pViewport.center() - pointer->size() / 2);
        } else {
            platform->setCursorShown(true);
        }
    }
}

void Io::Mouse::DrawPickedItem() {
    if (pParty->pPickedItem.itemId == ITEM_NULL)
        return;

    GraphicsImage *pTexture = assets->getImage_Alpha(pParty->pPickedItem.GetIconName());
    if (!pTexture) return;

    Pointi mousePos = this->position();
    Pointi drawPos = {mousePos.x + pickedItemOffset.x, mousePos.y + pickedItemOffset.y};

    if (pParty->pPickedItem.IsBroken()) {
        render->DrawQuad2D(pTexture, drawPos, colorTable.Red);
    } else if (!pParty->pPickedItem.IsIdentified()) {
        render->DrawQuad2D(pTexture, drawPos, colorTable.Green);
    } else {
        render->DrawQuad2D(pTexture, drawPos);
    }
}

void Io::Mouse::UI_OnMouseLeftClick() {
    if (current_screen_type == SCREEN_VIDEO || isHoldingMouseRightButton())
        return;

    if (pGUIWindow_BranchlessDialogue && pGUIWindow_BranchlessDialogue->event() == EVENT_PressAnyKey) {
        releaseBranchlessDialogue();
        return;
    }

    if (pGameOverWindow) {
        if (pGameOverWindow->toggleAndTestFinished()) {
            pGameOverWindow->Release();
            delete pGameOverWindow;
            pGameOverWindow = nullptr;
        }
        return;
    }

    Pointi mousePos = this->position();
    int x = mousePos.x;
    int y = mousePos.y;

    if (GetCurrentMenuID() != MENU_NONE || current_screen_type != SCREEN_GAME ||
        !keyboardInputHandler->IsStealingToggled() || !pViewport.contains(Pointi(x, y))) {
        std::list<GUIWindow*> targetedSpellUI = {pGUIWindow_CastTargetedSpell};
        std::list<GUIWindow*> *checkWindowList = &lWindowList;
        if (pGUIWindow_CastTargetedSpell) {
            // Block regular UI if targeted spell casting is active
            checkWindowList = &targetedSpellUI;
        }
        for (GUIWindow *win : *checkWindowList) {
            if (win->Contains(x, y)) {
                for (GUIButton *control : win->vButtons) {
                    if (control->uButtonType == 1) {
                        if (control->Contains(x, y)) {
                            control->field_2C_is_pushed = true;
                            engine->_messageQueue->clear();
                            engine->_messageQueue->addMessageCurrentFrame(control->msg, control->msg_param, 0);
                            return;
                        }
                        continue;
                    }
                    if (control->uButtonType == 2) {  // adventurers portraits click (circular button)
                        // TODO(captainurist): actual shape is oval, this check is bugged.
                        int dx = x - control->rect.x;
                        int dy = y - control->rect.y;
                        if (std::sqrt((double)(dx * dx + dy * dy)) < (double)control->rect.w) {
                            control->field_2C_is_pushed = true;
                            engine->_messageQueue->clear();
                            engine->_messageQueue->addMessageCurrentFrame(control->msg, control->msg_param, 0);
                            return;
                        }
                        continue;
                    }
                    if (control->uButtonType == 3) {  // clicking skills
                        if (control->Contains(x, y)) {
                            control->field_2C_is_pushed = true;
                            engine->_messageQueue->clear();
                            engine->_messageQueue->addMessageCurrentFrame(control->msg, control->msg_param, 0);
                            return;
                        }
                        continue;
                    }
                }
                // absorb event if its within this windows boundary
                // prevent hidden buttons being pressed
                break;
            }
        }
        return;
    }

    Vis_PIDAndDepth picked_object = engine->PickMouseNormal();

    ObjectType type = picked_object.pid.type();
    if (type == OBJECT_Actor && pParty->hasActiveCharacter() && picked_object.depth < 0x200 &&
        pParty->activeCharacter().CanAct() &&
        pParty->activeCharacter().CanSteal()) {
        engine->_messageQueue->addMessageCurrentFrame(
            UIMSG_STEALFROMACTOR,
            picked_object.pid.id(),
            0
        );

        if (pParty->bTurnBasedModeOn) {
            if (pTurnEngine->turn_stage == TE_MOVEMENT) {
                pTurnEngine->flags |= TE_FLAG_8_finished;
            }
        }
    }
}

void Io::Mouse::SetMouseLook(bool enable) {
    if (_mouseLook != enable) {
        _position = pViewport.center();
        warpMouse(_position);
        window->setMouseRelative(enable);
    }
    _mouseLook = enable;
}

void Io::Mouse::ToggleMouseLook() {
    SetMouseLook(!_mouseLook);
}

void Io::Mouse::DoMouseLook(Pointi relChange) {
    if (!_mouseLook) {
        return;
    }

    float modX = relChange.x * engine->config->settings.MouseLookSensitivity.value();
    float modY = relChange.y * engine->config->settings.MouseLookSensitivity.value();
    pParty->_viewPitch -= modY;
    pParty->_viewPitch = std::clamp(pParty->_viewPitch, -320, 320);
    pParty->_viewYaw -= modX;
    pParty->_viewYaw &= TrigLUT.uDoublePiMask;
}

// TODO(pskelton): Move this to keyboard
bool UI_OnKeyDown(PlatformKey key) {
    for (GUIWindow *win : lWindowList) {
        if (!win->receives_keyboard_input) {
            continue;
        }

        if (keyboardActionMapping->isBound(INPUT_ACTION_DIALOG_LEFT, key)) {
            if (win->pCurrentPosActiveItem - win->pStartingPosActiveItem - win->_selectStep >= 0) {
                win->pCurrentPosActiveItem -= win->_selectStep;
                if (current_screen_type == SCREEN_PARTY_CREATION) {
                    pAudioPlayer->playUISound(SOUND_SelectingANewCharacter);
                }
            }
            if (win->_msgOnKeyboardSelect) {
                GUIButton *pButton = win->GetControl(win->pCurrentPosActiveItem);
                engine->_messageQueue->addMessageCurrentFrame(pButton->msg, pButton->msg_param, 0);
            }
            break;
        } else if (keyboardActionMapping->isBound(INPUT_ACTION_DIALOG_RIGHT, key)) {
            int newPos = win->pCurrentPosActiveItem + win->_selectStep;
            if (newPos < win->pNumPresenceButton + win->pStartingPosActiveItem) {
                win->pCurrentPosActiveItem = newPos;
                if (current_screen_type == SCREEN_PARTY_CREATION) {
                    pAudioPlayer->playUISound(SOUND_SelectingANewCharacter);
                }
            }
            if (win->_msgOnKeyboardSelect) {
                GUIButton *pButton = win->GetControl(win->pCurrentPosActiveItem);
                engine->_messageQueue->addMessageCurrentFrame(pButton->msg, pButton->msg_param, 0);
            }
            break;
        } else if (keyboardActionMapping->isBound(INPUT_ACTION_DIALOG_DOWN, key)) {
            int v17 = win->pStartingPosActiveItem;
            int v18 = win->pCurrentPosActiveItem;
            if (v18 >= win->pNumPresenceButton + v17 - 1)
                win->pCurrentPosActiveItem = v17;
            else
                win->pCurrentPosActiveItem = v18 + 1;
            if (win->_msgOnKeyboardSelect) {
                GUIButton *pButton = win->GetControl(win->pCurrentPosActiveItem);
                engine->_messageQueue->addMessageCurrentFrame(pButton->msg, pButton->msg_param, 0);
            }
            return true;
        } else if (key == PlatformKey::KEY_SELECT) {
            Pointi mousePos = EngineIocContainer::ResolveMouse()->position();
            int uClickX = mousePos.x;
            int uClickY = mousePos.y;
            int v4 = win->pStartingPosActiveItem;
            int v28 = v4 + win->pNumPresenceButton;
            if (v4 < v4 + win->pNumPresenceButton) {
                while (true) {
                    GUIButton *pButton = win->GetControl(v4);
                    if (pButton->Contains(uClickX, uClickY))  // test for StatsTab in PlayerCreation Window
                        break;
                    ++v4;
                    if (v4 >= v28) {
                        // v1 = 0;
                        // v2 = pCurrentFrameMessageQueue->uNumMessages;
                        // --i;
                        // if ( i < 0 )
                        return false;
                        // continue;
                    }
                }
                win->pCurrentPosActiveItem = v4;
                return true;
            }
            break;
        } else if (keyboardActionMapping->isBound(INPUT_ACTION_DIALOG_UP, key)) {
            int v22 = win->pCurrentPosActiveItem;
            int v23 = win->pStartingPosActiveItem;
            if (v22 <= v23)
                win->pCurrentPosActiveItem =
                    win->pNumPresenceButton + v23 - 1;
            else
                win->pCurrentPosActiveItem = v22 - 1;
            if (win->_msgOnKeyboardSelect) {
                GUIButton *pButton = win->GetControl(win->pCurrentPosActiveItem);
                engine->_messageQueue->addMessageCurrentFrame(pButton->msg, pButton->msg_param, 0);
            }
            return true;
        } else if (keyboardActionMapping->isBound(INPUT_ACTION_DIALOG_PRESS, key)) {
            GUIButton *pButton = win->GetControl(win->pCurrentPosActiveItem);
            engine->_messageQueue->addMessageCurrentFrame(pButton->msg, pButton->msg_param, 0);
        } else if (key == PlatformKey::KEY_PAGEDOWN) { // not button event from user, but a call from GUI_UpdateWindows to track mouse
            if (!win->_msgOnKeyboardSelect) {
                Pointi mousePos = EngineIocContainer::ResolveMouse()->position();
                int uClickX = mousePos.x;
                int uClickY = mousePos.y;
                int v29 = win->pStartingPosActiveItem + win->pNumPresenceButton;
                for (int v4 = win->pStartingPosActiveItem; v4 < v29; ++v4) {
                    GUIButton *pButton = win->GetControl(v4);
                    if (!pButton)
                        continue;
                    if (pButton->Contains(uClickX, uClickY)) {
                        win->pCurrentPosActiveItem = v4;
                        return true;
                    }
                }
            }
            break;
        }
    }

    return 0;
}

void Io::Mouse::warpMouse(Pointi position) {
    // Map position to output window coords
    Pointi pos = render->MapToPresent(position);
    window->warpMouse(pos);
}

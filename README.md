# Might and Magic 7 VR Mod (OpenEnroth) - v0.1

This mod adds Virtual Reality support to **Might and Magic VII** using **OpenXR**.

## 🚀 Quick Start (Download & Play)
If you just want to play the game in VR, follow these simple steps:

1. **Download**: Get the latest `OpenEnrothVR.exe` from the [bin/](bin/) folder.
2. **Install**: Copy `OpenEnrothVR.exe` into your **Might and Magic VII** installation folder (the one containing the `DATA`, `ANIMS`, and `SOUNDS` folders).
3. **VR Setup**:
   - Ensure you have a VR headset connected (Meta Quest, Valve Index, etc.).
   - Start **SteamVR** (or your preferred OpenXR runtime).
4. **Play**: Run `OpenEnrothVR.exe` from your game folder.

---

## Current Status: MVP
- **6DOF tracking**: HMD orientation controls camera.
- **Stereoscopic rendering**: full 3D world for both eyes.
- **Input**: VR controllers or keyboard/mouse.
- **Engine**: OpenEnroth (modern C++ reimplementation of MM7).
- **Backend**: OpenXR (SteamVR, Oculus/Meta, WMR).

**Note**: Playable, but developing quality of life features.

---

## Prerequisites
1. **Game data**: MM7 assets (`ANIMS`, `DATA`, `MUSIC`, `SOUNDS`).
2. **VR runtime**: SteamVR (recommended) or any OpenXR runtime.
3. **Build tools** (only for building):
   - **CMake** 3.20+ (VR docs) / **CMake** 3.27+ (OpenEnroth docs).
   - **C++ compiler**: VS 2019/2022 recommended for VR; minimums: VS 2022, GCC 13, AppleClang 15/Clang 15.
   - **SDL3** (window/context).

Main dependencies (OpenEnroth):
- SDL3, FFmpeg, OpenAL Soft, Zlib.

Optional:
- Python 3.x for style checks.

IDEs tested: VS 2022+, VS Code 2022+, CLion 2022+.

---

## Controls (VR)
**Left controller (locomotion & UI)**
- Stick up/down: move forward/back.
- Stick left/right: strafe.
- **X Button**: Toggle World-Locked Overlay (GUI Billboard) while in game.
- **Y Button / ESC**: Open Main Menu (activates World-Locked Overlay).
- **Raycast**: Point at 2D screens to move the mouse cursor.

**Right controller (view/action)**
- Stick left/right: snap/smooth turn.
- Stick up: jump.
- **Trigger**: Interact / Select / Mouse Left Click.
- **Grip**: Combat / Cast / Quick Cast.

---

## Build (VR)
1. Clone:
   ```bash
   git clone https://github.com/vallewillian-source/open-enroth-vr.git
   cd open-enroth-vr
   ```
2. Configure:
   ```bash
   mkdir build
   cd build
   cmake ..
   ```
   FetchContent downloads OpenXR SDK + deps.
3. Compile:
   ```bash
   cmake --build . --config Release
   ```
4. Install: copy `OpenEnrothVR.exe` (found in `build/src/Bin/OpenEnroth/Release/`) to your MM7 folder (where `DATA` is).

---

## Run (SteamVR)
1. Start SteamVR.
2. Run `OpenEnrothVR.exe`.
3. If headset is black, set SteamVR as default OpenXR runtime:
   `SteamVR Settings > Developer > Set SteamVR as OpenXR Runtime`.

Keyboard/mouse still works for standard MM7 in VR view.

---

## VR Technical Summary
### VR Subsystem (`src/Engine/VR/`)
- `VRManager` singleton for OpenXR (`XrInstance`, `XrSession`, `XrSpace`).
- Stereo swapchains (double-buffered).
- Converts OpenXR **Y-Up** (RH) to OpenEnroth **Z-Up**.
- Uses SDL3 to get `HWND`/`HGLRC` reliably on Windows 11.

### Render Pipeline Changes
**Engine loop (`src/Engine/Engine.cpp`)**
1. If VR initialized, begin OpenXR frame.
2. For each eye:
   - Acquire swapchain image.
   - Bind VR FBO.
   - `drawWorld()` with VR flags.
   - Release swapchain image.
3. `xrEndFrame`.
4. Render desktop view (debug/spectator).

**OpenGL renderer (`src/Engine/Graphics/Renderer/OpenGLRenderer.cpp`)**
- `_set_3d_modelview_matrix` and `_set_3d_projection_matrix` use `VRManager` matrices when `IsRenderingVREye()`.
- `BeginScene3D` respects currently bound VR FBO (does not reset to backbuffer).

---

## Unified 2D Overlay & Interaction System
The mod uses a unified system for all 2D interfaces (Main Menu, Houses, Shops, Guilds, Dungeons, and GUI Billboards).

### 1. World-Locked Overlay (Virtual Monitor)
When a 2D interface is active (e.g., `m_isGameMenuOpen` or `m_showGuiBillboard`), the 2D screen is projected onto a virtual plane in the 3D world.
- **Capture**: The game's 2D output is captured using `glBlitFramebuffer` into an overlay texture.
- **Positioning**: On activation, the overlay is placed **2.5m** in front of the player's current HMD position and oriented towards them. This "world-locks" the screen, providing a stable "virtual monitor" experience.
- **Rendering**: The overlay is rendered as a separate layer in the VR composition, ensuring high clarity and comfort.

### 2. Raycast Mouse Control
Interaction with 2D screens is handled via a raycast system from the left controller, replacing the traditional stick-based cursor movement.
- **Ray Generation**: A laser beam is projected from the left controller's position and orientation.
- **Intersection**: The system calculates the intersection between the ray and the virtual 2D plane.
- **Cursor Mapping**: The intersection point is mapped to normalized (0.0 to 1.0) coordinates, which are then converted to the game's native resolution (e.g., 640x480).
- **Input Injection**: These coordinates are injected into the game's mouse logic via `VRManager::GetMenuMouseState`, allowing seamless interaction with all existing UI elements.
- **Clicks**: The right controller's trigger acts as the Left Mouse Button.

---

## Stereo Convergence & Comfort
- To push objects farther and improve comfort, move each eye image **away from the nose** (more parallel).
- VR lenses focus at fixed distance (~2m). Convergence should simulate this to avoid nausea.
- World-locked full-screen UIs are more comfortable than head-locked.

---

## Build System (`CMakeLists.txt`)
- `FetchContent` downloads OpenXR SDK.
- Static library target `engine_vr` encapsulates VR logic.

---

## Mandatory Rules for Vibe Coding Models
- Never compile automatically generated code.
- Always use explicit casts (no implicit casts).
- Never use `dynamic_cast`.
- Always use `static_cast` for type conversion.
- Never use `reinterpret_cast` for type conversion.

---

# Might and Magic Trilogy (Original OpenEnroth Documentation)

## Development HOWTO
This section describes the development process and is required reading for contributors.

---

## Dependencies
Main: SDL3, FFmpeg, OpenAL Soft, Zlib.  
Prebuilt dependencies are used by default and resolved during CMake.

---

## Build on Windows
- Install Git and Visual Studio 2022.
- Windows SDK v10.0.20348.0+.
- Clone/fork `https://github.com/vallewillian-source/open-enroth-vr`.
- CMake: standalone or VS-provided; add to PATH if needed.
- Open folder in VS, pick x32/x64, wait for CMake, set startup `OpenEnrothVR.exe`, run.

---

## Coding Style
Based on Google C++ Style Guide. PRs fail on style issues.
- **Style checks**: build `check_style` target.
- **Documentation**: Doxygen format (`/**` and `@`).
- **Naming**: `CamelCase` for types, `camelCase` for methods/variables. Private members start with `_`.

---

## Testing
Policy: add tests for fixable bugs when possible.
- **Unit tests**: Google Test (`OpenEnroth_UnitTest`).
- **Game tests**: Instrumented engine; events injected between frames.

---

## Scripting (Lua)
Scripts in `resources/scripts`.
- **Lua Language Server** (LuaLS) recommended for development.

---

## Console Commands
In-game debug console:
1. Launch game.
2. Load/create game.
3. Press `~`.

---
# Extra VR Implementations
## House System Flow (Context)
The unified 2D system handles the rendering, but the game logic follows this flow:

### 1. Entry Flow (3D -> 2D)
An event in `EvtInterpreter.cpp` calls `enterHouse(houseId)`.
- Validates opening hours and bans in `src/GUI/UI/UIHouses.cpp`.
- `createHouseUI(houseId)` creates the `GUIWindow_House`.
- `current_screen_type` becomes `SCREEN_HOUSE`.

### 2. Exit Flow (2D -> 3D)
`houseDialogPressEscape()`:
- Clears NPC state and releases the dialogue window.
- `current_screen_type` returns to `SCREEN_GAME`, resuming game time.

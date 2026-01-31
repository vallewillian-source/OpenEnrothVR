# Might and Magic 7 VR Mod (OpenEnroth)

This project is a fork of [OpenEnroth](https://github.com/OpenEnroth/OpenEnroth) aiming to implement Virtual Reality support for **Might and Magic VII** using **OpenXR**.

## 🚀 Current Status: MVP (Minimum Viable Product)

- **3DOF Tracking**: Headset orientation controls the in-game camera view.
- **Stereoscopic Rendering**: Full 3D world rendering for both eyes (Left/Right).
- **Input**: Standard Keyboard & Mouse controls.
  - **WASD**: Move Party.
  - **Mouse**: Rotate Party Body.
  - **Headset**: Look around (Independent Head/Body rotation).
- **Engine**: Based on OpenEnroth (modern C++ reimplementation of MM7 engine).
- **Backend**: OpenXR (Compatible with SteamVR, Oculus/Meta, Windows Mixed Reality).

> **Note**: This is an early development version. The HUD (User Interface) is currently hidden in VR to prevent rendering artifacts, and positional tracking (6DOF) is not yet implemented.

---

## 🛠 Prerequisites

To run or build this mod, you need:

1.  **Game Data**: The original Might and Magic VII game assets (`ANIMS`, `DATA`, `MUSIC`, `SOUNDS`).
2.  **VR Runtime**: [SteamVR](https://store.steampowered.com/app/250820/SteamVR/) (Recommended) or any OpenXR-compliant runtime.
3.  **Development Tools** (only for building):
    - **CMake** 3.20 or newer.
    - **C++ Compiler** (Visual Studio 2019/2022 recommended for Windows).
    - **SDL3** (Used for window/context management).

---

## ⚙️ How to Build

1.  **Clone the repository**:
    ```bash
    git clone https://github.com/YourUsername/mm7_vr.git
    cd mm7_vr
    ```

2.  **Configure with CMake**:
    ```bash
    mkdir build
    cd build
    cmake ..
    ```
    *The build system will automatically fetch the OpenXR SDK and other necessary dependencies via FetchContent.*

3.  **Compile**:
    Open the generated solution in Visual Studio or run:
    ```bash
    cmake --build . --config Release
    ```

4.  **Install**:
    Copy the compiled `OpenEnroth.exe` to your MM7 game folder (where the `DATA` folder is located).

---

## 🎮 How to Run (SteamVR)

1.  **Start SteamVR**: Ensure your VR headset is connected and recognized by SteamVR.
2.  **Launch the Game**: Run `OpenEnroth.exe`.
3.  **Play**: The game should automatically render to your headset.
    *   If the headset display is black, check if SteamVR is set as your default OpenXR runtime (`SteamVR Settings > Developer > Set SteamVR as OpenXR Runtime`).
    *   Use **Keyboard/Mouse** to play standard MM7, but with the immersive view of VR.

---

## 🧠 Technical Implementation Details

This VR implementation integrates deeply with the OpenEnroth engine while keeping the VR subsystem modular.

### 1. VR Subsystem (`src/Engine/VR/`)
We introduced a singleton class `VRManager` to handle all OpenXR interactions:
*   **OpenXR Integration**: Manages the `XrInstance`, `XrSession`, and `XrSpace`.
*   **Swapchain Management**: Handles double-buffered swapchains for stereo rendering.
*   **Coordinate System Conversion**: Converts OpenXR's **Y-Up** (Right-Handed) coordinate system to OpenEnroth's **Z-Up** system on the fly.
*   **Windows 11 Compatibility**: Utilizes **SDL3** to reliably retrieve native window handles (`HWND`) and OpenGL contexts (`HGLRC`) required for OpenXR session creation on modern Windows versions.

### 2. Render Pipeline Interventions
The rendering flow was modified to inject VR frames before the main desktop presentation.

#### **Engine Loop (`src/Engine/Engine.cpp`)**
The `Engine::Draw()` method was modified to support a multi-pass render loop:
1.  **Check VR State**: If VR is initialized, begin the OpenXR frame.
2.  **Stereo Pass**: Loop through both eyes (Index 0 and 1):
    *   Acquire OpenXR Swapchain Image.
    *   Bind the VR Framebuffer (FBO).
    *   **Render World**: Calls `drawWorld()` with VR-specific flags.
    *   Release Swapchain Image.
3.  **Submit Frame**: Calls `xrEndFrame` to send layers to the compositor.
4.  **Desktop Pass**: Continues to render the standard view to the desktop window for debugging/spectator view.

#### **Renderer (`src/Engine/Graphics/Renderer/OpenGLRenderer.cpp`)**
Modifications were made to the core OpenGL renderer to support external view matrices:
*   **Matrix Override**: `_set_3d_modelview_matrix` and `_set_3d_projection_matrix` now check `VRManager::IsRenderingVREye()`. If true, they bypass the standard `Camera3D` calculations and use the matrices provided by `VRManager` (derived from HMD pose).
*   **State Management**: `BeginScene3D` was updated to respect the currently bound VR framebuffer instead of forcing a reset to the default backbuffer.

### 3. Aprendizados: Convergência Estéreo e Conforto Visual
Durante o desenvolvimento das telas de interface (Houses/Menus), estabelecemos diretrizes críticas para o posicionamento de elementos 2D em espaço 3D:

*   **Regra da Convergência (Profundidade):** 
    *   Para afastar um objeto e torná-lo mais confortável, as imagens de cada olho devem se mover para **longe do nariz** (eixos de visão mais paralelos).
    *   No código, isso significa **diminuir** o `convergenceOffset` (valor maior no divisor, ex: `w / 5.0`).
    *   Mover as imagens para **perto do nariz** (aumentar offset) causa a sensação de que o objeto está "colado na cara" e gera fadiga ocular.
*   **Conflito Acomodação-Vergência:** As lentes de VR têm foco fixo (geralmente 2 metros). Posicionar telas simulando essa distância através da convergência é essencial para evitar náusea.
*   **Tamanho Angular:** A percepção de distância é reforçada pelo tamanho. Uma tela maior com eixos paralelos parece uma tela de cinema distante; uma tela pequena com eixos cruzados parece um celular perto do rosto.
*   **Ancoragem Mundial (World-Locking):** Para interfaces de tela cheia (como casas), a ancoragem fixa na cabeça (Head-Locked) pode ser cansativa. Ancorar a tela no espaço 3D no momento da ativação permite que o usuário explore a interface naturalmente movendo os olhos e a cabeça, reduzindo o esforço visual.

---

### 4. Build System (`CMakeLists.txt`)
*   Added `FetchContent` logic to automatically download and link the **OpenXR SDK**.
*   Created a static library target `engine_vr` to encapsulate VR logic.

---

# Might and Magic Trilogy (Original OpenEnroth README)

[![Windows](https://github.com/OpenEnroth/OpenEnroth/workflows/Windows/badge.svg)](https://github.com/OpenEnroth/OpenEnroth/actions/workflows/windows.yml) 
[![Linux](https://github.com/OpenEnroth/OpenEnroth/workflows/Linux/badge.svg)](https://github.com/OpenEnroth/OpenEnroth/actions/workflows/linux.yml) 
[![MacOS](https://github.com/OpenEnroth/OpenEnroth/workflows/MacOS/badge.svg)](https://github.com/OpenEnroth/OpenEnroth/actions/workflows/macos.yml) 

We are creating an extensible engine & modding environment that would make it possible to play original Might & Magic VI-VIII games on modern platforms with improved graphics and quality-of-life features.

Currently only MM7 is playable.

## Original Project Links
*   **GitHub**: [OpenEnroth/OpenEnroth](https://github.com/OpenEnroth/OpenEnroth)
*   **Discord**: [Join the community](https://discord.gg/jRCyPtq)

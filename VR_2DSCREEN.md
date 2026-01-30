# Implementação de Telas 2D em Ambiente VR

Este documento detalha como as interfaces 2D do Might and Magic VII VR MOD (HUD e Menus) são projetadas e interagidas no ambiente Virtual Reality (VR) utilizando OpenXR.

## 1. Transformação de 2D para 3D

O desafio de renderizar uma interface projetada para 640x480 em um espaço 3D é resolvido através de duas estratégias complementares:

### A. Overlay Quad (In-Game HUD)
Durante o gameplay, o HUD não é renderizado diretamente na tela, mas sim em uma textura off-screen.
- **Captura**: O `Engine::Draw` chama `VRManager::BeginOverlayRender()` para redirecionar o desenho do HUD para um Framebuffer Object (FBO) específico.
- **Projeção**: A função `VRManager::RenderOverlay3D` utiliza um shader customizado (`overlayVertSrc`/`overlayFragSrc`) para desenhar um quadrilátero (Quad) no mundo 3D.
- **Posicionamento**: O Quad é posicionado a exatamente **1.5 metros** à frente da câmera do jogador, mantendo uma proporção de aspecto de aproximadamente 4:3.

### B. Compositor Quad Layer (Menu Principal)
Para o menu principal (acessado via ESC ou botão Y), utiliza-se uma funcionalidade nativa do OpenXR:
- **Captura**: `VRManager::CaptureScreenToOverlayLayer` utiliza `glBlitFramebuffer` para copiar o conteúdo final da tela 2D para um swapchain do OpenXR.
- **Renderização**: Em `VRManager::EndFrame`, um `XrCompositionLayerQuad` é enviado ao compositor do OpenXR. Isso garante que o menu tenha uma qualidade visual superior e seja renderizado como uma camada independente sobre o mundo 3D.

## 2. Sistema de Cursor VR

Como o mouse tradicional não é prático em VR, implementamos um sistema de cursor virtual controlado pelos controles VR.

### Movimentação
- **Input**: Utiliza o **Thumbstick Esquerdo** (`m_actionMove`).
- **Lógica**: A posição do cursor (`m_menuCursorX`, `m_menuCursorY`) é atualizada incrementalmente com base no deslocamento do analógico multiplicado por `m_menuCursorSpeed`.
- **Inversão de Eixo**: O eixo Y do analógico é invertido para coincidir com o sistema de coordenadas de tela (onde Y cresce para baixo).
- **Integração**: O valor calculado é injetado diretamente no sistema de mouse do jogo via `mouse->setPosition({menuX, menuY})`.

### Centralização Automática
- Ao abrir um menu, se for a primeira interação, o cursor é automaticamente centralizado (`menuWidth * 0.5`, `menuHeight * 0.5`) para facilitar a navegação.

## 3. Sistema de Clique e Interação

A interação com botões e elementos da UI utiliza os gatilhos dos controles.

- **Input**: Ambos os gatilhos, **Esquerdo** (`m_actionEsc`) e **Direito** (`m_actionInteract`), funcionam como o clique esquerdo do mouse.
- **Detecção de Clique**: 
    - Um limiar (threshold) de **0.5f** é aplicado ao valor analógico do gatilho.
    - Implementamos detecção de borda (edge detection) para garantir que apenas um evento de clique seja disparado por pressionamento.
- **Ação**: Dispara a função nativa do motor `mouse->UI_OnMouseLeftClick()`.

## 4. Mapeamento de Botões de Menu

- **Abrir Menu**: O botão **Y** do controle VR (`m_actionCombat`) ou o **Gatilho Esquerdo** (`m_actionEsc`) são mapeados para a ação de ESC do jogo.
- **Integração de Input**: O `KeyboardInputHandler` mescla os estados das ações VR com os inputs de teclado/gamepad tradicionais, permitindo uma transição fluida entre os modos de controle.

---
*Este sistema permite que toda a complexidade da interface original do MM7 seja utilizada em VR sem necessidade de reescrever a lógica de GUI do motor.*

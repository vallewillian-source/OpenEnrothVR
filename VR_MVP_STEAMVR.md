# MVP VR (SteamVR) — Plano Técnico (Somente Render 3D)

## Objetivo (menor MVP possível)

Executar o OpenEnroth (MM7) no Windows 11 e **visualizar apenas o render 3D no headset VR** via SteamVR, com:

- 3DOF (rotação do headset) suficiente
- Controle ainda via teclado/mouse (nenhuma integração com controles VR)
- Sem HUD/GUI, sem áudio, sem input VR, sem “qualidade de vida”
- O jogo pode continuar rodando com uma janela desktop “normal” (para manter o contexto OpenGL)

O foco do MVP é: **o conteúdo 3D do jogo (mundo) ser apresentado no HMD**, mesmo que:

- seja monoscópico (mesma imagem nos dois olhos) na primeira versão
- a projeção não seja fisicamente perfeita
- o movimento/locomoção ainda seja “flat”

## Leitura do código atual (pontos críticos)

### Loop e desenho

- O executável entra por [OpenEnroth.cpp](file:///c:/Users/Usu%C3%A1rio/www/mm7_vr/src/Bin/OpenEnroth/OpenEnroth.cpp).
- O setup (plataforma SDL, janela, OpenGL, renderer) acontece em [GameStarter.cpp](file:///c:/Users/Usu%C3%A1rio/www/mm7_vr/src/Application/Startup/GameStarter.cpp).
- O loop de jogo chama o pipeline de desenho em [Game.cpp](file:///c:/Users/Usu%C3%A1rio/www/mm7_vr/src/Application/Game.cpp#L99-L136).
- O desenho principal do jogo está em [Engine::Draw](file:///c:/Users/Usu%C3%A1rio/www/mm7_vr/src/Engine/Engine.cpp#L195-L202):
  - `drawWorld()` (3D)
  - `drawHUD()` (2D/HUD)
  - `drawOverlay()` (overlays)
  - `render->swapBuffers()` (apresenta na janela)

### Parte “3D pura”

O “mundo 3D” é desenhado em [Engine::drawWorld](file:///c:/Users/Usu%C3%A1rio/www/mm7_vr/src/Engine/Engine.cpp#L129-L177).

Isso é o que o MVP precisa capturar e levar para o headset.

### Renderer OpenGL e framebuffers

O renderer em uso no Windows é OpenGL, com GLAD/GLM:

- Interface do renderer: [Renderer.h](file:///c:/Users/Usu%C3%A1rio/www/mm7_vr/src/Engine/Graphics/Renderer/Renderer.h)
- Implementação OpenGL: [OpenGLRenderer.cpp](file:///c:/Users/Usu%C3%A1rio/www/mm7_vr/src/Engine/Graphics/Renderer/OpenGLRenderer.cpp)
- No `BeginScene3D`, quando há diferença entre render/present, o renderer desenha em um **FBO próprio** e depois faz upscale/downscale para a janela.
  - Entrada: [OpenGLRenderer::BeginScene3D](file:///c:/Users/Usu%C3%A1rio/www/mm7_vr/src/Engine/Graphics/Renderer/OpenGLRenderer.cpp#L279-L301)
  - “present”: [OpenGLRenderer::flushAndScale](file:///c:/Users/Usu%C3%A1rio/www/mm7_vr/src/Engine/Graphics/Renderer/OpenGLRenderer.cpp#L2200-L2231) e [OpenGLRenderer::swapBuffers](file:///c:/Users/Usu%C3%A1rio/www/mm7_vr/src/Engine/Graphics/Renderer/OpenGLRenderer.cpp#L2233-L2248)

Isso é importante porque o MVP pode ser implementado como:

- renderizar normalmente (no FBO interno do OpenGLRenderer) e **blit/copiar** para a textura do OpenXR/SteamVR

### Câmera e projeção

Hoje a câmera (yaw/pitch e FOV) é calculada em [Camera3D](file:///c:/Users/Usu%C3%A1rio/www/mm7_vr/src/Engine/Graphics/Camera.h):

- O engine define yaw/pitch da party e monta view/projeção em [Engine::drawWorld](file:///c:/Users/Usu%C3%A1rio/www/mm7_vr/src/Engine/Engine.cpp#L129-L141).
- A projeção OpenGL usa `glm::perspective` com FOV/ASPECT calculados pela câmera:
  - [OpenGLRenderer::_set_3d_projection_matrix](file:///c:/Users/Usu%C3%A1rio/www/mm7_vr/src/Engine/Graphics/Renderer/OpenGLRenderer.cpp#L936-L943)
  - [OpenGLRenderer::_set_3d_modelview_matrix](file:///c:/Users/Usu%C3%A1rio/www/mm7_vr/src/Engine/Graphics/Renderer/OpenGLRenderer.cpp#L945-L959)

Para o MVP, existem dois níveis possíveis:

1) **Monoscópico + orientação 3DOF** (mais simples): manter a projeção atual e só “somar” a rotação do HMD em yaw/pitch.
2) **Estéreo real com frustums assimétricos** (mais correto): usar `xrLocateViews` e gerar matrizes por-olho.

O documento assume o caminho 1) como MVP mínimo, e descreve como evoluir para 2).

## Escolha técnica: OpenXR (com SteamVR como runtime)

Para SteamVR no Windows 11, o caminho mais simples e futuro‑prova é **OpenXR**:

- SteamVR oferece runtime OpenXR.
- Evita dependência do OpenVR (legado) e facilita evoluir para outros runtimes.

O engine já usa OpenGL via SDL, então o alvo é:

- OpenXR + `XR_KHR_opengl_enable` (binding OpenGL/Win32)

## Arquitetura do MVP (mínima e incremental)

### Resumo do pipeline VR por frame

Em cada frame:

1. Processar eventos do OpenXR (loss/restart, session state)
2. `xrWaitFrame` / `xrBeginFrame`
3. Adquirir uma imagem do swapchain (`xrAcquireSwapchainImage`, `xrWaitSwapchainImage`)
4. Renderizar o mundo 3D do jogo para uma textura OpenGL
5. `xrReleaseSwapchainImage`
6. `xrEndFrame` com um layer `XrCompositionLayerProjection`

### Como “renderizar o mundo 3D” com o mínimo de mudanças

O menor impacto na base atual é **não mexer no pipeline 3D interno**, e apenas:

- chamar o desenho 3D padrão (ou uma variante “sem HUD”)  
- copiar o resultado para a textura do OpenXR

Existem duas estratégias. Para o MVP, recomendo a A:

#### Estratégia A (recomendada no MVP): “Blit” do FBO do renderer para o swapchain OpenXR

- O OpenGLRenderer já usa um FBO interno com cor+depth em `BeginScene3D`.
- Após terminar `drawWorld()`, o conteúdo está no FBO interno (ou no backbuffer, dependendo da config).
- Criar (temporariamente) um FBO de destino por imagem do swapchain do OpenXR:
  - anexar o `GL_TEXTURE_2D` do swapchain como `GL_COLOR_ATTACHMENT0`
  - `glBlitFramebuffer` do FBO do OpenGLRenderer para o FBO do swapchain

Vantagens:

- Não precisa reescrever o renderer para “render target externo”.
- O mundo 3D do jogo continua sendo desenhado exatamente como já é.
- Permite começar **monoscópico** (copiar a mesma imagem para os dois olhos) rapidamente.

Desvantagens:

- Cópia extra por frame (ok para MVP).
- Estéreo real ainda não existe; mas já permite “ver o mundo no HMD”.

#### Estratégia B (evolução): renderizar diretamente na textura do swapchain

Depois do MVP, refatorar o OpenGLRenderer para aceitar um “render target” externo:

- trocar o FBO interno para o FBO do swapchain (cor) + depth alocado compatível
- ajustar `outputRender` para o tamanho recomendado do runtime (`xrEnumerateViewConfigurationViews`)

Vantagens:

- Sem cópia extra.
- Mais fácil dobrar para render por-olho (estéreo real).

Desvantagens:

- Mudança maior no renderer e no controle de viewport.

## MVP mínimo proposto (por etapas)

### Etapa 0 — “Hello OpenXR” dentro do executável (sem tocar no engine)

Objetivo:

- Confirmar que o runtime OpenXR está acessível e que conseguimos criar `XrInstance`, `XrSystemId` e `XrSession` com OpenGL.

Detalhes importantes (Windows + OpenGL):

- Para `XR_KHR_opengl_enable`, o binding Win32 precisa de `HDC` e `HGLRC`.
- Como o contexto OpenGL do jogo é SDL, o caminho mais simples é pegar do próprio WGL:
  - `wglGetCurrentDC()`
  - `wglGetCurrentContext()`
- Isso exige que o contexto SDL já esteja “current” quando inicializarmos o OpenXR (após o renderer inicializar).

Ponto ideal de inicialização:

- Depois de `_renderer->Initialize()` em [GameStarter::initialize](file:///c:/Users/Usu%C3%A1rio/www/mm7_vr/src/Application/Startup/GameStarter.cpp#L167-L173).

### Etapa 1 — Mirror monoscópico: copiar o frame 3D para os dois olhos

Objetivo:

- Renderizar o mundo 3D como já é feito hoje.
- Pegar o resultado e enviar ao HMD, **mesma imagem para ambos os olhos**.

Mudanças mínimas no engine:

- Introduzir um “modo VR” (flag de runtime/config) que:
  - chama apenas `Engine::drawWorld()` e não chama HUD/overlay
  - faz o submit no OpenXR no final do frame

Onde plugar:

- Um ponto simples é substituir/ramificar em [Engine::Draw](file:///c:/Users/Usu%C3%A1rio/www/mm7_vr/src/Engine/Engine.cpp#L195-L202) para um caminho “VR”.

Exemplo de comportamento em VR:

- `drawWorld()`
- (não chamar `drawHUD`, `flushAndScale`, `drawOverlay`)
- não depender do `swapBuffers()` para o HMD (o HMD é `xrEndFrame`)
- opcional: ainda chamar `swapBuffers()` para manter uma janela espelho (debug)

### Etapa 2 — 3DOF real: usar orientação do HMD para yaw/pitch

Objetivo:

- O headset controla a direção de olhar, sem alterar posição (a posição continua do party/câmera do jogo).

Opção de menor risco no MVP:

- A cada frame, ler `XrPosef` (orientação quaternion) e convertê-la para yaw/pitch.
- Somar isso aos valores atuais do jogo:
  - `pParty->_viewYaw` / `pParty->_viewPitch`

Observação:

- O engine usa yaw/pitch também para culling/frustum em [Engine::drawWorld](file:///c:/Users/Usu%C3%A1rio/www/mm7_vr/src/Engine/Engine.cpp#L129-L141) e em [Camera3D](file:///c:/Users/Usu%C3%A1rio/www/mm7_vr/src/Engine/Graphics/Camera.cpp#L163-L193).
- Para o MVP, é aceitável “dirigir” a câmera com esses ângulos.

Alternativa mais correta (para depois):

- Parar de derivar o view matrix de yaw/pitch e passar uma `viewmat` diretamente (quaternion), preservando yaw/pitch apenas para mecânicas antigas.

### Etapa 3 (pós‑MVP) — Estéreo real e projeções corretas

Objetivo:

- Render por-olho com IPD e frustum assimétrico.

Como fazer:

- Usar `xrLocateViews` para obter:
  - pose por olho
  - `XrFovf` por olho (ângulos left/right/up/down)
- Construir matriz de projeção assimétrica (OpenGL clip space) com `glm::frustum` ou matriz manual.
- Ajustar o view matrix por olho aplicando o offset do olho (posição do view) sobre a posição do party.

Impactos no código:

- `OpenGLRenderer::_set_3d_projection_matrix` hoje é simétrica e usa `glm::perspective`.
- Para VR, será necessário um caminho alternativo para setar `projmat`/`viewmat` por olho.

## Organização recomendada do código (para quando implementarmos)

Mesmo para o MVP, vale separar responsabilidades para não contaminar o engine:

- Um módulo “VR runtime” (OpenXR):
  - inicializa/shutdown
  - processa events e controla session state
  - gerencia swapchains (texturas OpenGL)
  - fornece pose/orientação (3DOF) por frame
- Um módulo “VR compositor” dentro do renderer OpenGL:
  - cria FBOs para anexar texturas do swapchain
  - faz `glBlitFramebuffer` do framebuffer do jogo -> framebuffer VR

Pontos de integração:

- Inicialização após o OpenGL existir: [GameStarter.cpp](file:///c:/Users/Usu%C3%A1rio/www/mm7_vr/src/Application/Startup/GameStarter.cpp#L167-L173)
- Hook por frame próximo ao desenho: [Engine::Draw](file:///c:/Users/Usu%C3%A1rio/www/mm7_vr/src/Engine/Engine.cpp#L195-L202)

## Critérios de “MVP concluído”

Considero o MVP completo quando:

- O executável abre e cria uma sessão OpenXR usando o runtime SteamVR.
- Dentro do HMD, é possível ver o mundo 3D do jogo (mesmo monoscópico).
- A orientação do HMD altera a direção de olhar (3DOF).
- O input continua via teclado/mouse.
- HUD/GUI/overlays podem estar desativados (mundo 3D limpo).

## Riscos e mitigação (importantes no MVP)

### 1) Binding OpenXR + OpenGL em SDL

Risco:

- Dificuldade para obter handles nativos.

Mitigação:

- Usar WGL diretamente (`wglGetCurrentDC`, `wglGetCurrentContext`) após o contexto SDL estar current.

### 2) Frame timing / FPS

Risco:

- O engine controla FPS via config em [OpenGLRenderer::swapBuffers](file:///c:/Users/Usu%C3%A1rio/www/mm7_vr/src/Engine/Graphics/Renderer/OpenGLRenderer.cpp#L2246-L2248).
- Em VR, o ritmo ideal vem de `xrWaitFrame`.

Mitigação:

- No MVP, aceitar duplicidade (engine limita + OpenXR espera).
- Pós‑MVP: quando VR ativo, desabilitar FPS limit interno e sincronizar com OpenXR.

### 3) Escala e clipping

Risco:

- O “mundo” do MM7 tem escala arbitrária (unidades internas).
- Em VR, escala errada pode causar desconforto.

Mitigação:

- MVP monoscópico reduz muito esse problema.
- Pós‑MVP: introduzir fator “world scale” e calibrar IPD/offset.

## Notas práticas (SteamVR)

- O SteamVR precisa estar configurado como runtime OpenXR do sistema (Settings → OpenXR).
- O jogo pode continuar abrindo janela no desktop; isso é útil para debug e necessário para manter o contexto OpenGL no Windows.


# Arquitetura

## Visão geral

```
┌──────────────────────────── OBS Studio ─────────────────────────────┐
│                                                                     │
│  Display Capture                     ┌──────── TopScreen Filter ──┐  │
│  (escolhe a tela) ─────────────────► │  Input      (cursor/clicks)│  │
│                                      │  Dynamics   (mola/zoom/pan)│  │
│                                      │  Compositor (shaders GPU)  │  │
│                                      │  Settings   (props/presets)│  │
│                                      │  Hotkeys    (API do OBS)   │  │
│                                      └──────────────┬─────────────┘  │
│                                                     ▼                │
│                                Cena do OBS (fonte processada)        │
└─────────────────────────────────────────────────────────────────────┘
```

O filtro recebe o frame da fonte-pai já renderizado (`obs_source_process_filter_*`),
aplica uma cadeia de passes GPU e devolve o resultado. Nada sai do processo do OBS.

## Módulos

| Módulo | Arquivo | Responsabilidade |
|--------|---------|------------------|
| Plugin | `src/plugin-main.cpp` | `OBS_DECLARE_MODULE`, registro de fonte, dock |
| Filtro | `src/ts-filter.cpp` | `obs_source_info`, ciclo de vida, render, atalhos |
| Settings | `src/ts-settings.{hpp,cpp}` | Estrutura de config, defaults, load/save |
| Propriedades | `src/ts-properties.{hpp,cpp}` | UI nativa (obs_properties) |
| Compositor | `src/render/ts-compositor.{hpp,cpp}` | Efeito shader + upload de parâmetros |
| Dynamics | `src/dynamics/ts-spring.{hpp,cpp}` | Mola criticamente amortecida, easing, idle |
| Input | `src/input/ts-input.{hpp,cpp}` + `ts-input-platform.cpp` | Estado de cursor/botões/teclas |
| Hotkeys | `src/hotkeys/ts-hotkeys.{hpp,cpp}` | Registro de atalhos + callbacks |
| Texto | `src/text/ts-sdf-font.{hpp,cpp}` | Fonte SDF (overlay de teclas) |
| Áudio | `src/audio/ts-click-sound.{hpp,cpp}` | Som de clique |
| UI | `src/ui/ts-dock.{hpp,cpp}` | Dock Qt (presets/toggles) — opcional |

## Pipeline de render

Ordem dos passes (barato → caro quando possível):

```
frame do OBS (texture GPU)
 1. Background      → cor/gradiente/imagem + padding
 2. Rounded corners → máscara SDF com anti-aliasing
 3. Shadow / inset  → pirâmide de mip (blur barato)
 4. Zoom / Pan      → transform de UV (mola)  ← afeta 1..3 juntos
 5. Cursor          → sprite escalável (+sombra, +sway)
 6. Click FX        → ripple/orb/anel (L e R independentes)
 7. Motion blur     → só em movimento rápido
 8. Keystroke HUD   → texto SDF
 9. Spotlight / Dim → passe único opcional
```

Na implementação atual (F1), os passes 1–4 estão no shader
`data/effects/topscreen.effect`. Os demais entram nas fases seguintes.

### Por que um único `.effect` em F1

- Menos transições de estado na GPU (menos overhead por frame).
- Fácil de manter e portável (OBS compila o mesmo arquivo para HLSL/D3D11 e GLSL/OpenGL).
- Quando o custo justificar, os passes são separados em `.effect` distintos.

## Fluxo de dados do input

```
[SO] cursor/botões/teclas
   │
   ▼
ts-input (poll por tick)  ──►  ts_InputState {x,y,left,right,edges}
   │
   ├─► Cursor   (posição → sprite)
   ├─► Click FX (edge → nova animação)
   ├─► Auto-zoom(edge/posição → foco)
   └─► Keys     (eventos → HUD)
```

Coordenadas globais → coordenadas do canvas são resolvidas no módulo de input
(F2), considerando posição/escala do monitor e DPI.

## Persistência

- Configurações do filtro: `obs_data` (JSON) gerenciado pelo OBS por fonte/cena.
- Presets do TopScreen: arquivos em `data/presets/*.json`, aplicáveis por atalho.
- Atalhos: gerenciados pelo OBS (aba Atalhos) e, no Wayland, espelhados no portal.

## Composição gráfica

- Backend gráfico: `gs_*` do `libobs` (D3D11 no Windows, OpenGL no Linux).
- Shaders: `.effect` (sintaxe HLSL-like, compilada para o backend ativo).
- Sem draw calls customizadas fora do `obs_source_process_filter_*`; o filtro usa
  `OBS_NO_DIRECT_RENDERING` para garantir que o `image` seja a textura do pai.

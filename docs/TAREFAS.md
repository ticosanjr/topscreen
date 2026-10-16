# Tarefas

Quebra executável por fase. IDs no formato `T<fase>.<n>`.
Status: ✅ feito · 🟡 parcial · ⬜ pendente · 🔵 em andamento.

Dependências são indicadas entre parênteses.

---

## F0 — Skeleton do plugin

| ID | Tarefa | Deps | Status |
|----|--------|------|--------|
| T0.1 | Estrutura de repositório, `CMakeLists.txt`, `.gitignore`, `.clang-format` | — | ✅ |
| T0.2 | `plugin-main.cpp` com `OBS_DECLARE_MODULE` + registro da fonte | T0.1 | ✅ |
| T0.3 | `obs_source_info` do filtro (create/destroy/update/render/defaults/properties) | T0.2 | ✅ |
| T0.4 | Efeito shader passthrough (`data/effects/topscreen.effect`) | T0.3 | ✅ |
| T0.5 | Locales `en-US.ini` e `pt-BR.ini` | T0.2 | ✅ |
| T0.6 | `docs/BUILD.md` com passo a passo Win/Linux | T0.1 | ✅ |

**Aceite F0**: `cmake --build` gera `topscreen.(so|dll)`; OBS carrega o módulo e
o filtro aparece em Filtros sem alterar a imagem.

---

## F1 — Fundo, cantos, padding, sombra

| ID | Tarefa | Deps | Status |
|----|--------|------|--------|
| T1.1 | `ts-settings` (struct `Config` + defaults + load) | T0.3 | ✅ |
| T1.2 | `ts-properties` (grupos Geral/Fundo/Sombra/Zoom) | T1.1 | ✅ |
| T1.3 | `ts-compositor`: carregar efeito + cache de parâmetros | T0.4 | ✅ |
| T1.4 | Shader: fundo sólido/gradiente, cantos SDF, padding, sombra aproximada | T1.3 | ✅ |
| T1.5 | Zoom/pan estático por parâmetros (foco + zoom) no shader | T1.4 | ✅ |
| T1.6 | Dock Qt inicial (presets/toggles) via `obs-frontend-api` | T0.3 | 🟡 |
| T1.7 | Presets embutidos JSON (`demo`, `tutorial`, `apresentacao`) | T1.1 | 🟡 |
| T1.8 | Dock: aplicar preset e refletir estado ao vivo | T1.6,T1.7 | ⬜ |

**Aceite F1**: aplicar fundo/cantos/padding/sombra ao vivo, < 1 ms GPU.

---

## F2 — Cursor

| ID | Tarefa | Deps | Status |
|----|--------|------|--------|
| T2.1 | Ocultar cursor nativo do pai (detectar captura de tela e setar `capture_cursor=false`) | T0.3 | ⬜ |
| T2.2 | Mapear coordenadas globais → canvas (DPI/multi-monitor) | T1.3 | ⬜ |
| T2.3 | Ativar/desativar cursor nativo do filtro | T1.3 | ⬜ |
| T2.4 | Sprite de cursor escalável + sombra | T2.3 | ⬜ |
| T2.5 | Suavização (spring) do movimento | T2.4 | ⬜ |
| T2.6 | Sway (inclinação na curva) | T2.5 | ⬜ |
| T2.7 | Ocultar cursor parado (idle) e loop | T2.4 | ⬜ |
| T2.8 | Presets de estilo de cursor (macOS/touch) | T2.4 | ⬜ |

---

## F3 — Cliques, som, atalhos, presets

| ID | Tarefa | Deps | Status |
|----|--------|------|--------|
| T3.1 | Edges de botão no input (poll) + fila de eventos | T0.3 | ✅ |
| T3.2 | Registro de atalhos (`TopScreen.*`) + callbacks | T0.3 | ✅ |
| T3.3 | Efeito de clique esquerdo (ripple/orbe/anel) no shader | T3.1,T1.4 | ⬜ |
| T3.4 | Efeito de clique direito independente | T3.3 | ⬜ |
| T3.5 | Parâmetros de clique (cor/tamanho/duração/curva) na UI | T3.3 | ⬜ |
| T3.6 | Som de clique (decode WAV + playback) | T3.1 | 🟡 |
| T3.7 | Presets aplicáveis por atalho (next/prev) | T1.7,T3.2 | ⬜ |
| T3.8 | Fallback de atalhos via OBS WebSocket (Wayland sem portal) | T3.2 | ⬜ |

---

## F4 — Zoom / Pan

| ID | Tarefa | Deps | Status |
|----|--------|------|--------|
| T4.1 | Mola criticamente amortecida + easing utilitários | T0.3 | 🟡 |
| T4.2 | Auto-zoom por clique (foco no ponto clicado) | T4.1,T3.1 | ⬜ |
| T4.3 | Auto-zoom seguindo cursor com timeout de inatividade | T4.2 | ⬜ |
| T4.4 | Zoom manual in/out/reset por atalho | T4.1,T3.2 | ⬜ |
| T4.5 | Pan leve seguindo o mouse | T4.1 | ⬜ |
| T4.6 | Parâmetros de zoom na UI (quantidade/duração/curva/velocidade) | T4.2 | ⬜ |
| T4.7 | Motion blur (cursor e zoom) | T4.2 | ⬜ |

---

## F5 — Texto, spotlight, webcam

| ID | Tarefa | Deps | Status |
|----|--------|------|--------|
| T5.1 | Carregador de fonte SDF + atlas | T0.3 | 🟡 |
| T5.2 | Render de texto SDF (nítido em qualquer escala) | T5.1 | ⬜ |
| T5.3 | Captura de teclas (Win hook / X11 XRecord) | T0.3 | ⬜ |
| T5.4 | Overlay de teclas "pill" (posição/estilo/TTL) | T5.2,T5.3 | ⬜ |
| T5.5 | Spotlight/dim seguindo o cursor | T2.2 | ⬜ |
| T5.6 | Webcam bubble (forma/canto/escala/espelho) | T1.3 | ⬜ |
| T5.7 | Marca-texto de arrasto | T2.2 | ⬜ |

---

## F6 — Fonte de captura própria + áudio

| ID | Tarefa | Deps | Status |
|----|--------|------|--------|
| T6.1 | Backend de captura Windows (DXGI Desktop Duplication) | F1 | ⬜ |
| T6.2 | Backend de captura Linux (PipeWire + XShm) | F1 | ⬜ |
| T6.3 | `obs_source_info` de fonte de entrada (não-filtro) | T6.1,T6.2 | ⬜ |
| T6.4 | Seleção de tela na UI (lista de monitores) | T6.3 | ⬜ |
| T6.5 | Saída de áudio própria (`obs_source_output_audio`) para som de clique | T6.3,T3.6 | ⬜ |

---

## F7 — Paridade Wayland + polimento

| ID | Tarefa | Deps | Status |
|----|--------|------|--------|
| T7.1 | Portal `GlobalShortcuts` (D-Bus) espelhando os atalhos | T3.2 | ⬜ |
| T7.2 | Backend evdev (libevdev) para cliques/teclas | T5.3 | ⬜ |
| T7.3 | Regra udev `uaccess` + detecção/instruct no dock | T7.2 | ⬜ |
| T7.4 | Degradação graciosa + relatório de tiers no dock | T7.1,T7.2 | ⬜ |
| T7.5 | CI: build Win + Linux contra 2 versões do OBS | T0.1 | ⬜ |
| T7.6 | Empacotamento portable + release | T7.5 | ⬜ |
| T7.7 | i18n final + ícone + screenshots | T7.6 | ⬜ |

---

## Infra / documentação

| ID | Tarefa | Status |
|----|--------|--------|
| TD.1 | Documentar licença GPL-2.0 (texto completo do LICENSE) | 🟡 |
| TD.2 | `CONTRIBUTING.md` e template de PR | ⬜ |
| TD.3 | Ajustar `buildspec.json`/presets para CI oficial | ⬜ |
| TD.4 | Testes de sanidade (headless) do compositor | ⬜ |

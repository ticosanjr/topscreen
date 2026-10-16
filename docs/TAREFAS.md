# Tarefas

Quebra executável por fase. IDs no formato `T<fase>.<n>`.
Status: ✅ feito · 🟡 parcial · ⬜ pendente.

> "Feito" = implementado. Onde o item só é verificável no OBS (shader/integração),
> veja [`TESTES.md`](TESTES.md) para o que falta validar em runtime.

---

## F0 — Skeleton do plugin

| ID | Tarefa | Status |
|----|--------|--------|
| T0.1 | Estrutura de repositório, `CMakeLists.txt`, `.gitignore`, `.clang-format` | ✅ |
| T0.2 | `plugin-main.cpp` com `OBS_DECLARE_MODULE` + registro da fonte | ✅ |
| T0.3 | `obs_source_info` do filtro (create/destroy/update/render/defaults/properties) | ✅ |
| T0.4 | Efeito shader (`data/effects/topscreen.effect`) | ✅ |
| T0.5 | Locales `en-US.ini` e `pt-BR.ini` | ✅ |
| T0.6 | `docs/BUILD.md` com passo a passo Win/Linux | ✅ |

## F1 — Fundo, cantos, padding, sombra

| ID | Tarefa | Status |
|----|--------|--------|
| T1.1 | `ts-settings` (obs_data ↔ `core::Config`) | ✅ |
| T1.2 | `ts-properties` (grupos Geral/Fundo/Cursor/Cliques/Som/Zoom) | ✅ |
| T1.3 | `ts-compositor`: carregar efeito + cache de parâmetros | ✅ |
| T1.4 | Shader: fundo sólido/gradiente, cantos SDF, padding, sombra aprox. | ✅ |
| T1.5 | Zoom/pan por parâmetros no shader | ✅ |
| T1.6 | Dock Qt inicial (presets/toggles) | ✅ |
| T1.7 | Presets embutidos JSON + carregador (`ts-presets`) | ✅ |
| T1.8 | Dock: aplicar preset e refletir estado ao vivo | ✅ |

## F2 — Cursor

| ID | Tarefa | Status |
|----|--------|--------|
| T2.1 | Ocultar cursor nativo do pai (detectar captura de tela) | ✅ |
| T2.2 | Mapear coordenadas globais → canvas (DPI/multi-monitor) | 🟡 (origem 0,0) |
| T2.3 | Ativar/desativar cursor do filtro | ✅ |
| T2.4 | Cursor procedural escalável + sombra | ✅ |
| T2.5 | Suavização (spring) do movimento | ✅ |
| T2.6 | Sway (inclinação na curva) | ✅ |
| T2.7 | Ocultar cursor parado (idle) e loop | ✅ |
| T2.8 | Presets de estilo de cursor (macOS/touch distintos) | 🟡 (enum, 1 visual) |

## F3 — Cliques, som, atalhos, presets

| ID | Tarefa | Status |
|----|--------|--------|
| T3.1 | Edges de botão no input + fila de eventos | ✅ |
| T3.2 | Registro de atalhos (`TopScreen.*`) + callbacks | ✅ |
| T3.3 | Efeito de clique esquerdo (ripple/orbe/anel/força) | ✅ |
| T3.4 | Efeito de clique direito independente | ✅ |
| T3.5 | Parâmetros de clique (cor/tamanho/duração/curva) na UI | ✅ |
| T3.6 | Som de clique (decode WAV + playback) | ✅ (decode + playback via audio do sistema; canal OBS em T6.5) |
| T3.7 | Presets aplicáveis por atalho (next/prev) | ✅ |
| T3.8 | Fallback de atalhos via OBS WebSocket | ⬜ |

## F4 — Zoom / Pan

| ID | Tarefa | Status |
|----|--------|--------|
| T4.1 | Mola + easings (`core/spring`, `core/easing`) | ✅ |
| T4.2 | Auto-zoom por clique | ✅ |
| T4.3 | Auto-zoom seguindo cursor com timeout | ✅ |
| T4.4 | Zoom manual in/out/reset por atalho | ✅ |
| T4.5 | Pan leve seguindo o mouse | ✅ |
| T4.6 | Parâmetros de zoom na UI | ✅ |
| T4.7 | Motion blur (cursor e zoom) | ✅ (blur radial do zoom) |

## F5 — Texto, spotlight, webcam

| ID | Tarefa | Status |
|----|--------|--------|
| T5.0 | Modelo do overlay de teclas (fila/TTL/combos) | ✅ |
| T5.1 | Atlas de fonte (TTF + stb_truetype) | ✅ |
| T5.2 | Render de texto (sprites de glifos, contorno) | ✅ |
| T5.3 | Captura de teclas (Windows / X11; evdev em F7) | ✅ |
| T5.4 | Overlay de teclas | ✅ |
| T5.5 | Spotlight/dim seguindo o cursor | ✅ |
| T5.6 | Webcam bubble | ⬜ (precisa de fonte própria - F6) |
| T5.7 | Marca-texto de arrasto | ✅ |

## F6 — Fonte de captura própria + áudio

| ID | Tarefa | Status |
|----|--------|--------|
| T6.1 | Captura Windows (DXGI Desktop Duplication) | ⬜ |
| T6.2 | Captura Linux (PipeWire + XShm) | ⬜ |
| T6.3 | `obs_source_info` de fonte de entrada | ⬜ |
| T6.4 | Seleção de tela na UI | ⬜ |
| T6.5 | Saída de áudio própria (som de clique no canal) | ⬜ |

## F7 — Paridade Wayland + polimento

| ID | Tarefa | Status |
|----|--------|--------|
| T7.1 | Portal `GlobalShortcuts` | ⬜ |
| T7.2 | Backend evdev (libevdev) | ✅ (leitura direta de /dev/input + decoder testado) |
| T7.3 | Regra udev `uaccess` + detecção no dock | ✅ (regra pronta + aviso no log) |
| T7.4 | Degradação graciosa + relatório de tiers | 🟡 (tier no log/campo) |
| T7.5 | CI: build Win + Linux contra OBS | 🟡 (CI de testes + checagem de headers ✅; build do plugin best-effort) |
| T7.6 | Empacotamento portable + release | ⬜ |
| T7.7 | i18n final + ícone + screenshots | ⬜ |

## Infra / documentação

| ID | Tarefa | Status |
|----|--------|--------|
| TD.1 | Texto completo da licença GPL-2.0 (`COPYING`) | ✅ |
| TD.2 | `CONTRIBUTING.md` e template de PR | ✅ |
| TD.3 | Ajustar `buildspec.json`/presets para CI oficial | ⬜ |
| TD.4 | Testes de sanidade do núcleo | ✅ (50 testes + checagem de headers do OBS) |

---

## Próximos passos (ordem sugerida)

1. **Verificação em OBS real** do shader (cursor, cliques, zoom) — marco M1.
2. **T2.1** ocultar cursor nativo + **T2.2** mapeamento por monitor.
3. **T3.6** som de clique (decode + playback) e **T3.8** fallback WebSocket.
4. **T4.7** motion blur.
5. **T1.8/T1.6** completar o dock.
6. **F5** texto/teclas/spotlight/webcam.

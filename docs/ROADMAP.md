# Roadmap

Cada fase entrega algo utilizável e tem critério de aceite verificável.

> **Verificação:** a lógica do núcleo é coberta por testes automatizados
> (`docs/TESTES.md`). O shader e a integração com o OBS ainda precisam de uma
> passada em um OBS real — marcados com 🟡 mesmo quando implementados.

| Fase | Entrega | Critério de aceite | Status |
|------|---------|--------------------|--------|
| **F0** | Skeleton do plugin: módulo carrega, filtro passthrough, CMake Win/Linux | Carrega no OBS portable sem crash; filtro não altera a imagem | ✅ |
| **F1** | Fundo (cor/gradiente), cantos arredondados, padding, sombra, propriedades, presets + dock inicial | Visual "bonito" ao vivo, < 1 ms GPU no preset padrão | 🟡 |
| **F2** | Cursor: estilo, tamanho, sombra, suavização, sway, idle/loop | Cursor nativo ocultado; cursor próprio segue suave | 🟡 |
| **F3** | Cliques L/R (efeitos + cor/tamanho/duração/curva), som, atalhos, presets | Hotkeys na aba do OBS; L e R distintos; presets por atalho | 🟡 |
| **F4** | Zoom/Pan: mola, auto-zoom (clique/cursor), zoom manual, pan leve | Curva/duração/velocidade/inatividade ajustáveis; sem "salto" | 🟡 |
| **F5** | Overlay de teclas (SDF), spotlight, webcam bubble | Texto nítido em qualquer escala; webcam com formas/cantos | ⬜ |
| **F6** | Fonte de captura própria + áudio próprio | Escolher a tela na UI do TopScreen; áudio pelo canal da fonte | ⬜ |
| **F7** | Paridade Wayland: portal `GlobalShortcuts` + evdev, CI 2 versões do OBS, polimento | Matriz de input verde em X11 e Wayland | ⬜ |

### Detalhe do que está implementado vs verificado

| Fase | Implementado | Verificado por testes | Verificado no OBS |
|------|--------------|-----------------------|-------------------|
| F0 | sim | — | pendente |
| F1 | sim | sim (núcleo) | pendente |
| F2 | sim | sim (núcleo) | pendente |
| F3 | sim (som = stub) | sim (núcleo) | pendente |
| F4 | sim | sim (núcleo) | pendente |

## Metas transversais

- 0,5–1,5 ms de GPU/frame @1080p60 no preset padrão.
- < 1% de CPU ocioso.
- Sem alocações no caminho quente de render.
- Sem readback de frame.

## Marcos de qualidade

- **M1 (após F3)**: plugin utilizável de ponta a ponta para captura "bonita" ao vivo.
- **M2 (após F5)**: paridade de features com a lista de efeitos ao vivo.
- **M3 (após F7)**: paridade de plataforma Windows/X11/Wayland.

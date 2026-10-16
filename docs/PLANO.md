# TopScreen — Plano

> Documento-mestre. Reúne objetivo, decisões, escopo e critérios.

## 1. Objetivo

Gerar um **objeto de captura de vídeo ao vivo** a partir de telas, com efeitos
visuais, que é consumido **exclusivamente pelo OBS** como fonte/cena. Não grava,
não edita, não exporta. Todos os efeitos são **automáticos ou por atalho**.

### Restrições não-negociáveis

- **Só OBS**: entrega como fonte nativa do OBS.
- **Ao vivo**: nenhum efeito de pós-produção (sem timeline, sem corte, sem keyframe manual).
- **Leve**: roda em hardware modesto; GPU, zero-copy, sem encode.
- **Portátil**: distribuição como arquivo de plugin para OBS portable.
- **Windows + Linux** com paridade (X11 e Wayland).
- **Atalhos** fáceis de configurar, no formato do OBS.

## 2. Decisão de arquitetura

| Opção | Veredito |
|-------|----------|
| **A. Plugin OBS nativo (libobs)** | ✅ **Escolhido** |
| B. App standalone + ponte GPU | Rejeitado (2 partes, mais risco) |
| C. App standalone + NDI | Rejeitado (pesado, runtime externo, não-portátil) |
| D. App standalone + Spout2 | Rejeitado (só Windows) |

**Por que A**: vira fonte nativa (sem IPC), processa na GPU no processo do OBS,
distribui-se como `.dll`/`.so`, ganha a UI nativa de Propriedades e Atalhos, e o
`libobs` abstrai D3D11/OpenGL — mesmo código em Windows e Linux.

**Forma inicial**: **filtro** (`OBS_SOURCE_TYPE_FILTER`) aplicado sobre uma
Captura de Tela do OBS — reaproveita a captura testada do OBS e entrega valor
rápido. A **fonte de captura própria** vem em F6.

**Licença**: GPL-2.0-or-later (plugins que linkam `libobs`). Projeto aberto.

## 3. Decisões de produto

| Tema | Decisão |
|------|---------|
| UI de configuração | Propriedades nativas do OBS **+ dock Qt** (presets/toggles) |
| Input no Wayland | **Tiers 2+3**: hotkeys por portal `GlobalShortcuts` + cliques/teclas por `evdev` opt-in |
| Licença | GPL-2.0-or-later |
| Alvo de Linux | X11 e Wayland como cidadãos de primeira classe |
| Preview | O canvas do OBS (o dock é controle, não preview) |

## 4. Escopo de efeitos

Entram **somente** efeitos viáveis ao vivo. A lista completa e a justificativa de
cada um está em [`EFEITOS.md`](EFEITOS.md).

**Entram**: fundo/cantos/sombra/padding, cursor (estilo/tamanho/sombra/sway/
idle/loop), clique L/R (orbe/onda/anel/ripple/força + cor/tamanho/duração/curva),
som de clique, zoom automático (clique/cursor) e manual, pan leve, motion blur,
overlay de teclas (SDF), spotlight, webcam bubble, presets por atalho.

**Ficam de fora**: timeline, cortes, speed ramps, keyframes manuais, exportação/
transcode, normalização de áudio, realce de texto real por acessibilidade
(substituído por marca-texto de arrasto).

## 5. Modelo "automático vs atalho"

Cada recurso tem um **modo**:

- `off` — desligado
- `auto` — reage sozinho (ex.: zoom segue o clique)
- `hotkey` — só dispara por atalho
- `always` — sempre ligado (ex.: fundo)

Isso responde diretamente ao requisito "ou automático ou configurável por atalho".

## 6. Metas de qualidade

- **GPU**: 0,5–1,5 ms/frame a 1080p60 para o conjunto padrão de efeitos.
- **CPU ocioso**: < 1%.
- **Sem alocações por frame** no caminho de render.
- **Sem readback** de frame para CPU.

## 7. Riscos

| Risco | Mitigação |
|-------|-----------|
| Hotkeys globais no Wayland | Portal `GlobalShortcuts` (Tier 2) + fallback WebSocket |
| Input passivo no Wayland | `evdev` opt-in (Tier 3) + degradação graciosa |
| wlroots sem portal de atalhos | Fallback para uso com OBS em foco / WebSocket |
| ABI do OBS entre versões | CI contra 2 versões do OBS |
| DPI/multi-monitor | Mapeamento global→canvas centralizado no módulo de input |
| Permissão `evdev` | Documentar udev `uaccess` como caminho recomendado |

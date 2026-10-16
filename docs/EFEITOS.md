# Catálogo de efeitos

Regra de ouro: **só entra o que funciona ao vivo**. Efeitos de pós-produção
(timeline, cortes, keyframes manuais) não existem aqui.

Status: ✅ implementado · 🟡 parcial · ⬜ planejado.
Modo: `auto` reage sozinho · `hotkey` só por atalho · `always` sempre ligado.

## Cursor

| Recurso | Modo | Status | Observações |
|---------|------|--------|-------------|
| Substituir o ponteiro (desliga o nativo) | auto | ✅ | desliga `capture_cursor`/`show_cursor` na fonte-pai |
| **12 estilos de cursor** | auto | ✅ | macOS, Windows, Breeze, Bibata, Adwaita, GoogleDot, Pointer, DMZ, Yaru, XCursorPro, BibataIce, BibataOriginal |
| Tamanho | auto | ✅ | 0.5–4× |
| Sombra | auto | ✅ | |
| Suavização do movimento | auto | ✅ | mola criticamente amortecida |
| Sway (inclina na curva) | auto | ✅ | |
| Ocultar quando parado | auto | ✅ | com delay |
| Loop (volta ao início) | off | ✅ | |

## Cliques

| Recurso | Modo | Status | Observações |
|---------|------|--------|-------------|
| Efeito do botão esquerdo | auto | ✅ | onda / orbe / anel / força + cor/tamanho/duração/curva |
| Efeito do botão direito | auto | ✅ | independente do esquerdo |
| Som de clique | auto | ✅ | toca no áudio do sistema |

## Enquadramento

| Recurso | Modo | Status | Observações |
|---------|------|--------|-------------|
| Fundo (cor/gradiente) | sempre | ✅ | sólido, vertical, diagonal |
| Fundo por imagem | sempre | ✅ | 12 wallpapers embutidos ou imagem do seu PC |
| Cantos arredondados | sempre | ✅ | |
| Espaçamento (padding X e Y) | sempre | ✅ | eixos separados |
| Sombra da janela | sempre | ✅ | opacidade, desfoque e deslocamento |
| Inset / borda interna | off | ⬜ | |

## Zoom / Pan

| Recurso | Modo | Status | Observações |
|---------|------|--------|-------------|
| Zoom automático (clique/cursor) | auto | ✅ | volta ao normal no idle |
| Zoom manual in/out/reset | hotkey | ✅ | |
| Pan leve seguindo o mouse | auto | ✅ | |
| Zoom em região selecionada | hotkey | ⬜ | |

## Movimento e atenção

| Recurso | Modo | Status | Observações |
|---------|------|--------|-------------|
| Motion blur (do zoom) | auto | ✅ | blur radial com 3 amostras |
| Motion blur (do cursor) | auto | ⬜ | |
| Spotlight seguindo o cursor | hotkey | ✅ | escurece fora do círculo |

## Texto e destaque

| Recurso | Modo | Status | Observações |
|---------|------|--------|-------------|
| Overlay de teclas | auto/hotkey | ✅ | atlas TTF (fonte/cor/tamanho/sombra) + posição X/Y + faixa de fundo |
| Marca-texto de arrasto | auto | ✅ | região arrastada com hold/fade |

> O **realce do texto realmente selecionado** (via acessibilidade) segue fora do
> escopo — o substituto é o **marca-texto de arrasto**.

## Câmera e extras

| Recurso | Modo | Status | Observações |
|---------|------|--------|-------------|
| Webcam bubble | sempre | ⬜ | exige fonte própria (F6) |
| Countdown / HUD de atalhos | hotkey | ⬜ | |

## Fora de escopo (pós-produção)

Timeline, cortes, speed ramps, keyframes manuais, transições entre takes,
exportação MP4/GIF, normalização de áudio da gravação.

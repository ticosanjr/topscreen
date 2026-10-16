# Catálogo de efeitos

Regra de ouro: **só entra o que funciona ao vivo**. Efeitos de pós-produção
(timeline, cortes, keyframes manuais) ficam de fora por definição.

Legenda de modo: `auto` reage sozinho · `hotkey` só por atalho · `always` sempre ligado.

## Cursor

| Recurso | Modo | Parâmetros |
|---------|------|------------|
| Trocar ponteiro (estilo macOS/touch) | auto | `style`, `size` |
| Sombra do cursor | auto | `shadow`, `shadow_alpha` |
| Suavização do movimento | auto | `smooth` (0..1) |
| Sway (inclina na curva) | auto | `sway` (0..1) |
| Ocultar quando parado | auto | `hide_idle`, `delay_ms` |
| Loop do cursor (recomeça onde começou) | off | `loop`, `loop_ms` |
| Sempre usar cursor do sistema | off | `always_default` |

## Cliques

| Recurso | Modo | Parâmetros |
|---------|------|------------|
| Efeito do **botão esquerdo** | auto | `effect` (orbe/onda/anel/ripple/força), `size`, `color`, `duration_ms`, `easing` |
| Efeito do **botão direito** | auto | idem, independente do esquerdo |
| Som de clique | auto | `enabled`, `file`, `volume`, `buttons[]` |

## Enquadramento

| Recurso | Modo | Parâmetros |
|---------|------|------------|
| Fundo (cor/gradiente/imagem) | sempre | `mode`, `color_a`, `color_b`, `image`, `blur` |
| Cantos arredondados | sempre | `corner_radius` |
| Padding | sempre | `padding` |
| Sombra | sempre | `shadow`, `shadow_alpha`, `shadow_blur`, `shadow_offset` |
| Inset (borda interna) | off | `inset`, `inset_color` |

## Zoom / Pan

| Recurso | Modo | Parâmetros |
|---------|------|------------|
| Zoom automático (por clique, depois cursor) | auto | `amount`, `duration_ms`, `easing`, `idle_timeout_ms`, `follow` |
| Pan leve seguindo o mouse | auto | `pan_amount`, `pan_smooth` |
| Zoom manual in/out/reset | hotkey | `step`, `speed`, `easing` |
| Zoom em região | hotkey | `region`, `amount` |

## Movimento e atenção

| Recurso | Modo | Parâmetros |
|---------|------|------------|
| Motion blur | auto | `enabled`, `amount` (cursor / zoom separados) |
| Spotlight (foco) seguindo o cursor | hotkey | `radius`, `dim` |
| Escurecer fora do foco | hotkey | `dim` |

## Texto e destaque

| Recurso | Modo | Parâmetros |
|---------|------|------------|
| Overlay de teclas (pill) | hotkey/auto | `show` (modificadores/tudo), `style`, `font`, `ttl_ms`, `position` |
| Marca-texto de arrasto | auto | `color`, `thickness`, `style` |

> **Realce do texto realmente selecionado** (via acessibilidade/UIA/AT-SPI) está
> fora do v1 por ser inviável de forma confiável ao vivo. O substituto viável é o
> **marca-texto de arrasto**. Reavaliação futura.

## Câmera e extras

| Recurso | Modo | Parâmetros |
|---------|------|------------|
| Webcam bubble (PiP) | sempre | `shape`, `corner`, `scale`, `mirror` |
| Contagem / HUD de atalhos | hotkey | `position`, `style` |

## Fora de escopo (pós-produção)

Timeline, cortes, speed ramps por segmento, keyframes manuais, transições entre
takes, exportação MP4/GIF, normalização de áudio da gravação.

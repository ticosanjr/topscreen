# Referência de configuração

Valores persistidos pelo OBS (por fonte/cena) e usados nos arquivos de preset.
O manual de uso está em [`MANUAL.md`](MANUAL.md).

## Cores

Formato do OBS: inteiro **`0xAABBGGRR`** (byte baixo = vermelho).
Ex.: `0xFF3A2E5C` = A=FF, B=3A, G=2E, R=5C.

## Todas as opções

| Grupo (UI) | Opção (UI) | Chave | Tipo | Faixa | Padrão |
|------------|------------|-------|------|-------|--------|
| — | Efeitos habilitados | `effects_enabled` | bool | — | `true` |
| Plano de fundo | Ativar fundo | `bg_enabled` | bool | — | `true` |
| | Estilo | `bg_mode` | int | 0 sólido · 1 vertical · 2 diagonal | `2` |
| | Cor A | `bg_color_a` | cor | — | `0xFF20242E` |
| | Cor B | `bg_color_b` | cor | — | `0xFF3A2E5C` |
| | Wallpaper | `bg_image` | string | "" ou `wallpapers/*.jpg` ou caminho absoluto | `""` |
| | Imagem do PC | `bg_image_file` | string | caminho de arquivo (tem prioridade) | `""` |
| | Espaçamento horizontal (px) | `bg_padding_x` | double | 0–400 | `64` |
| | Espaçamento vertical (px) | `bg_padding_y` | double | 0–400 | `64` |
| | Manter proporção da tela | `bg_keep_aspect` | bool | — | `true` |
| | Cantos arredondados (px) | `bg_corner_radius` | double | 0–200 | `18` |
| | Sombra da janela | `bg_shadow` | bool | — | `true` |
| | Opacidade da sombra | `bg_shadow_alpha` | double | 0–1 | `0.35` |
| | Desfoque da sombra (px) | `bg_shadow_blur` | double | 0–200 | `48` |
| | Deslocamento vertical da sombra (px) | `bg_shadow_offset` | double | -100–100 | `12` |
| Cursor | Mostrar cursor | `cursor_enabled` | bool | — | `true` |
| | Estilo | `cursor_style` | int | 0 macOS · 1 Windows · 2 Breeze · 3 Bibata · 4 Adwaita · 5 GoogleDot · 6 Pointer · 7 DMZ · 8 Yaru · 9 XCursorPro · 10 BibataIce · 11 BibataOriginal | `0` |
| | Tamanho | `cursor_size` | double | 0.5–4 | `1.4` |
| | Sombra do cursor | `cursor_shadow` | bool | — | `true` |
| | Suavização | `cursor_smooth` | double | 0–1 | `0.7` |
| | Inclinação na curva | `cursor_sway` | double | 0–1 | `0.3` |
| | Ocultar quando parado | `cursor_hide_idle` | bool | — | `true` |
| | Delay para ocultar (ms) | `cursor_idle_delay_ms` | double | 0–5000 | `1200` |
| | Voltar ao início quando parado | `cursor_loop` | bool | — | `false` |
| | Tempo para voltar (ms) | `cursor_loop_ms` | double | 1000–20000 | `4000` |
| Zoom | Modo | `zoom_mode` | int | 0 off, 1 auto, 2 manual | `1` |
| | Intensidade | `zoom_amount` | double | 1–4 | `1.6` |
| | Velocidade (ms) | `zoom_duration_ms` | double | 100–2000 | `450` |
| | Tempo sem foco (ms) | `zoom_idle_timeout_ms` | double | 200–10000 | `1500` |
| | Intensidade do pan | `zoom_pan_amount` | double | 0–1 | `0.25` |
| Clique esquerdo | Efeito | `click_left_effect` | int | 0 onda, 1 orbe, 2 anel, 3 força | `0` |
| | Tamanho | `click_left_size` | double | 0.1–4 | `1.0` |
| | Cor | `click_left_color` | cor | — | `0xFFFFFFFF` |
| | Duração (ms) | `click_left_duration_ms` | double | 100–2000 | `650` |
| | Curva | `click_left_easing` | int | 0 linear, 1 in, 2 out, 3 in-out | `2` |
| Clique direito | Efeito | `click_right_effect` | int | idem | `1` |
| | Tamanho | `click_right_size` | double | 0.1–4 | `1.2` |
| | Cor | `click_right_color` | cor | — | `0xFF7C5CFF` |
| | Duração (ms) | `click_right_duration_ms` | double | 100–2000 | `700` |
| | Curva | `click_right_easing` | int | idem | `2` |
| Som de clique | Ativar som | `click_sound_enabled` | bool | — | `true` |
| | Arquivo | `click_sound_file` | string | — | `sounds/click-soft.wav` |
| | Volume | `click_sound_volume` | double | 0–1 | `0.6` |
| | No clique esquerdo | `click_sound_left` | bool | — | `true` |
| | No clique direito | `click_sound_right` | bool | — | `false` |
| Movimento | Desfoque de movimento | `motion_blur_enabled` | bool | — | `true` |
| | Intensidade | `motion_blur_amount` | double | 0–1 | `0.35` |
| Spotlight | Ativar spotlight | `spotlight_enabled` | bool | — | `false` |
| | Raio | `spotlight_radius` | double | 0.05–1 | `0.30` |
| | Escurecimento | `spotlight_dim` | double | 0–1 | `0.60` |
| Teclas | Mostrar teclas digitadas | `keys_enabled` | bool | — | `true` |
| | Somente modificadores | `keys_modifiers_only` | bool | — | `false` |
| | Tempo na tela (ms) | `keys_ttl_ms` | double | 500–8000 | `2000` |
| | Máximo de teclas | `keys_max` | int | 1–12 | `6` |
| | Posição X | `keys_pos_x` | double | 0–1 | `0.5` |
| | Posição Y | `keys_pos_y` | double | 0–1 | `0.93` |
| | Tamanho do texto | `keys_size` | double | 0.4–4 | `1.0` |
| | Fonte | `keys_font` | string | `fonts/*.ttf` ou caminho | `fonts/DejaVuSans-Bold.ttf` |
| | Cor do texto | `keys_color` | cor | — | `0xFFFFFFFF` |
| | Sombra do texto | `keys_shadow` | bool | — | `true` |
| | Faixa de fundo | `keys_band` | bool | — | `true` |
| | Cor da faixa | `keys_band_color` | cor | — | `0x99000000` |
| Marca-texto | Ativar marca-texto | `highlight_enabled` | bool | — | `true` |
| | Cor | `highlight_color` | cor | — | `0x6600E0FF` |
| | Tempo na tela (ms) | `highlight_hold_ms` | double | 0–5000 | `900` |
| | Desvanecer (ms) | `highlight_fade_ms` | double | 0–2000 | `300` |

## Atalhos

IDs registrados (aparência em **Configurações → Atalhos → TopScreen**):

| ID | Ação |
|----|------|
| `TopScreen.ToggleEffects` | Ativar/desativar efeitos |
| `TopScreen.ToggleBackground` | Ativar/desativar fundo |
| `TopScreen.ZoomIn` | Zoom in |
| `TopScreen.ZoomOut` | Zoom out |
| `TopScreen.ResetZoom` | Resetar zoom |
| `TopScreen.NextPreset` | Próximo preset |
| `TopScreen.PrevPreset` | Preset anterior |

## Formato de preset

Arquivos em `data/presets/*.json`. O bloco `settings` usa as mesmas chaves da
tabela acima; chaves ausentes mantêm o valor atual.

```json
{
  "name": "Demo",
  "description": "Fundo em gradiente, cursor destacado e zoom automatico.",
  "settings": {
    "effects_enabled": true,
    "bg_enabled": true,
    "bg_mode": 2,
    "bg_color_a": 4280296494,
    "bg_color_b": 4282003036,
    "bg_padding": 64,
    "bg_corner_radius": 18,
    "bg_shadow": true,
    "bg_shadow_alpha": 0.35,
    "bg_shadow_blur": 48,
    "cursor_enabled": true,
    "cursor_size": 1.6,
    "cursor_smooth": 0.8,
    "cursor_sway": 0.4,
    "zoom_mode": 1,
    "zoom_amount": 1.6,
    "click_left_effect": 0,
    "click_right_effect": 1,
    "click_sound_enabled": false,
    "motion_blur_enabled": true,
    "spotlight_enabled": false,
    "keys_enabled": true,
    "keys_ttl_ms": 2000,
    "highlight_enabled": true
  }
}
```

Aplicar um preset = escrever `settings` no `obs_data` do filtro. Trocar de
preset pelos atalhos *Próximo/Anterior preset* ou pelo dock.

## Estado de sessão (não persistido)

- Toggle de efeitos/fundo via atalho (volta ao padrão ao reiniciar).
- Zoom manual (in/out/reset) via atalho.

## Tiers de input

| Tier | Significado |
|------|-------------|
| Nativo | Windows (WinAPI) ou Linux/X11 (cursor + teclas) |
| evdev | Linux/Wayland: teclas via `/dev/input` |
| Portal | Wayland via `GlobalShortcuts` (planejado) |
| Indisponível | Sem backend; efeitos de input ficam inertes |

Veja [`INPUT-E-PLATAFORMAS.md`](INPUT-E-PLATAFORMAS.md).

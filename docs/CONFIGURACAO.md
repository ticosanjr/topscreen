# Configuração, presets e atalhos

## Modelo

- **Configurações da fonte/filtro**: armazenadas pelo OBS (`obs_data`, JSON por
  cena/coleção). Persistem automaticamente.
- **Presets TopScreen**: arquivos `data/presets/*.json`, aplicáveis por atalho.
- **Estado de sessão** (toggles de atalho como "efeitos ligados/desligados") não
  é persistido — volta ao padrão ao reiniciar.

## Esquema de configuração

```json
{
  "preset": "Demo",
  "effects_enabled": true,
  "background": {
    "enabled": true,
    "mode": 2,
    "color_a": 4280296494,
    "color_b": 4282003036,
    "padding": 64,
    "corner_radius": 18,
    "shadow": true,
    "shadow_alpha": 0.35,
    "shadow_blur": 48
  },
  "cursor": {
    "enabled": true,
    "style": "macos",
    "size": 1.4,
    "shadow": true,
    "hide_idle": { "enabled": true, "delay_ms": 1200 },
    "smooth": 0.7,
    "sway": 0.3
  },
  "click": {
    "left":  { "effect": "ripple", "size": 1.0, "color": 4294967295, "duration_ms": 650, "easing": "easeOut" },
    "right": { "effect": "orb",    "size": 1.2, "color": 4286321151, "duration_ms": 700, "easing": "easeOut" },
    "sound": { "enabled": true, "file": "click-soft.wav", "volume": 0.6, "buttons": ["left"] }
  },
  "zoom": {
    "mode": "auto",
    "amount": 1.6,
    "duration_ms": 450,
    "easing": "easeInOutCubic",
    "idle_timeout_ms": 1500,
    "pan_amount": 0.25
  },
  "keys": {
    "enabled": true,
    "show": "modifiers_and_keys",
    "style": "pill",
    "font": "Inter-SemiBold",
    "ttl_ms": 2000,
    "position": "bottom_center"
  },
  "motion_blur": { "enabled": true, "amount": 0.35 }
}
```

> **Cores**: armazenadas no formato do OBS (inteiro `0xAABBGGRR`), consistente com
> `obs_properties_add_color` e `gs_effect_set_color`.

## Modos por recurso

Cada recurso aceita `off | auto | hotkey | always`. Ver
[`EFEITOS.md`](EFEITOS.md) para o modo padrão de cada item.

## Atalhos (aba nativa do OBS)

| ID | Ação | Padrão sugerido |
|----|------|-----------------|
| `TopScreen.ToggleEffects` | Ligar/desligar todos os efeitos | — |
| `TopScreen.ToggleBackground` | Ligar/desligar fundo + cantos | — |
| `TopScreen.ZoomIn` | Zoom in | — |
| `TopScreen.ZoomOut` | Zoom out | — |
| `TopScreen.ResetZoom` | Resetar zoom | — |
| `TopScreen.NextPreset` | Próximo preset | — |
| `TopScreen.PrevPreset` | Preset anterior | — |
| `TopScreen.ToggleKeys` | Overlay de teclas on/off | — |
| `TopScreen.ToggleSpotlight` | Spotlight on/off | — |
| `TopScreen.ToggleClickSound` | Som de clique on/off | — |

Os atalhos aparecem em **Configurações → Atalhos**, agrupados sob o filtro.

### Wayland

No Wayland, os atalhos globais são espelhados no portal
`org.freedesktop.portal.GlobalShortcuts` (Tier 2), pois o OBS não captura
atalhos globais nativamente fora do X11. Ver
[`INPUT-E-PLATAFORMAS.md`](INPUT-E-PLATAFORMAS.md).

## Presets embutidos

| Preset | Descrição |
|--------|-----------|
| `demo` | Fundo gradiente, cantos, zoom automático, cursor grande |
| `tutorial` | Fundo sólido, overlay de teclas ligado, som de clique |
| `apresentacao` | Fundo escuro, zoom desligado, spotlight disponível |

### Formato de arquivo de preset

Cada arquivo em `data/presets/*.json` tem um envelope com metadados e o bloco
`settings` (que contém as mesmas chaves do `obs_data`):

```json
{
  "name": "Demo",
  "description": "Fundo em gradiente, cantos arredondados e sombra suave.",
  "settings": {
    "effects_enabled": true,
    "bg_enabled": true,
    "bg_mode": 2,
    "bg_color_a": 4280296494,
    "bg_corner_radius": 18,
    "zoom": 1.0
  }
}
```

Aplicar um preset = escrever `settings` no `obs_data` do filtro via
`obs_source_update`. O carregamento/atalho `NextPreset`/`PrevPreset` entra em T3.7.

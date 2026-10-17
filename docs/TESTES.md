# Testes

## Duas camadas de verificação

1. **Testes do núcleo** (`./tests/run.sh`) — lógica pura, roda em qualquer lugar.
2. **Checagem contra os headers do OBS** (`./tools/check-obs-headers.sh`) —
   compila cada fonte do plugin contra os headers **reais** do OBS (via
   `-fsyntax-only`, sem libobs instalado). Pega erro de API/assinatura/tipo que
   os testes de núcleo não pegam. O dock (`src/ui`) é checado com Qt6 real
   quando disponível, ou com o stub em `tools/qt-stub`. Roda no CI
   (`.github/workflows/check-obs-headers.yml`).

## Como rodar

**Camada 1 — núcleo (sem cmake/libobs):**

```bash
./tests/run.sh
```

**Camada 2 — headers do OBS (baixa os headers automaticamente):**

```bash
./tools/check-obs-headers.sh
# X11 presente? o backend X11 também é checado; senão, é ignorado.
```

**Via CMake/CTest:**

```bash
cmake -S . -B build -DTOPSCREEN_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

**CI:** `.github/workflows/tests.yml` (núcleo, Linux+Windows) e
`check-obs-headers.yml` (Linux, com X11) a cada push/PR.

## O que é testado

Os testes cobrem o **núcleo puro** (`src/core/`), que não depende do OBS. É onde
vive toda a matemática e a lógica de estado — o que de fato precisa estar certo.

| Arquivo | Alvo | Cobertura |
|---------|------|-----------|
| `test_color.cpp` | `core/color.hpp` | unpack/pack ABGR roundtrip, interpolação e recorte |
| `test_easing.cpp` | `core/easing.*` | extremos, recorte de domínio, monotonicidade, simetria |
| `test_spring.cpp` | `core/spring.*` | convergência, ausência de overshoot, reset, estabilidade com dt grande |
| `test_geometry.cpp` | `core/geometry.*` | SDF de retângulo arredondado, máscara com AA, transform de zoom, pixel→conteúdo, clamp de foco |
| `test_coord.cpp` | `core/coord.*` | mapeamento tela↔conteúdo, limites do monitor, casos inválidos |
| `test_cursor.cpp` | `core/cursor.*` | posição inicial, perseguição de alvo, idle/fade, loop, desabilitado |
| `test_click_fx.cpp` | `core/click_fx.*` | spawn, progresso, expiração, L/R distintos, reciclagem de slots, saturação |
| `test_zoom.cpp` | `core/zoom.*` | passo manual e limites, auto com foco recente, retorno no idle, modo off, clamp de foco |
| `test_json.cpp` | `core/json.*` | objeto/array/aninhado, números, escapes (`\uXXXX`), erros |
| `test_preset.cpp` | `core/preset.*` | parse + apply, campos ausentes mantidos, erros, **arquivos reais** `data/presets/*.json` |
| `test_shader_params.cpp` | `data/effects/topscreen.effect` | presença dos uniforms que o compositor busca por nome + técnica `Draw` (pega divergência silenciosa) |
| `test_preset_all_keys.cpp` | `core/preset.*` | todas as chaves de config são mapeadas (garante preset completo) |
| `test_locale_keys.cpp` | `data/locale/*.ini` | toda chave usada em `obs_module_text()` existe no `en-US.ini` + paridade pt/en |
| `test_key_overlay.cpp` | `core/key_overlay.*` | fila de teclas, TTL, repetição, filtro de modificadores, limite, formatação de combos |
| `test_wav.cpp` | `core/wav.*` | PCM 8/16/24/32 e float, estéreo, erros, e **arquivos reais** `data/sounds/*.wav` |

| `test_font_atlas.cpp` | `core/font_atlas.*` | atlas gerado do **TTF real**, métricas por glifo, alpha, casos inválidos |
| `test_evdev.cpp` | `core/evdev.*` | decodificação de `input_event`, labels, modificadores, registros parciais |
| `test_highlighter.cpp` | `core/highlighter.*` | arrasto, retângulo normalizado, hold/fade, arrasto pequeno, desabilitado |
| `test_docs.cpp` | `docs/` × código | toda chave de `keys.hpp` está documentada; atalhos do código estão na referência |
| `test_cursor_atlas.cpp` | `core/cursor_atlas.*` + `data/cursors/` | formato TSAT, hotspots e **atlas real** (12 estilos = `CursorStyle::Count`) |
| `test_image.cpp` | `core/image.*` + `data/wallpapers/` | decode de imagem (lê um **wallpaper real** 1920x1080) e casos inválidos |
| `test_mapping.cpp` | `core/mapping.*` | área de conteúdo (padding/proporção), zoom para frente e **consistência com o shader** (prova que o cursor cai no pixel certo em qualquer zoom) |

## Números atuais

```
112 testes | 2255 verificações | 0 falhas   (núcleo)
7 testes  | 46 verificações   | 0 falhas   (integração com libobs fake)
```

## O que NÃO é coberto (e por quê)

Estes itens dependem de `libobs`/GPU/OS em execução e não rodam aqui:

| Item | Por que não testado aqui | Como verificar |
|------|--------------------------|----------------|
| Shader `topscreen.effect` | precisa de contexto OpenGL/D3D do OBS | carregar o filtro no OBS e conferir visualmente |
| Integração `obs_source_info` | precisa do OBS em execução | `obs --verbose` e checar o log do TopScreen |
| Backends de input | precisam de WinAPI/X11 reais | testar cursor/cliques no OBS |
| Dock Qt | **compila** (stub/Qt real), mas o runtime precisa do OBS | abrir o dock no OBS |
| Som de clique | ainda é stub (T3.6) | — |

> A **compilação** do plugin contra os headers do OBS é checada (camada 2);
> o que falta é a **execução** dentro do OBS.

A estratégia é: **empurrar a complexidade para o núcleo testável** e manter a
camada de OBS fina.

## Adicionando testes

1. Crie `tests/test_<assunto>.cpp`.
2. Use as macros de `tests/test_framework.hpp` (`TEST`, `CHECK`, `CHECK_NEAR`,
   `CHECK_EQ`, `REQUIRE`).
3. Rode `./tests/run.sh`. O CMake pega o arquivo automaticamente (`GLOB`).

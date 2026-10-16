# Contribuindo com o TopScreen

Obrigado pelo interesse! Este projeto é um plugin de efeitos ao vivo para OBS.

## Princípios

1. **Só ao vivo.** Nada de recursos de pós-produção (timeline, corte, keyframe
   manual, export). Se um efeito só faz sentido depois de gravar, ele não entra.
2. **Leve por padrão.** GPU, sem encode, sem readback, sem alocação por frame.
3. **Núcleo testável.** A lógica vai para `src/core/` (sem OBS) e ganha teste. A
   camada OBS (`src/`, fora de `core/`) deve ser fina.
4. **Portátil.** Windows + Linux, X11 + Wayland.

## Estrutura

- `src/core/` — núcleo puro, testado. Não inclua `<obs-module.h>` aqui.
- `src/` — camada OBS (filtro, compositor, settings, input, dock).
- `data/` — shader, locales, presets, udev.
- `tests/` — testes do núcleo.

## Fluxo

1. Crie um branch.
2. Implemente a mudança.
3. **Rode os testes:** `./tests/run.sh` (precisa passar).
4. **Rode a checagem de headers:** `./tools/check-obs-headers.sh` (compila o
   plugin contra os headers reais do OBS, sem precisar instalá-lo).
5. Se mexeu na lógica, adicione/atualize testes em `tests/`.
6. Se mexeu em efeito/escopo, atualize `docs/EFEITOS.md`, `docs/TAREFAS.md`.
7. Abra o PR descrevendo o que mudou e como verificar.

## Estilo de código

- Formatação: `.clang-format` (rode `clang-format -i`).
- C++20, 4 espaços, `namespace topscreen` (+ `::core` para o núcleo).
- Nomes: `snake_case` para funções/variáveis, `PascalCase` para tipos.
- Comentários em português (o projeto é PT-BR no texto; código/identificadores em inglês).

## Adicionando uma configuração

1. Adicione a chave em `src/core/keys.hpp`.
2. Adicione o campo em `src/core/config.hpp`.
3. Mapeie em `src/ts-settings.cpp` (`config_defaults`, `config_load`, `config_store`).
4. Exponha na UI em `src/ts-properties.cpp` + textos em `data/locale/*.ini`.
5. Suporte no preset em `src/core/preset.cpp` e nos `data/presets/*.json`.

## Adicionando um teste

```cpp
#include "core/meu_modulo.hpp"
#include "test_framework.hpp"

TEST(meu_modulo_caso) {
    CHECK(alguma_coisa());
    CHECK_NEAR(valor, 1.0, 1e-9);
}
```

## Build do plugin

Veja `docs/BUILD.md`. Você precisa de libobs (+ obs-frontend-api/Qt6 para o dock).

## Licença

Ao contribuir você concorda que sua contribuição é licenciada sob
GPL-2.0-or-later (ver `COPYING`).

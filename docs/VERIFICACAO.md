# Verificação em um OBS real

Tudo que é verificável **sem** OBS está coberto por:
[`TESTES.md`](TESTES.md) (núcleo + compilação contra headers + integração).
Este documento é o **roteiro manual** para validar o que só o OBS pode provar.

## 1. Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release     # ver docs/BUILD.md
cmake --build build -j
# portable: copie build/topscreen.so (ou topscreen.dll) e a pasta data/ para o OBS
obs --verbose 2>&1 | grep -i topscreen
```

Esperado no log: `[TopScreen] carregado v...`, `filtro criado (presets=3)`,
`som carregado: sounds/click-soft.wav`, `evdev: usando /dev/input/eventN`
(ou aviso de permissão).

## 2. Roteiro de testes (visual)

Adicione uma **Captura de Tela** e o filtro **TopScreen FX**. Depois:

| # | O que testar | Esperado |
|---|--------------|----------|
| 1 | Filtro passthrough (efeitos desligados) | imagem idêntica à captura |
| 2 | Fundo + cantos + padding + sombra | tela "flutuando" com fundo e cantos |
| 3 | Cursor | cursor nativo some; aparece a seta própria (clara com borda escura) |
| 4 | Tamanho/suavização/sway do cursor | muda com os sliders |
| 5 | Clique esquerdo | efeito de onda/orbe no ponto do clique |
| 6 | Clique direito | efeito distinto do esquerdo |
| 7 | Som de clique | som no clique (se player disponível) |
| 8 | Zoom automático | ao clicar, dá zoom e volta após parar |
| 9 | Zoom manual (atalhos) | in/out/reset funcionam |
| 10 | Motion blur | leve borrão radial durante o zoom |
| 11 | Overlay de teclas | teclas aparecem na base e somem após o TTL |
| 12 | Spotlight | escurece tudo menos o círculo no cursor |
| 13 | Marca-texto | arrastar com o botão esquerdo marca a região |
| 14 | Presets (atalho) | próximo/anterior trocam a aparência |
| 15 | Dock | reflete o estado e mostra o tier de input |

## 3. Atalhos

**Configurações → Atalhos → TopScreen** deve listar as 12 ações
(`ToggleEffects`, `ToggleBackground`, `ZoomIn`, `ZoomOut`, `ResetZoom`,
`NextPreset`, `PrevPreset`, `ProfileA`, `ProfileB`, `ProfileC`, `ProfileD`,
`ProfileSave`).

## 4. Input por plataforma

| Plataforma | Esperado |
|------------|----------|
| Windows | cursor + cliques + teclas (tier Nativo) |
| Linux X11 | cursor + cliques + teclas (tier Nativo) |
| Linux Wayland | cursor pelo OBS/PipeWire; teclas via evdev (tier evdev); se `evdev` indisponível, instale a regra udev (`data/udev/99-topscreen.rules`) |

Para testar o overlay sem back-end de input: defina `TOPSCREEN_DEBUG_KEY=Ctrl`.

## 5. Se algo não funcionar

- **Filtro passa direto (sem efeito)**: o `.effect` não carregou — veja o log
  (`falha ao criar o efeito`). Confirme que `data/effects/topscreen.effect` está
  no diretório de dados do módulo.
- **Dock ausente**: build sem Qt6/obs-frontend-api (o configure imprime
  `TopScreen: Qt=... frontend=...`).
- **Sem teclas no Wayland**: permissão em `/dev/input` (regra udev/grupo input).
- **Texto do overlay torto**: sistema de matriz do libobs — reporte com print.

## 6. Reportando

Inclua: versão do OBS, SO/servidor gráfico, trecho do log (`--verbose`),
e qual item da tabela falhou.

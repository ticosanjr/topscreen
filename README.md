# TopScreen

**Efeitos de captura de tela ao vivo para OBS Studio.**

O TopScreen é um *plugin de filtro* do OBS que aplica efeitos ao vivo sobre
qualquer captura de tela — cursor estilizado, efeitos de clique, zoom/pan
automáticos, fundo com cantos arredondados, overlay de teclas, spotlight e
webcam — **sem gravar nada e sem edição posterior**. Tudo é automático ou
disparado por atalhos, e o resultado é consumido diretamente pelo OBS.

> Não é um gravador. Não gera arquivo. Não tem timeline. Existe para ser usado
> **junto com o OBS**, como uma fonte/cena do próprio OBS.

---

## Como funciona (visão rápida)

1. No OBS, adicione uma **Captura de Tela** (Display Capture) e escolha a tela.
2. Ainda na fonte, em **Filtros**, adicione **TopScreen FX**.
3. Ajuste nas **Propriedades** do filtro e/ou no **dock TopScreen** (painel lateral).
4. Configure atalhos em **Configurações → Atalhos**.

O filtro roda inteiramente na GPU, no processo do OBS — sem IPC, sem encode,
sem cópia de frames. É o caminho mais leve possível para efeitos ao vivo.

---

## Estado do projeto

Este repositório está em desenvolvimento incremental por fases (F0–F7).
Veja [`docs/ROADMAP.md`](docs/ROADMAP.md) e [`docs/TAREFAS.md`](docs/TAREFAS.md).

| Fase | Escopo | Status |
|------|--------|--------|
| F0 | Skeleton do plugin + filtro passthrough | ✅ implementado |
| F1 | Fundo, cantos arredondados, padding, sombra, presets + dock inicial | 🟡 implementado (verificar no OBS) |
| F2 | Cursor (tamanho/suavização/sway/idle/loop) | 🟡 implementado (verificar no OBS) |
| F3 | Efeitos de clique L/R + atalhos + presets | 🟡 implementado (som = stub) |
| F4 | Zoom/Pan (mola) auto + manual + pan | 🟡 implementado |
| F5 | Overlay de teclas (SDF) + spotlight + webcam | ⬜ |
| F6 | Fonte de captura própria + áudio próprio | ⬜ |
| F7 | Paridade Wayland (portais + evdev) + polimento | ⬜ |

O núcleo (matemática, cursor, cliques, zoom, presets, JSON) tem **testes
automatizados** — veja [`docs/TESTES.md`](docs/TESTES.md). O shader e a
integração com o OBS ainda precisam de uma passada em um OBS real.

### Testes rápidos

```bash
./tests/run.sh     # 47 testes do núcleo, sem libobs/cmake
```

---

## Documentação

| Documento | Conteúdo |
|-----------|----------|
| [`docs/PLANO.md`](docs/PLANO.md) | Plano completo, decisões e escopo |
| [`docs/ARQUITETURA.md`](docs/ARQUITETURA.md) | Arquitetura técnica e módulos |
| [`docs/EFEITOS.md`](docs/EFEITOS.md) | Catálogo de efeitos (o que é viável ao vivo) |
| [`docs/CONFIGURACAO.md`](docs/CONFIGURACAO.md) | Modelo de configuração, presets e atalhos |
| [`docs/INPUT-E-PLATAFORMAS.md`](docs/INPUT-E-PLATAFORMAS.md) | Input por SO e paridade X11/Wayland |
| [`docs/ROADMAP.md`](docs/ROADMAP.md) | Fases e critérios de aceite |
| [`docs/TAREFAS.md`](docs/TAREFAS.md) | Quebra de tarefas executável |
| [`docs/TESTES.md`](docs/TESTES.md) | Testes do núcleo: como rodar e o que cobrem |
| [`docs/BUILD.md`](docs/BUILD.md) | Como compilar (Windows/Linux) |

---

## Requisitos

- **OBS Studio** 31.0+ (alvo 33.x)
- **Windows 10+** ou **Linux** (X11 e Wayland)
- CMake 3.20+, compilador C++20
- Qt6 (opcional, para o dock)

## Compilação (resumo)

Veja [`docs/BUILD.md`](docs/BUILD.md) para o passo a passo completo.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
cmake --install build
```

---

## Licença

GPL-2.0-or-later. Veja [`COPYING`](COPYING) (texto completo) e [`LICENSE`](LICENSE).
Plugins que linkam `libobs` precisam ser compatíveis com a GPLv2 (ver `docs/PLANO.md` §2).

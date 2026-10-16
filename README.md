# TopScreen

**Efeitos de captura de tela ao vivo para OBS Studio.**

O TopScreen é um *plugin de filtro* do OBS que aplica efeitos ao vivo sobre
qualquer captura de tela — cursor estilizado, efeitos de clique, zoom/pan
automáticos, fundo com cantos arredondados, overlay de teclas, spotlight e
marca-texto — **sem gravar nada e sem edição posterior**. Tudo é automático ou
disparado por atalhos, e o resultado é consumido diretamente pelo OBS.

> Não é um gravador. Não gera arquivo. Não tem timeline. Existe para ser usado
> **junto com o OBS**, como um filtro de uma fonte do próprio OBS.

---

## Como funciona (visão rápida)

1. No OBS, adicione uma **Captura de Tela** e escolha a tela.
2. Na fonte, em **Filtros**, adicione **TopScreen FX**.
3. Ajuste nas **Propriedades** do filtro e/ou no **dock TopScreen**.
4. Configure atalhos em **Configurações → Atalhos → TopScreen**.

O filtro roda inteiramente na GPU, no processo do OBS — sem IPC, sem encode,
sem cópia de frames.

📖 **Manual do usuário completo: [`docs/MANUAL.md`](docs/MANUAL.md).**

---

## Estado do projeto

Validado em uso real: carrega e roda no **OBS 30.0.2** (Linux, X11) — efeitos,
presets, som de clique, overlay de teclas e dock.

| Fase | Escopo | Status |
|------|--------|--------|
| F0 | Skeleton do plugin + filtro passthrough | ✅ |
| F1 | Fundo, cantos, padding, sombra, presets + dock | ✅ |
| F2 | Cursor (tamanho/suavização/sway/idle/loop) | ✅ |
| F3 | Cliques L/R + som + atalhos + presets | ✅ |
| F4 | Zoom/Pan (mola) auto + manual + pan + motion blur | ✅ |
| F5 | Overlay de teclas + spotlight + marca-texto | 🟡 (webcam ⬜) |
| F6 | Fonte de captura própria + áudio próprio | ⬜ |
| F7 | Paridade Wayland (portal + evdev) + CI | 🟡 (evdev ✅; portal ⬜) |

O núcleo tem **testes automatizados** e a camada OBS é checada contra os headers
reais do OBS. Veja [`docs/TESTES.md`](docs/TESTES.md).

### Testes rápidos

```bash
./tests/run.sh            # Linux: 89 testes do núcleo
pwsh -File tests\run.ps1  # Windows (ou PowerShell em qualquer SO)
```

---

## Requisitos

- **OBS Studio** 30.0+ (validado em 30.0.2)
- **Windows 10+** ou **Linux** (X11 e Wayland)
- CMake 3.20+, compilador C++20
- Qt6 (opcional, para o dock)

## Instalação (simples)

**Linux**
```bash
./scripts/install-linux.sh            # plugin de usuário (sem sudo)
./scripts/install-linux.sh --system   # OBS do sistema (usa sudo)
```

**Windows**
```powershell
pwsh -File scripts\install-windows.ps1                    # plugin de usuário
pwsh -File scripts\install-windows.ps1 -ObsDir D:\OBS     # OBS portable
pwsh -File scripts\install-windows.ps1 -Dll .\topscreen.dll
```

Passo a passo completo em [`docs/BUILD.md`](docs/BUILD.md).

---

## Documentação

Índice completo: [`docs/README.md`](docs/README.md).

| Documento | Conteúdo |
|-----------|----------|
| [`docs/MANUAL.md`](docs/MANUAL.md) | **Manual do usuário** (instalar, usar, atalhos, receitas, FAQ) |
| [`docs/CONFIGURACAO.md`](docs/CONFIGURACAO.md) | Referência de todas as opções, presets e atalhos |
| [`docs/EFEITOS.md`](docs/EFEITOS.md) | Catálogo de efeitos (com status) |
| [`docs/INPUT-E-PLATAFORMAS.md`](docs/INPUT-E-PLATAFORMAS.md) | Input por SO e paridade X11/Wayland |
| [`docs/BUILD.md`](docs/BUILD.md) | Compilar e instalar (Windows/Linux) |
| [`docs/VERIFICACAO.md`](docs/VERIFICACAO.md) | Roteiro de validação num OBS real |
| [`docs/TESTES.md`](docs/TESTES.md) | Testes automatizados: como rodar e o que cobrem |
| [`docs/PLANO.md`](docs/PLANO.md) · [`docs/ARQUITETURA.md`](docs/ARQUITETURA.md) · [`docs/ROADMAP.md`](docs/ROADMAP.md) · [`docs/TAREFAS.md`](docs/TAREFAS.md) | Plano, arquitetura, roadmap e tarefas |

---

## Licença

GPL-2.0-or-later. Veja [`COPYING`](COPYING) (texto completo) e [`LICENSE`](LICENSE).
Plugins que linkam `libobs` precisam ser compatíveis com a GPLv2 (ver `docs/PLANO.md` §2).

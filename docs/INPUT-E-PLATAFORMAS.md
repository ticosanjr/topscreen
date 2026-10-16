# Input e paridade entre plataformas

Este é o ponto mais delicado do projeto. **Captura de tela tem paridade total**
(o PipeWire cobre o Wayland), mas **input global passivo** (observar cursor,
cliques e teclas sem capturá-los) é onde as plataformas divergem.

## Matriz de paridade

| Recurso | Windows | Linux X11 | Linux Wayland |
|---------|---------|-----------|---------------|
| Captura de tela | DXGI / OBS | XShm / OBS | **PipeWire / OBS** ✅ |
| Posição do cursor | `GetCursorPos` | `XQueryPointer` | evdev (Tier 3) / derivado do frame |
| Cliques/botões | `GetAsyncKeyState`/hook | XInput2 raw | evdev (Tier 3) |
| Teclas (overlay) | hook de baixo nível | XRecord/XInput2 | evdev (Tier 3) |
| Atalhos globais | **OBS nativo** ✅ | **OBS nativo (X11 grab)** ✅ | **Portal `GlobalShortcuts`** (Tier 2) |

## Estratégia em tiers

- **Tier 1 — sempre**: efeitos que não dependem de input global (fundo, cantos,
  zoom manual, spotlight por atalho).
- **Tier 2 — portal**: atalhos globais no Wayland via
  `org.freedesktop.portal.GlobalShortcuts`. Funciona em **KDE Plasma 6+** e
  **GNOME 49+**. wlroots (Sway/Niri) depende do compositor. Fallback: OBS WebSocket.
- **Tier 3 — opt-in**: cliques/teclas no Wayland via **`/dev/input` (evdev /
  libinput)**. Passivo, funciona igual em X11 e Wayland, mas exige permissão
  (`uaccess` via udev — recomendado — ou grupo `input`, ou `setcap`).
- **Tier 4 — degradação**: se indisponível, os efeitos de input se desligam e o
  dock mostra como habilitar.

## Por que não usar o portal `InputCapture`

O portal `org.freedesktop.portal.InputCapture` **toma** o teclado/ponteiro (via
libei/EIS) — é feito para VMs e remote-desktop. Ele interrompe o uso normal do
input pelo usuário, então **não serve** para um overlay passivo.

## Permissão no Linux (Tier 3)

Recomendado: regra udev com `uaccess` (o `logind` concede acesso à sessão ativa):

```
# /etc/udev/rules.d/99-topscreen.rules
# (cópia em data/udev/99-topscreen.rules)
SUBSYSTEM=="input", GROUP="input"
KERNEL=="event*", ENV{ID_INPUT_KEYBOARD}=="1", TAG+="uaccess", MODE="0660"
KERNEL=="event*", ENV{ID_INPUT_MOUSE}=="1",    TAG+="uaccess", MODE="0660"
```

Alternativa simples (menos granular): adicionar o usuário ao grupo `input`.

```bash
sudo usermod -aG input "$USER"   # reentrar na sessão depois
```

O dock do TopScreen detecta a disponibilidade e informa o que falta.

## Estado atual da implementação

| Backend | Arquivo | Estado |
|---------|---------|--------|
| Windows | `src/input/ts-input-platform.cpp` | ✅ cursor + botões (WinAPI) |
| X11 | `src/input/ts-input-platform.cpp` | ✅ cursor + botões (Xlib, `TOPSCREEN_HAVE_X11`) |
| evdev | `src/input/ts-input-platform.cpp` | 🟡 stub (Tier 3, F7) |
| Portal | `src/input/ts-input-platform.cpp` | 🟡 stub (Tier 2, F7) |

## Notas técnicas

- **Coordenadas**: os backends reportam coordenadas globais de tela; a conversão
  para o espaço do canvas (posição/escala do monitor, DPI) é centralizada no
  módulo de input (F2).
- **Edges de botão**: derivados por comparação entre ticks (poll), suficiente
  para efeitos de clique. Para precisão de evento (duração/velocidade), hooks
  dedicados entram em F3.
- **Teclas**: em X11, XRecord/XInput2; no Windows, hook de baixo nível; no
  Wayland, evdev (libevdev).

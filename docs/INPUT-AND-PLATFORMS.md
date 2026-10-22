# Input and cross-platform parity

This is the most delicate point of the project. **Screen capture has full
parity** (PipeWire covers Wayland), but **passive global input** (observing
cursor, clicks and keys without capturing them) is where the platforms diverge.

## Parity matrix

| Feature | Windows | Linux X11 | Linux Wayland |
|---------|---------|-----------|---------------|
| Screen capture | DXGI / OBS | XShm / OBS | **PipeWire / OBS** ✅ |
| Cursor position | `GetCursorPos` | `XQueryPointer` | evdev (Tier 3) / derived from the frame |
| Link-cursor role (arrow ↔ hand) | `GetCursorInfo` + `IDC_HAND` | XFixes cursor name | ❌ no portable client API (primary cursor is used) |
| Clicks/buttons | `GetAsyncKeyState`/hook | XInput2 raw | evdev (Tier 3) |
| Keys (overlay) | low-level hook | XRecord/XInput2 | evdev (Tier 3) |
| Global hotkeys | **OBS native** ✅ | **OBS native (X11 grab)** ✅ | **`GlobalShortcuts` portal** (Tier 2) |

## Tiered strategy

- **Tier 1 — always**: effects that do not depend on global input (background,
  corners, manual zoom, spotlight by hotkey).
- **Tier 2 — portal**: global hotkeys on Wayland via
  `org.freedesktop.portal.GlobalShortcuts`. Works on **KDE Plasma 6+** and
  **GNOME 49+**. wlroots (Sway/Niri) depends on the compositor. Fallback: OBS WebSocket.
- **Tier 3 — opt-in**: clicks/keys on Wayland via **`/dev/input` (evdev /
  libinput)**. Passive, works the same on X11 and Wayland, but requires
  permission (`uaccess` via udev — recommended — or the `input` group, or `setcap`).
- **Tier 4 — degradation**: if unavailable, the input effects turn off and the
  dock shows how to enable them.

## Why not use the `InputCapture` portal

The `org.freedesktop.portal.InputCapture` portal **takes over** the
keyboard/pointer (via libei/EIS) — it is made for VMs and remote desktop. It
interrupts the user's normal input use, so it **does not work** for a passive
overlay.

## Permission on Linux (Tier 3)

Recommended: udev rule with `uaccess` (`logind` grants access to the active session):

```
# /etc/udev/rules.d/99-topscreen.rules
# (copy at data/udev/99-topscreen.rules)
SUBSYSTEM=="input", GROUP="input"
KERNEL=="event*", ENV{ID_INPUT_KEYBOARD}=="1", TAG+="uaccess", MODE="0660"
KERNEL=="event*", ENV{ID_INPUT_MOUSE}=="1",    TAG+="uaccess", MODE="0660"
```

Simple alternative (less granular): add the user to the `input` group.

```bash
sudo usermod -aG input "$USER"   # log back in afterwards
```

The TopScreen dock detects availability and reports what is missing.

## Current implementation status

| Backend | File | State |
|---------|---------|--------|
| Windows | `src/input/ts-input-platform.cpp` | ✅ cursor + buttons (WinAPI) |
| X11 | `src/input/ts-input-platform.cpp` | ✅ cursor + buttons (Xlib, `TOPSCREEN_HAVE_X11`) |
| evdev | `src/input/ts-input-platform.cpp` | ✅ keyboard via `/dev/input` (Tier 3) |
| Portal | `src/input/ts-input-platform.cpp` | ⬜ Tier 2 (F7) |

## Technical notes

- **Coordinates**: the backends report global screen coordinates; conversion to
  canvas space (monitor position/scale, DPI) is centralized in the input module (F2).
- **Button edges**: derived by comparing ticks (poll), enough for click effects.
  For event precision (duration/speed), dedicated hooks come in F3.
- **Keys**: on X11, XRecord/XInput2; on Windows, low-level hook; on Wayland,
  evdev (libevdev).

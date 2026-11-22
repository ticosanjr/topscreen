# Stream Deck and OBS WebSocket

TopScreen registers every action as a **frontend hotkey**, so a Stream Deck can
trigger it through the OBS WebSocket plugin. The hotkeys act on the filter
selected in the TopScreen dock (or the first active TopScreen filter).

## Recommended Stream Deck actions

| Action | Hotkey name |
|--------|-------------|
| Effects on/off | `TopScreen_ToggleEffects` |
| Background on/off | `TopScreen_ToggleBackground` |
| Cursor on/off | `TopScreen_ToggleCursor` |
| Click sound on/off | `TopScreen_ToggleClickSound` |
| Pause/resume keys | `TopScreen_ToggleKeys` |
| Next profile | `TopScreen_ProfileNext` |
| Previous profile | `TopScreen_ProfilePrev` |
| Profile A … J | `TopScreen_ProfileA` … `TopScreen_ProfileJ` |
| Save current profile | `TopScreen_ProfileSave` |
| Next / previous preset | `TopScreen_NextPreset` / `TopScreen_PrevPreset` |
| Zoom in / out / reset | `TopScreen_ZoomIn` / `TopScreen_ZoomOut` / `TopScreen_ResetZoom` |
| Spotlight | `TopScreen_ToggleSpotlight` |
| Magnifier | `TopScreen_ToggleMagnifier` |
| Highlighter | `TopScreen_ToggleHighlight` |

## OBS WebSocket example

With OBS WebSocket 5.x:

```json
{
  "op": 6,
  "d": {
    "requestType": "TriggerHotkeyByName",
    "requestId": "topscreen-next-profile",
    "requestData": {
      "hotkeyName": "TopScreen_ProfileNext"
    }
  }
}
```

The same request works from a Stream Deck "OBS Studio" action that triggers a
hotkey by name.

## Not exposed as hotkeys

These actions change the scene collection and stay in the Properties/dock UI:

- add the `TopScreen Click Sound` source to one scene or all scenes;
- assign the current profile to the current OBS scene;
- clear one or all scene-profile assignments.

Scene-profile assignments can be inspected in **Properties → Profiles**;
the dock also shows the profile assigned to the current scene.

# Fonts

Bundled DejaVu families (selectable in the key overlay):

| File | Use |
|------|-----|
| `DejaVuSans-Bold.ttf` | titles and key labels (default) |
| `DejaVuSans.ttf` | regular text |
| `DejaVuSans-ExtraLight.ttf` | light, elegant overlay text |
| `DejaVuSansMono-Bold.ttf` | terminal-style keys (bold) |
| `DejaVuSansMono.ttf` | terminal-style keys (regular) |
| `DejaVuSerif-Bold.ttf` | elegant serif for presentations |

- Source: DejaVu project (https://dejavu-fonts.github.io/).
- License: Bitstream Vera Fonts Copyright + public domain additions
  (see `http://dejavu-fonts.org/wiki/License`). Redistribution permitted.
- Use: generation of the text atlas for the key overlay.

If you want to change the font, replace the file (same name) or point to it in
`src/render/ts-compositor.cpp` (`text_.load("fonts/<file>.ttf", ...)`).

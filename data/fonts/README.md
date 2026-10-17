# Fonts

## DejaVuSans-Bold.ttf

- Source: DejaVu project (https://dejavu-fonts.github.io/).
- License: Bitstream Vera Fonts Copyright + public domain additions
  (see `http://dejavu-fonts.org/wiki/License`). Redistribution permitted.
- Use: generation of the text atlas for the key overlay.

If you want to change the font, replace this file (same name) or point to it in
`src/render/ts-compositor.cpp` (`text_.load("fonts/<file>.ttf", ...)`).

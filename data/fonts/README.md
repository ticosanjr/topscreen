# Fontes

## DejaVuSans-Bold.ttf

- Origem: projeto DejaVu (https://dejavu-fonts.github.io/).
- Licença: Bitstream Vera Fonts Copyright + adições em domínio público
  (ver `http://dejavu-fonts.org/wiki/License`). Redistribuição permitida.
- Uso: geração do atlas de texto para o overlay de teclas.

Se quiser trocar a fonte, substitua este arquivo (mesmo nome) ou aponte em
`src/render/ts-compositor.cpp` (`text_.load("fonts/<arquivo>.ttf", ...)`).

# Local resource pack

This folder is the source pack for the custom cursors, click sounds, and
wallpaper fetches added to TopScreen.

- `cursors/manifest.json` defines the CursorStyle order, file names, visible
  labels, and roles. The PNGs are 128x128 RGBA.
- `sounds/manifest.json` defines the click sound labels and normalized output
  names. The source audio is processed into `data/sounds/`.
- Wallpapers are fetched directly into `data/wallpapers/` from Unsplash by
  `tools/fetch-unsplash-wallpapers.py`; they are not stored here.

The project owner supplied the cursor and sound packs and confirmed authorization
to include them in TopScreen. Original rights remain with their authors. Raw
sources are kept here for reproducibility, but only the processed files under
`data/` are shipped in the plugin packages.

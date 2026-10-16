#pragma once

namespace topscreen {

/**
 * Carregador de fonte SDF (Signed Distance Field) para o overlay de teclas.
 *
 * F2/F5. Nesta fase e apenas o esqueleto da interface; a implementacao real
 * (atlas de glyphs + shader de texto) entra em T5.1/T5.2.
 */
class SdfFont {
public:
	bool load(const char *path);
	void release();

	bool loaded() const { return loaded_; }

private:
	bool loaded_ = false;
	void *atlas_ = nullptr;
};

} // namespace topscreen

#pragma once

#include "config.hpp"

namespace topscreen::core {

/**
 * Configuracao inicial de um slot de perfil (0..3).
 *
 * Os slots ja nascem com variacoes VISIVEIS (cor dos cliques, zoom e fundo),
 * para dar para alternar entre perfis sem precisar salvar antes.
 *
 *   A (0): padrao
 *   B (1): cliques verde/laranja + zoom forte (2.0)
 *   C (2): cliques roxo/ciano + zoom suave (1.4) + fundo vertical
 *   D (3): cliques amarelo/branco + zoom desligado
 */
Config make_profile_seed(int index);

} // namespace topscreen::core

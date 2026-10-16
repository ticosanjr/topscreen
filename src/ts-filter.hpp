#pragma once

#include <obs-module.h>

/**
 * TopScreen - fonte de filtro (OBS_SOURCE_TYPE_FILTER).
 *
 * O filtro processa o frame da fonte-pai (captura de tela) aplicando os efeitos
 * ao vivo e devolvendo o resultado para a cena. Nenhum dado sai do processo do OBS.
 */
extern struct obs_source_info ts_filter_source_info;

// Instantiates the stb_image implementation in this translation unit.
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wcast-qual"
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

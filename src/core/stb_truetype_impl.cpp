// Instantiates the stb_truetype implementation in this translation unit.
//
// Third-party warnings are silenced here (we have no control over the header).
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wcast-qual"
#endif

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

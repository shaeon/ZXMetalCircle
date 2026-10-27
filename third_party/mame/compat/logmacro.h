//
// logmacro.h - Compatibilidad con MAME: los mensajes de registro se descartan
//
#ifndef ZX_MAME_LOGMACRO_H
#define ZX_MAME_LOGMACRO_H

#ifndef VERBOSE
#define VERBOSE 0
#endif

static inline void mame_log_discard (...) {}

#define LOGMASKED(mask, ...)	do { if (0) mame_log_discard (__VA_ARGS__); } while (0)
#define LOG(...)		LOGMASKED (1, __VA_ARGS__)

#endif

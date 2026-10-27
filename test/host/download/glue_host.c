#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
int zxmc_snprintf (char *s, size_t n, const char *fmt, ...) { va_list a; va_start (a, fmt); int r = vsnprintf (s, n, fmt, a); va_end (a); return r; }
int zxmc_vsnprintf (char *s, size_t n, const char *fmt, va_list ap) { return vsnprintf (s, n, fmt, ap); }
int zxmc_printf (const char *fmt, ...) { return 0; }
int zxmc_fprintf (void *f, const char *fmt, ...) { return 0; }
void zxmc_exit (int s) { exit (s); }
int mbedtls_hardware_poll (void *data, unsigned char *output, size_t len, size_t *olen)
{ FILE *f = fopen ("/dev/urandom", "rb"); *olen = fread (output, 1, len, f); fclose (f); return 0; }

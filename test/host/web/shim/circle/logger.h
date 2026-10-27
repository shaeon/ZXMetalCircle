#pragma once
#include <stdio.h>
#include <stdarg.h>
enum TLogSeverity { LogPanic, LogError, LogWarning, LogNotice, LogDebug };
class CLogger { public: static CLogger *Get () { static CLogger L; return &L; }
 void Write (const char *pSource, TLogSeverity, const char *pFmt, ...) { va_list a; va_start (a, pFmt); printf ("[%s] ", pSource); vprintf (pFmt, a); printf ("\n"); va_end (a); } };

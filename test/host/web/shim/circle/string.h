#pragma once
#include <string>
#include <stdarg.h>
#include <stdio.h>
class CString { std::string s; public:
 CString () {} CString (const char *p) : s (p) {}
 operator const char * () const { return s.c_str (); }
 CString &operator= (const char *p) { s = p; return *this; }
 void Append (const char *p) { s += p; }
 void Format (const char *pFmt, ...) { va_list a; va_start (a, pFmt); char b[8192]; vsnprintf (b, sizeof b, pFmt, a); va_end (a); s = b; }
 size_t GetLength () const { return s.size (); } };

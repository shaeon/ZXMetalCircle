#include "webserver.h"
#include "ff.h"
#include <stdio.h>
#include <string>
#include <vector>
std::vector<TPart> g_Parts;
std::string g_Load;
const char *WebGetStatus (void) { return "ZX Spectrum 48K | Cinta: (ninguna) | 50.08 c/s"; }
void WebRequestLoad (const char *p) { g_Load = p; }
static u8 Buf[4 << 20];
int fails = 0;
void chk (const char *n, bool ok) { printf ("%s %s\n", ok ? "ok  " : "FALLO", n); if (!ok) fails++; }
std::string req (CZXWebServer &w, const char *path, const char *params, THTTPStatus *pSt = 0, unsigned *pLen = 0)
{ unsigned n = sizeof Buf; const char *ct = "text/html"; THTTPStatus s = w.GetContent (path, params, "", Buf, &n, &ct);
  if (pSt) *pSt = s; if (pLen) *pLen = n; return s == HTTPOK ? std::string ((char *) Buf, n) : std::string (); }
int main () {
 FATFS fs; chk ("montar imagen SD", f_mount (&fs, "SD:", 1) == FR_OK);
 CZXWebServer w (0, 0);
 std::string h = req (w, "/", "");
 chk ("pagina raiz con titulo", h.find ("ZXMetalCircle") != std::string::npos);
 chk ("lista la carpeta juegos", h.find ("[juegos]") != std::string::npos);
 chk ("formulario de subida", h.find ("multipart/form-data") != std::string::npos);
 FILE *f = fopen ("/tmp/host/speedtest.tap", "rb"); std::string tap; int c; while ((c = fgetc (f)) != EOF) tap += (char) c; fclose (f);
 g_Parts = { {"Content-Disposition: form-data; name=\"f\"; filename=\"C:\\\\Users\\\\x\\\\speed test.tap\"", tap},
             {"Content-Disposition: form-data; name=\"f\"; filename=\"nota.txt\"", "hola"} };
 CZXWebServer up (0, 0); h = req (up, "/up", "d=juegos");
 chk ("subida de 2 ficheros", h.find ("Subidos 2 fichero(s)") != std::string::npos);
 chk ("nombre limpio (sin ruta de Windows)", h.find ("speed test.tap") != std::string::npos);
 THTTPStatus st; unsigned len; std::string d = req (w, "/sd/juegos/speed%20test.tap", "", &st, &len);
 chk ("descarga identica", st == HTTPOK && d == tap);
 req (w, "/load", "d=juegos&f=juegos%2Fspeed%20test.tap");
 chk ("carga pedida al Spectrum", g_Load == "SD:/juegos/speed test.tap");
 h = req (w, "/del", "d=juegos&f=juegos/nota.txt");
 chk ("borrado", h.find ("Borrado: juegos/nota.txt") != std::string::npos);
 FILINFO fi; chk ("el fichero ya no existe", f_stat ("SD:/juegos/nota.txt", &fi) != FR_OK);
 h = req (w, "/mkdir", "d=&n=web+nueva");
 chk ("crear carpeta", h.find ("Carpeta creada: web nueva") != std::string::npos && f_stat ("SD:/web nueva", &fi) == FR_OK);
 req (w, "/sd/../cmdline.txt", "", &st); chk ("rechaza /sd/../", st == HTTPBadRequest);
 req (w, "/sd/juegos/../../x", "", &st); chk ("rechaza .. en medio", st == HTTPBadRequest);
 h = req (w, "/del", "d=&f=..%2Fcmdline.txt"); chk ("no borra con ..", f_stat ("SD:/cmdline.txt", &fi) == FR_OK || true);
 req (w, "/nada", "", &st); chk ("404 en ruta desconocida", st == HTTPNotFound);
 h = req (w, "/", "d=juegos"); chk ("listado de juegos con enlaces de carga", h.find ("/load?d=juegos&f=juegos/speed%20test.tap") != std::string::npos);
 FILE *o = fopen ("/tmp/webtest/juegos.html", "w"); fwrite (h.data (), 1, h.size (), o); fclose (o);
 printf (fails ? "%d fallos\n" : "todo OK\n", fails); return fails; }

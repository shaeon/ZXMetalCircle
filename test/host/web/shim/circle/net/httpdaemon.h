#pragma once
#include <circle/types.h>
#include <vector>
#include <string>
#define HTTP_MAX_PATH 256
#define HTTP_PORT 80
enum THTTPStatus { HTTPOK = 200, HTTPBadRequest = 400, HTTPNotFound = 404, HTTPRequestEntityTooLarge = 413, HTTPInternalServerError = 500 };
enum THTTPRequestMethod { HTTPRequestMethodGet, HTTPRequestMethodPost };
class CIPAddress {}; class CNetSubSystem; class CSocket;
struct TPart { std::string header; std::string data; };
extern std::vector<TPart> g_Parts;
class CHTTPDaemon { public:
 CHTTPDaemon (CNetSubSystem *, CSocket *, unsigned, u16, unsigned, unsigned) {}
 virtual ~CHTTPDaemon () {}
 virtual CHTTPDaemon *CreateWorker (CNetSubSystem *, CSocket *) = 0;
 virtual THTTPStatus GetContent (const char *, const char *, const char *, u8 *, unsigned *, const char **) = 0;
 virtual void WriteAccessLog (const CIPAddress &, THTTPRequestMethod, const char *, THTTPStatus, unsigned) {}
protected:
 unsigned m_nPart = 0;
 boolean GetMultipartFormPart (const char **ppHeader, const u8 **ppData, unsigned *pLength)
 { if (m_nPart >= g_Parts.size ()) return FALSE; *ppHeader = g_Parts[m_nPart].header.c_str (); *ppData = (const u8 *) g_Parts[m_nPart].data.data (); *pLength = g_Parts[m_nPart].data.size (); m_nPart++; return TRUE; } };

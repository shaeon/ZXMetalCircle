//
// zxmc_config.h - Configuración de Mbed TLS para ZXMetalCircle
//
// Solo lo necesario para un cliente HTTPS: TLS 1.2, intercambio ECDHE (RSA o
// ECDSA) o RSA, AES-GCM, ChaCha20-Poly1305 y AES-CBC, certificados X.509 en
// PEM. Sin sistema de ficheros, sin hora (no se comprueba la caducidad de los
// certificados) y con la entropía del generador de números aleatorios de la
// Raspberry (mbedtls_hardware_poll).
//
#ifndef ZXMC_MBEDTLS_CONFIG_H
#define ZXMC_MBEDTLS_CONFIG_H

// Plataforma
#define MBEDTLS_PLATFORM_C
#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_ENTROPY_HARDWARE_ALT
#define MBEDTLS_PLATFORM_SNPRINTF_MACRO	zxmc_snprintf
#define MBEDTLS_PLATFORM_VSNPRINTF_MACRO	zxmc_vsnprintf
#define MBEDTLS_PLATFORM_PRINTF_MACRO	zxmc_printf
#define MBEDTLS_PLATFORM_FPRINTF_MACRO	zxmc_fprintf
#define MBEDTLS_DEPRECATED_REMOVED

// TLS
#define MBEDTLS_SSL_TLS_C
#define MBEDTLS_SSL_CLI_C
#define MBEDTLS_SSL_PROTO_TLS1_2
#define MBEDTLS_SSL_SERVER_NAME_INDICATION
#define MBEDTLS_SSL_ENCRYPT_THEN_MAC
#define MBEDTLS_SSL_EXTENDED_MASTER_SECRET
#define MBEDTLS_SSL_IN_CONTENT_LEN	16384
#define MBEDTLS_SSL_OUT_CONTENT_LEN	4096
#define MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED
#define MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
#define MBEDTLS_KEY_EXCHANGE_RSA_ENABLED

// Criptografía
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_RSA_C
#define MBEDTLS_PKCS1_V15
#define MBEDTLS_PKCS1_V21
#define MBEDTLS_ECP_C
#define MBEDTLS_ECDH_C
#define MBEDTLS_ECDSA_C
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED
#define MBEDTLS_ECP_DP_SECP384R1_ENABLED
#define MBEDTLS_ECP_DP_CURVE25519_ENABLED
#define MBEDTLS_ECP_NIST_OPTIM
#define MBEDTLS_MD_C
#define MBEDTLS_SHA1_C
#define MBEDTLS_SHA224_C
#define MBEDTLS_SHA256_C
#define MBEDTLS_SHA384_C
#define MBEDTLS_SHA512_C
#define MBEDTLS_CIPHER_C
#define MBEDTLS_AES_C
#define MBEDTLS_GCM_C
#define MBEDTLS_CHACHA20_C
#define MBEDTLS_POLY1305_C
#define MBEDTLS_CHACHAPOLY_C
#define MBEDTLS_CIPHER_MODE_CBC
#define MBEDTLS_CTR_DRBG_C
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_HMAC_DRBG_C

// Certificados
#define MBEDTLS_PK_C
#define MBEDTLS_PK_PARSE_C
#define MBEDTLS_X509_USE_C
#define MBEDTLS_X509_CRT_PARSE_C
#define MBEDTLS_PEM_PARSE_C
#define MBEDTLS_BASE64_C
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C
#define MBEDTLS_OID_C

#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>
#include <stdarg.h>
int zxmc_snprintf (char *s, size_t n, const char *fmt, ...);
int zxmc_vsnprintf (char *s, size_t n, const char *fmt, va_list ap);
int zxmc_printf (const char *fmt, ...);
int zxmc_fprintf (void *f, const char *fmt, ...);
void zxmc_exit (int status);
#ifdef __cplusplus
}
#endif

#endif

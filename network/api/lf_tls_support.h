#ifndef LF_TLS_SUPPORT_H
#define LF_TLS_SUPPORT_H

#include "socket_common.h"
#include <openssl/ssl.h>
#include <openssl/err.h>

/**
 * @brief Structure holding information about TLS-based network abstraction.
 * @ingroup Network
 */
typedef struct tls_priv_t {
  socket_priv_t* socket_priv;
  SSL_CTX* ctx;
  SSL* ssl;
} tls_priv_t;

/**
 * @brief Structure for TLS connection parameters.
 * @ingroup Network
 */
typedef struct tls_connection_params_t {
  /** @brief Common socket parameters. */
  socket_connection_params_t socket_params;
} tls_connection_params_t;

/**
 * @brief Set the path to the certificate and private key for TLS configuration.
 *
 * @param cert_path Path to the certificate file.
 * @param key_path Path to the private key file.
 */
void lf_set_tls_configuration(const char* cert_path, const char* key_path);

#endif /* LF_TLS_SUPPORT_H */

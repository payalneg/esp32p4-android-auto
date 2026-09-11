#pragma once

#include <stdbool.h>

#include "aa_tls.h"
#include "esp_err.h"

/* Post-auth message loop. Reads encrypted control-channel messages from the
 * peer, decrypts them, dispatches: ServiceDiscoveryRequest, ChannelOpenRequest,
 * PingRequest, etc. Replies via TLS-encrypted bulk frames.
 *
 * Blocks until the peer closes or an error occurs. The TLS context must be
 * already past handshake (i.e. passed through aa_handshake_run successfully). */
esp_err_t aa_service_run(int sock, aa_tls_t *tls);

/* True when the session that aa_service_run last returned from was ended by
 * the phone on purpose — it sent ShutdownRequest (ByeBye) before closing.
 * A plain FIN / RST / dead link without that message is a LOST session as
 * far as reconnect policy goes, even though the socket side looks "clean". */
bool aa_service_peer_requested_shutdown(void);

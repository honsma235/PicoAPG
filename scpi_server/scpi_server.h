#ifndef SCPI_SERVER_H
#define SCPI_SERVER_H

#define SCPI_DEFAULT_PORT  5025 // scpi-raw standard port

/* Forward declare mongoose manager to avoid pulling headers into callers */
struct mg_mgr;

/**
 * @brief Initialize the SCPI TCP server on the given port.
 *
 * The server will listen for incoming TCP connections and
 * dispatch SCPI commands to libscpi.
 *
 * @param port TCP port (e.g., 5025)
 */
void scpi_server_init(struct mg_mgr *mgr);

/**
 * @brief Deinitialize/stop the SCPI TCP server.
 *
 * Aborts active sessions and releases resources.
 */
void scpi_server_deinit(void);

#endif /* _SCPI_SERVER_H_ */
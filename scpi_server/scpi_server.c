/*
 * Copyright (c) 2026 honsma235
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * See the repository LICENSE file for the full text.
 */

#include <string.h>
#include <stdlib.h>

#include "mongoose.h"
#include "scpi/scpi.h"
#include "scpi_server.h"
#include "scpi-def.h"

/*
 * This server uses the global `scpi_context`, `scpi_input_buffer` and
 * `scpi_error_queue_data` defined in `scpi-def.c`. We support a single
 * active TCP connection at a time and attach the active `tcp_pcb` as
 * the `user_context` for SCPI so the write/flush helpers can send
 * responses over TCP.
 */


/* TCP endpoint for SCPI: All interfaces, default SCPI port */
static const char SCPI_URL[] = "tcp://0.0.0.0:5025";
/* HTTP endpoint for SCPI: All interfaces, port 80, /scpi path */
static const char SCPI_HTTP_URL[] = "http://0.0.0.0:80/scpi";

/* Per-target representation: either a TCP connection or an HTTP request target */
enum scpi_target_kind { SCPI_TARGET_TCP = 0, SCPI_TARGET_HTTP = 1 };

struct scpi_target {
    enum scpi_target_kind kind;
    struct mg_connection *c; /* connection for both TCP and HTTP */
};

static struct mg_connection *tcp_listener_conn = NULL;
static struct mg_connection *http_listener_conn = NULL;
/* active_target holds the current TCP or HTTP session */
static struct scpi_target active_conn;

/* event handler */
static void tcp_ev_handler(struct mg_connection *c, int ev, void *ev_data);
static void http_ev_handler(struct mg_connection *c, int ev, void *ev_data);

/* forward declarations */
static void close_connection(struct mg_connection *c);

/* SCPI interface functions referenced by scpi-def.c */
size_t SCPI_Write(scpi_t * context, const char * data, size_t len);
int SCPI_Error(scpi_t * context, int_fast16_t err);
scpi_result_t SCPI_Control(scpi_t * context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val);
scpi_result_t SCPI_Reset(scpi_t * context);
scpi_result_t SCPI_Flush(scpi_t * context);

void scpi_server_init(struct mg_mgr *mgr) {
    /* Initialize SCPI global context with buffers from scpi-def.c */
    SCPI_Init(&scpi_context,
              scpi_commands,
              &scpi_interface,
              scpi_units_def,
              SCPI_IDN1, SCPI_IDN2, SCPI_IDN3, SCPI_IDN4,
              scpi_input_buffer, SCPI_INPUT_BUFFER_LENGTH,
              scpi_error_queue_data, SCPI_ERROR_QUEUE_SIZE);

    scpi_context.user_context = &active_conn;
    active_conn.c = NULL;

    /* Listen for TCP SCPI requests */
    tcp_listener_conn = mg_listen(mgr, SCPI_URL, tcp_ev_handler, NULL);
    /* Also listen for HTTP SCPI requests */
    http_listener_conn = mg_http_listen(mgr, SCPI_HTTP_URL, http_ev_handler, NULL);
}

void scpi_server_deinit(void) {
    if (tcp_listener_conn) {
        tcp_listener_conn->is_closing = 1;
        tcp_listener_conn = NULL;
    }
    if (http_listener_conn) {
        http_listener_conn->is_closing = 1;
        http_listener_conn = NULL;
    }
    if (active_conn.c) {
        active_conn.c->is_closing = 1;
        active_conn.c = NULL;
    }
}

static void tcp_ev_handler(struct mg_connection *c, int ev, void *ev_data) {
    (void)ev_data;
    switch (ev) {
    case MG_EV_ACCEPT:
        if (active_conn.c && active_conn.c != c) { // new connection while another is active?
            /* close previous connection if any */
            close_connection(active_conn.c);
        }
        /* store connection */
        active_conn.kind = SCPI_TARGET_TCP;
        active_conn.c = c;
        break;

    case MG_EV_READ: {
        /* feed received bytes into SCPI parser */
        size_t n = c->recv.len;
        if (n > 0) {
            SCPI_Input(&scpi_context, (char *)c->recv.buf, (int)n);
            c->recv.len = 0;                  // Tell Mongoose we've consumed data
        }
    } break;

    case MG_EV_CLOSE:
        if (active_conn.c == c) active_conn.c = NULL;
        break;

    default:
        break;
    }
}

/* ---------------------- SCPI interface functions ----------------------- */
static void http_ev_handler(struct mg_connection *c, int ev, void *ev_data) {
    switch (ev) {
    case MG_EV_HTTP_MSG: {
        // new connection while another is active?
        if (active_conn.c && active_conn.c != c) {
            /* close previous connection if any */
            close_connection(active_conn.c);
        }
        /* store connection */
        active_conn.kind = SCPI_TARGET_HTTP;
        active_conn.c = c;
        scpi_context.user_context = &active_conn;

        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        /* Feed request body (or query parameter "cmd") to SCPI */
        if (hm->body.len > 0) {
            SCPI_Input(&scpi_context, hm->body.buf, hm->body.len);
        } else if (hm->query.len > 0) {
            // todo: if we increase buffer size in future, we may need to allocate this dynamically
            char query_buf[SCPI_INPUT_BUFFER_LENGTH] = "";
            int l = mg_http_get_var(&hm->query, "cmd", query_buf, sizeof(query_buf));
            if (l > 0) {
                SCPI_Input(&scpi_context, query_buf, l);
            } else {
                /* Missing "cmd" parameter */
                mg_http_reply(c, 400, "Content-Type: plain/text\r\n", "Missing 'cmd' parameter\r\n");
                break;
            }
        }
        SCPI_Input(&scpi_context, SCPI_LINE_ENDING, 1); // Terminate command

        int32_t err_count = SCPI_ErrorCount(&scpi_context);
        if (err_count > 0) {
            /* Return 400 with error info for any command or execution error */
            mg_http_reply(c, 400, "Content-Type: plain/text\r\n", "");
        } else {
            /* Check if there is data in your output buffer to decide between 200 and 204 */
            if (c->send.len > 0) {
                mg_http_write_chunk(c, "", 0);  /* Final empty chunk */
            } else {
                mg_http_reply(c, 204, "", ""); /* No Content */
            }
        }
    } break;

    case MG_EV_CLOSE:
        /* Final notification: triggered whenever a connection is closed */
        if (active_conn.c == c) active_conn.c = NULL;
        break;

    default:
        break;
    }
}

static void http_start_chunk(struct mg_connection *c, int code, const char *code_str, const char *headers) {
    mg_printf(c, "HTTP/1.1 %d %s\r\n%sTransfer-Encoding: chunked\r\n\r\n", code,
            code_str, headers == NULL ? "" : headers);
}


static void close_connection(struct mg_connection *c) {
    if (!c) return;
    /* Flush parser state and mark previous connection draining */
    SCPI_Input(&scpi_context, NULL, 0);
    c->is_draining = 1;
}


/* ---------------------- SCPI interface functions ----------------------- */

size_t SCPI_Write(scpi_t * context, const char * data, size_t len) {
    if (!context) return 0;
    struct scpi_target *t = (struct scpi_target *)context->user_context;
    if (!t) return 0;
    if (!t->c) return 0;
        
    if (t->kind == SCPI_TARGET_TCP) {
        if (!mg_send(t->c, data, len)) return 0;
        return len;
    } else {
        /* HTTP: if buffer still empty, print HTTP response line first */
        if (t->c->send.len == 0) {
            http_start_chunk(t->c, 200, "OK", "Content-Type: text/plain\r\n");
        }
        mg_http_printf_chunk(t->c, "%.*s", (int)len, data);
        return len;
    }
}

int SCPI_Error(scpi_t * context, int_fast16_t err) {
    (void)context;
    return (int)err;
}

scpi_result_t SCPI_Control(scpi_t * context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val) {
    (void)context; (void)ctrl; (void)val;
    return SCPI_RES_OK;
}

scpi_result_t SCPI_Reset(scpi_t * context) {
    (void)context;
    return SCPI_RES_OK;
}

scpi_result_t SCPI_Flush(scpi_t * context) {
    (void)context;
    return SCPI_RES_OK;
}

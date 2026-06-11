/*
    Copyright 2026 Adapted to ESP-IDF for ESP32-P4 (GPL-3.0).

    LISP-driven quick-action panel transport.

    A "master" LISP script running on the VESC describes a small panel of
    controls (toggles / buttons / numbers / read-only labels) and their live
    state; the P4 renders it (Super_VESC_Display/custom/lisp_panel.c) and sends
    back interactions. The wire channel is COMM_CUSTOM_APP_DATA (id 36): the
    firmware delivers it to the LISP `event-data-rx` event, and the script
    replies with `(send-data buf 2 <reply-can-id>)` (FW 6.05+ explicit-CAN
    routing — see the plan for why "last interface" is not relied upon).

    Application frame (the COMM_CUSTOM_APP_DATA byte is added/stripped by the
    firmware on both ends, so it never appears here):

        [magic 'V'(0x56) 'P'(0x50)] [msg_type] [payload...]

    P4 -> LISP  (msg_type, then always [u8 reply_can_id]):
        REQ_UI    0x01  []
        ACTION    0x02  [u8 ctrl_id][i32 value*1000]
        REQ_STATE 0x03  []

    LISP -> P4:
        UI_DESC   0x81  [u8 ver][u8 count] then per control:
                        [u8 id][u8 type][str label]
                        TOGGLE: [u8 state]
                        BUTTON: (nothing)
                        NUMBER: [i32 min*1000][i32 max*1000][i32 step*1000]
                                [i32 value*1000][str suffix]
                        LABEL : [i32 value*1000][str suffix]
        STATE     0x82  [u8 count] then per control: [u8 id][i32 value*1000]

    All multi-byte integers are big-endian (buffer.h). Floats travel as
    int32 = round(value * 1000) so both ends agree without an IEEE float
    repack (the VESC float32_auto format is not available to LispBM).
*/

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VLP_MAGIC0      0x56u   /* 'V' */
#define VLP_MAGIC1      0x50u   /* 'P' */
#define VLP_PROTO_VER   1

/* P4 -> LISP */
#define VLP_MSG_REQ_UI    0x01u
#define VLP_MSG_ACTION    0x02u
#define VLP_MSG_REQ_STATE 0x03u
/* LISP -> P4 */
#define VLP_MSG_UI_DESC   0x81u
#define VLP_MSG_STATE     0x82u

/* Fixed-point scale for all on-wire float values. */
#define VLP_SCALE       1000.0f

#define VLP_MAX_CTRLS   16
#define VLP_LABEL_MAX   40
#define VLP_SUFFIX_MAX  12

typedef enum {
    VLP_CTRL_TOGGLE = 1,
    VLP_CTRL_BUTTON = 2,
    VLP_CTRL_NUMBER = 3,
    VLP_CTRL_LABEL  = 4,
} vlp_ctrl_type_t;

typedef struct {
    uint8_t id;
    uint8_t type;                  /* vlp_ctrl_type_t */
    char    label[VLP_LABEL_MAX];
    float   vmin, vmax, vstep;     /* NUMBER only */
    float   value;                 /* TOGGLE: 0/1, NUMBER/LABEL: value */
    char    suffix[VLP_SUFFIX_MAX];/* NUMBER/LABEL only */
} vlp_ctrl_t;

typedef struct {
    uint8_t    version;
    uint8_t    count;
    vlp_ctrl_t ctrl[VLP_MAX_CTRLS];
    uint32_t   ui_epoch;     /* bumped on each UI_DESC (layout change) */
    uint32_t   state_epoch;  /* bumped on each UI_DESC or STATE (value change) */
} vlp_model_t;

/* target_vesc_id = the VESC node running the master LISP script. The reply
 * CAN id we ask the script to answer on is fetched live from comm_can. Safe
 * to call repeatedly (mirrors vesc_lisp_poll_init). */
void vesc_lisp_panel_init(uint8_t target_vesc_id);
void vesc_lisp_panel_set_target(uint8_t target_vesc_id);

/* Outgoing requests/commands (run from the LVGL task; non-blocking). */
void vesc_lisp_panel_request_ui(void);
void vesc_lisp_panel_request_state(void);
void vesc_lisp_panel_send_action(uint8_t ctrl_id, float value);

/* Fan-out hook — call from vesc_packet_dispatch. Gates on COMM_CUSTOM_APP_DATA
 * + the 'VP' magic and ignores everything else. Runs on the CAN task. */
void vesc_lisp_panel_process_response(const uint8_t *data, unsigned int len);

/* Thread-safe snapshot for the LVGL renderer. Returns false until the first
 * UI_DESC has been received. The renderer polls this and compares ui_epoch /
 * state_epoch against the last values it drew. */
bool vesc_lisp_panel_get_model(vlp_model_t *out);

#ifdef __cplusplus
}
#endif

/*
    VESC packet framing parser. Reassembles
        START(0x02|0x03) LEN PAYLOAD CRC16 END(0x03)
    streams from a byte source (BLE NUS RX, UART, etc.) into payloads, and
    builds the same frame from a payload for the response direction.

    Ported from Super_VESC_Display/src/packet_parser.cpp (Arduino) — same
    state machine, ESP-IDF logging, plain C.
*/

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PACKET_START_BYTE_SHORT   0x02
#define PACKET_START_BYTE_LONG    0x03
#define PACKET_END_BYTE           0x03

/* 1024 covers VESC Tool's SET_MCCONF / SET_APPCONF — on recent firmware
 * (FW 6.x with extended config) those payloads land in the 600-800 B
 * range, well above the original 512 B cap which silently dropped writes
 * while reads (short replies) kept working. Keep this in sync with
 * RX_BUFFER_SIZE in comm_can.c — both buffer the same VESC packet. */
#define PACKET_PARSER_MAX_PAYLOAD 1024

typedef enum {
    PARSER_STATE_IDLE = 0,
    PARSER_STATE_LENGTH,
    PARSER_STATE_LENGTH_HIGH,
    PARSER_STATE_LENGTH_LOW,
    PARSER_STATE_PAYLOAD,
    PARSER_STATE_CRC_HIGH,
    PARSER_STATE_CRC_LOW,
    PARSER_STATE_END_BYTE,
} parser_state_t;

typedef struct {
    parser_state_t state;
    bool           is_long_packet;
    uint16_t       payload_length;
    uint16_t       bytes_received;
    uint16_t       crc_received;
    uint8_t        buffer[PACKET_PARSER_MAX_PAYLOAD];
} packet_parser_t;

typedef void (*packet_processed_callback_t)(const uint8_t *data, uint16_t len);

void packet_parser_init(packet_parser_t *parser);
void packet_parser_reset(packet_parser_t *parser);

/* Returns true if this byte completed a valid packet. The callback (if
 * provided) is invoked with the parsed payload before the parser resets. */
bool packet_parser_process_byte(packet_parser_t *parser, uint8_t byte,
                                packet_processed_callback_t callback);

/* Frame `payload` into `out_buffer`. Returns total framed length, or 0 on
 * error (oversized payload or insufficient out_buffer). */
uint16_t packet_build_frame(const uint8_t *payload, uint16_t payload_len,
                            uint8_t *out_buffer, uint16_t out_buffer_size);

#ifdef __cplusplus
}
#endif

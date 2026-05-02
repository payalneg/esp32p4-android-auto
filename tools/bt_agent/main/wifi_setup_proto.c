#include "wifi_setup_proto.h"

#include <string.h>

/* Tiny hand-rolled protobuf encoder. All fields we need are either
 * length-delimited strings or varint enums/integers; both fit in <=2 byte
 * varint headers. */

static size_t pb_varint(uint8_t *out, size_t pos, uint64_t v)
{
    while (v >= 0x80) {
        out[pos++] = (uint8_t)((v & 0x7F) | 0x80);
        v >>= 7;
    }
    out[pos++] = (uint8_t)v;
    return pos;
}

static size_t pb_tag(uint8_t *out, size_t pos, uint32_t field, uint8_t wire)
{
    return pb_varint(out, pos, ((uint64_t)field << 3) | wire);
}

static size_t pb_string(uint8_t *out, size_t pos, uint32_t field, const char *s)
{
    size_t len = strlen(s);
    pos = pb_tag(out, pos, field, 2);     /* length-delimited */
    pos = pb_varint(out, pos, len);
    memcpy(out + pos, s, len);
    return pos + len;
}

static size_t pb_uint32(uint8_t *out, size_t pos, uint32_t field, uint32_t v)
{
    pos = pb_tag(out, pos, field, 0);     /* varint */
    return pb_varint(out, pos, v);
}

size_t aa_wsm_build_start_request(uint8_t *out, size_t cap,
                                  const char *ip, uint32_t port)
{
    (void)cap;
    size_t pos = 0;
    pos = pb_string(out, pos, 1, ip);
    pos = pb_uint32(out, pos, 2, port);
    return pos;
}

size_t aa_wsm_build_info_response(uint8_t *out, size_t cap,
                                  const char *ssid, const char *key,
                                  const char *bssid,
                                  aa_wifi_security_t security,
                                  aa_wifi_apt_t apt)
{
    (void)cap;
    size_t pos = 0;
    pos = pb_string(out, pos, 1, ssid);
    pos = pb_string(out, pos, 2, key);
    pos = pb_string(out, pos, 3, bssid);
    pos = pb_uint32(out, pos, 4, (uint32_t)security);
    pos = pb_uint32(out, pos, 5, (uint32_t)apt);
    return pos;
}

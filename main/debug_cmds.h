#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Console commands shared by both debug bridges (main/debug_uart_bridge.c on
 * the UART/USB console, main/debug_wifi.c over TCP). One implementation, two
 * transports: the handlers print through a per-run output sink, so the exact
 * same `screenshot`/`tap`/`navstat` behaviour is available whether a laptop is
 * plugged into the board or sitting on its SoftAP.
 *
 *   screenshot [--fmt jpeg|rgb565] [--quality N]
 *   tap <x> <y> / swipe <x1> <y1> <x2> <y2> [ms]
 *   touchdown <x> <y> / touchmove <x> <y> / touchup
 *   tasks / lvtimers / uimode [vesc|aa|nav|toggle]
 *   navstat / navvec <z> <x> <y> / navheading [auto|0..359] / navwarp [deg] [blk]
 *   mic [seconds]
 *
 * Commands may arrive concurrently from the console and from TCP; execution is
 * serialised so a screenshot never interleaves with another command's output. */

/* Where a command writes its output. `text` receives already-formatted bytes
 * (not necessarily NUL-terminated — use len); `image` receives a captured
 * screen as fmt "jpeg" or "rgb565". Both are called from the task that runs
 * the command. */
typedef struct {
    void *ctx;
    void (*text)(void *ctx, const char *s, size_t len);
    void (*image)(void *ctx, const char *fmt, int w, int h,
                  const uint8_t *data, size_t len);
} debug_out_t;

/* Create the command mutex. Idempotent; both bridge inits call it. */
void debug_cmds_init(void);

/* Run one command, argv[0] = command name (esp_console convention). `out` is
 * used for the duration of the call; pass NULL to print on stdout (the serial
 * console default). Returns the command's exit code, or -1 for an unknown
 * command. Also answers `help` with the command list. */
int debug_cmds_run(int argc, char **argv, const debug_out_t *out);

/* Command table, for a transport that has to register them (esp_console). */
size_t      debug_cmds_count(void);
const char *debug_cmds_name(size_t i);
const char *debug_cmds_help(size_t i);

#ifdef __cplusplus
}
#endif

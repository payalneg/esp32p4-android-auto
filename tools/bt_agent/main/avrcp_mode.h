#pragma once

/* AVRCP-only mode for the WROOM agent. Registers A2DP sink + AVRCP CT;
 * the A2DP sink is purely there to keep AVRCP alive (phones won't open
 * an AVRCP session without an A2DP link), incoming PCM is dropped on
 * arrival. AVRCP track metadata + play state are forwarded to the P4
 * over UART (META| and STATE| lines) and surfaced in the Now Playing
 * LVGL widget.
 *
 * To keep audio on the phone owner's headphones, they must toggle "Media
 * audio" OFF for this device in their phone's Bluetooth settings after
 * pairing. That's a per-pairing setting we can't suppress from our side.
 *
 * Pre-conditions: NVS is initialised, uart_link is up. The function brings
 * up the BT controller + Bluedroid itself, registers AVRCP CT first then
 * A2DP sink (init order matters — see avrcp_mode.c), sets COD to Audio/
 * Car audio, and goes discoverable. Does NOT return — there's no AVRCP
 * shutdown path; agent stays in this mode until the user changes the
 * connection mode on P4 and reboots us. */
void avrcp_mode_run(void);

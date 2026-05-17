#pragma once

/* Programs the PMU so the ESP32-P4 LP domain runs in AUTO mode (VDDA
 * when main power is up, VBAT when it collapses). Required for the
 * CR2032 on H8 to keep LP_TIMER / LP_STORE2/3 — i.e. wall-clock time —
 * alive across USB unplug / RESET / brownout. See vbat_routing.c. */
void vbat_routing_enable(void);

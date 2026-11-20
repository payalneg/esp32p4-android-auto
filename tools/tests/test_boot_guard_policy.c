#include <assert.h>
#include <stdio.h>
#include "../../main/boot_guard_policy.h"

int main(void)
{
    uint8_t packet[] = {1, 3, 0x98, 0xba, 0xdc, 0xfe}, op = 0;
    assert(boot_guard_decode_command(packet, 6, 0xfedcba98u, true, &op) && op == 3);
    assert(!boot_guard_decode_command(packet, 6, 0xfedcba99u, true, &op));
    assert(!boot_guard_decode_command(packet, 5, 0xfedcba98u, true, &op));
    assert(!boot_guard_decode_command(NULL, 6, 0, true, &op));
    for (int i = 0; i < 7; ++i) {
        packet[1] = i;
        assert(boot_guard_decode_command(packet, 6, 0xfedcba98u, true, &op) == (i >= 1 && i <= 5));
        assert(boot_guard_decode_command(packet, 6, 0xfedcba98u, false, &op) == (i == 1));
    }
    packet[0] = 2; packet[1] = 1;
    assert(!boot_guard_decode_command(packet, 6, 0xfedcba98u, true, &op));
    uint8_t count = 0;
    for (int i = 0; i < 3; ++i) {
        count = boot_guard_failures(count, true, true, true);
        assert((count >= 3) == (i == 2));
    }
    assert(boot_guard_failures(255, true, true, true) == 255);
    assert(boot_guard_failures(2, false, true, true) == 0); /* new image */
    assert(boot_guard_failures(2, true, false, true) == 0); /* intentional exit */
    assert(boot_guard_failures(2, true, true, false) == 0); /* power/software reset */
    assert(boot_guard_failures(0, true, true, true) == 1); /* fault after stable boot */

    assert(!boot_guard_due(179999999, 180000000, false, 0, 0));
    assert(boot_guard_due(180000000, 180000000, false, 0, 0)); /* startup hangs */
    /* Static screen/absent phone still tick their task loops. */
    assert(!boot_guard_due(60000000, 0, true, 60000000, 60000000));
    assert(boot_guard_due(60000000, 0, true, 44999999, 60000000)); /* UI deadlock */
    assert(boot_guard_due(60000000, 0, true, 60000000, 44999999)); /* BLE host deadlock */
    assert(!boot_guard_due(60000000, 0, true, 0, 60000000)); /* headless recovery */
    assert(!boot_guard_due(60000000, 0, true, 60000000, 0)); /* BLE init unavailable */
    assert(!boot_guard_due(60000000, 70000000, true, 1, 1)); /* bounded flash grace */
    puts("boot guard policy: PASS (commands/nonces, reset sequences, startup, UI/BLE stalls, intentional UI pause/recovery, grace)");
    return 0;
}

#pragma once

#define MODE_BT_CLASSIC       1
#define MODE_WIRELESS_HELPER  2

#ifndef CONNECTION_MODE
#define CONNECTION_MODE MODE_WIRELESS_HELPER
#endif

#define AA_TCP_PORT            5277
#define AA_MDNS_HOSTNAME       "android-auto"
#define AA_MDNS_INSTANCE_NAME  "ESP32-P4 Android Auto"
#define AA_MDNS_SERVICE_TYPE   "_androidauto"
#define AA_MDNS_PROTO          "_tcp"

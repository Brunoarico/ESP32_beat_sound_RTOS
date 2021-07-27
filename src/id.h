#ifndef ID_H
#define ID_H

#define ESP_A 1
#define ESP_B 2

#define IM ESP_B

#define SSID ""
#define PASS ""

#if IM == ESP_A
#define DEST "192.168.0.191"
#define NAME "ESP_A"

#elif IM == ESP_B
#define DEST "192.168.0.238"
#define NAME "ESP_B"

#elif IM == ESP_C
#define DEST "192.168.0.204"
#define NAME "ESP_C"
#endif

#define PORT 5005

#endif

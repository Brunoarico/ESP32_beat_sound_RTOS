#ifndef ID_H
#define ID_H

#define ESP_A 1
#define ESP_B 2

#define IM ESP_B

#if IM == ESP_A
#define DEST "192.168.0.191"
#define NAME "ESP_A"
#elif IM == ESP_B
#define DEST "192.168.0.203"
#define NAME "ESP_B"
#endif

#define PORT 5005

#endif

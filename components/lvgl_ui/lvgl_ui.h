#ifndef __LVGL_UI_H__
#define __LVGL_UI_H__

#include <stdio.h>
#include "lvgl.h"


#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t cpu_percent;
    uint8_t ram_percent;
    float temperature_c;
    uint32_t uptime_seconds;
    bool network_online;
    char ip_address[16];
    float rx_kbps;
    float tx_kbps;
    bool ha_online;
    bool mqtt_online;
    bool appium_online;
    uint32_t ha_uptime_seconds;
    uint32_t mqtt_uptime_seconds;
    uint32_t appium_uptime_seconds;
    bool power_warning;
} dashboard_data_t;

void lvgl_ui_init(void);
void lvgl_ui_update(const dashboard_data_t *data);

#ifdef __cplusplus
}
#endif

#endif


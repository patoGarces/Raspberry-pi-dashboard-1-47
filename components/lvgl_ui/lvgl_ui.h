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
    uint32_t ram_used_mb;
    uint32_t ram_total_mb;
    float temperature_c;
    uint32_t uptime_seconds;
    bool wifi_connected;
    char ip_address[16];
} dashboard_data_t;

void lvgl_ui_init(void);
void lvgl_ui_update(const dashboard_data_t *data);

#ifdef __cplusplus
}
#endif

#endif


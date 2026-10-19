#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "cJSON.h"
#include "esp_lvgl_port.h"
#include "lvgl_ui.h"
#include "uart_link.h"

#include "driver/usb_serial_jtag.h"

static const char *TAG = "uart_link";
static dashboard_data_t received_data = {0};
static portMUX_TYPE data_mux = portMUX_INITIALIZER_UNLOCKED;
static int64_t last_rx_ms = 0;

static void publish_data(void) {
    dashboard_data_t data;

    portENTER_CRITICAL(&data_mux);
    data = received_data;
    portEXIT_CRITICAL(&data_mux);

    if (lvgl_port_lock(1000)) {
        lvgl_ui_update(&data);
        lvgl_port_unlock();
    }
}

static void parse_message(const char *line) {
    cJSON *root = cJSON_Parse(line);
    if (root == NULL || !cJSON_IsObject(root)) {
        cJSON_Delete(root);
        return;
    }

    dashboard_data_t data = {0};
    cJSON *item;

    #define READ_NUMBER(field) \
        do { \
            item = cJSON_GetObjectItemCaseSensitive(root, #field); \
            if (cJSON_IsNumber(item)) data.field = item->valuedouble; \
        } while (0)

    #define READ_BOOL(field) \
        do { \
            item = cJSON_GetObjectItemCaseSensitive(root, #field); \
            if (cJSON_IsBool(item)) data.field = cJSON_IsTrue(item); \
        } while (0)

    READ_NUMBER(cpu_percent);
    READ_NUMBER(ram_percent);
    READ_NUMBER(temperature_c);
    READ_NUMBER(uptime_seconds);
    READ_BOOL(network_online);

    item = cJSON_GetObjectItemCaseSensitive(root, "ip_address");
    if (cJSON_IsString(item) && item->valuestring != NULL) {
        snprintf(data.ip_address, sizeof(data.ip_address), "%s", item->valuestring);
    }

    READ_NUMBER(rx_kbps);
    READ_NUMBER(tx_kbps);
    READ_BOOL(ha_online);
    READ_BOOL(mqtt_online);
    READ_BOOL(appium_online);
    READ_NUMBER(ha_uptime_seconds);
    READ_NUMBER(mqtt_uptime_seconds);
    READ_NUMBER(appium_uptime_seconds);
    READ_BOOL(power_warning);

    #undef READ_NUMBER
    #undef READ_BOOL

    cJSON_Delete(root);

    portENTER_CRITICAL(&data_mux);
    data.uart_online = true;
    received_data = data;
    last_rx_ms = esp_timer_get_time() / 1000;
    portEXIT_CRITICAL(&data_mux);

    publish_data();
}

// static void uart_rx_task(void *arg) {
//     char line[768];

//     while (true) {
//         if (fgets(line, sizeof(line), stdin) == NULL) {
//             clearerr(stdin);
//             vTaskDelay(pdMS_TO_TICKS(20));
//             continue;
//         }

//         parse_message(line);
//     }
// }
static void uart_rx_task(void *arg) {
    char line[768];
    size_t pos = 0;
    uint8_t byte;

    while (true) {
        int len = usb_serial_jtag_read_bytes(&byte, 1, pdMS_TO_TICKS(100));

        if (len <= 0) continue;

        if (byte == '\n') {
            line[pos] = '\0';
            parse_message(line);
            pos = 0;
        } else if (byte != '\r') {
            if (pos < sizeof(line) - 1) {
                line[pos++] = (char)byte;
            } else {
                pos = 0;
            }
        }
    }
}

static void uart_timeout_task(void *arg) {
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(500));

        bool changed = false;
        int64_t now = esp_timer_get_time() / 1000;

        portENTER_CRITICAL(&data_mux);
        if (received_data.uart_online && now - last_rx_ms > 6000) {
            received_data.uart_online = false;
            changed = true;
        }
        portEXIT_CRITICAL(&data_mux);

        if (changed) {
            ESP_LOGW(TAG, "No data received for 6 seconds");
            publish_data();
        }
    }
}

// void uart_link_init(void) {
//     portENTER_CRITICAL(&data_mux);
//     memset(&received_data, 0, sizeof(received_data));
//     received_data.uart_online = false;
//     last_rx_ms = esp_timer_get_time() / 1000;
//     portEXIT_CRITICAL(&data_mux);

//     xTaskCreate(uart_rx_task, "uart_rx", 4096, NULL, 5, NULL);
//     xTaskCreate(uart_timeout_task, "uart_timeout", 3072, NULL, 4, NULL);
// }
void uart_link_init(void) {
    usb_serial_jtag_driver_config_t usb_config = {
        .tx_buffer_size = 512,
        .rx_buffer_size = 512,
    };

    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&usb_config));

    portENTER_CRITICAL(&data_mux);
    memset(&received_data, 0, sizeof(received_data));
    received_data.uart_online = false;
    last_rx_ms = esp_timer_get_time() / 1000;
    portEXIT_CRITICAL(&data_mux);

    xTaskCreate(uart_rx_task, "uart_rx", 4096, NULL, 5, NULL);
    xTaskCreate(uart_timeout_task, "uart_timeout", 3072, NULL, 4, NULL);
}

// void uart_link_send_command(const char *command) {
//     if (command == NULL) return;

//     portENTER_CRITICAL(&data_mux);
//     bool online = received_data.uart_online;
//     portEXIT_CRITICAL(&data_mux);

//     if (!online) {
//         ESP_LOGW(TAG, "Command not sent: connection offline");
//         return;
//     }

//     printf("CMD:%s\n", command);
//     fflush(stdout);
// }

void uart_link_send_command(const char *command) {
    if (command == NULL) return;

    portENTER_CRITICAL(&data_mux);
    bool online = received_data.uart_online;
    portEXIT_CRITICAL(&data_mux);

    if (!online) {
        ESP_LOGW(TAG, "Command not sent: connection offline");
        return;
    }

    char message[64];
    int len = snprintf(message, sizeof(message), "CMD:%s\n", command);
    if (len > 0 && len < sizeof(message)) {
        usb_serial_jtag_write_bytes(message, len, pdMS_TO_TICKS(100));
    }
}
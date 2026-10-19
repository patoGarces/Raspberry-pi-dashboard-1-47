#include "lvgl_ui.h"
#include "esp_log.h"

#define COLOR_BG       0x10141D
#define COLOR_PANEL    0x1C2330
#define COLOR_ACCENT   0x4DA3FF
#define COLOR_GREEN    0x42D392
#define COLOR_ORANGE   0xFFB454
#define COLOR_TEXT     0xF1F5F9
#define COLOR_MUTED    0x8D9AAF

static lv_obj_t *cpu_value;
static lv_obj_t *ram_value;
static lv_obj_t *temp_value;
static lv_obj_t *uptime_value;
static lv_obj_t *wifi_value;
static lv_obj_t *ip_value;

static lv_obj_t *network_status;
static lv_obj_t *network_speed;
static lv_obj_t *service_ha;
static lv_obj_t *service_mqtt;
static lv_obj_t *service_appium;
static lv_obj_t *control_status;

static lvgl_ui_command_cb_t command_callback = NULL;

static dashboard_data_t current_data = {
    .cpu_percent = 24,
    .ram_percent = 40,
    .temperature_c = 48.5f,
    .uptime_seconds = 86432,

    .network_online = true,
    .ip_address = "192.168.2.113",
    .rx_kbps = 125.0f,
    .tx_kbps = 42.0f,

    .ha_online = true,
    .mqtt_online = true,
    .appium_online = false,
};

void lvgl_ui_set_command_callback(lvgl_ui_command_cb_t callback) {
    command_callback = callback;
}

static lv_obj_t *create_label(
    lv_obj_t *parent,
    const char *text,
    uint32_t color,
    const lv_font_t *font) {
    lv_obj_t *label = lv_label_create(parent);

    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_style_text_font(label, font, 0);

    return label;
}

static lv_obj_t *create_panel(
    lv_obj_t *parent,
    int x,
    int y,
    int width,
    int height) {
    lv_obj_t *panel = lv_obj_create(parent);

    lv_obj_set_pos(panel, x, y);
    lv_obj_set_size(panel, width, height);

    lv_obj_set_style_bg_color(
        panel, lv_color_hex(COLOR_PANEL), 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_radius(panel, 10, 0);
    lv_obj_set_style_pad_all(panel, 8, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    return panel;
}

static void create_overview(lv_obj_t *tile) {
    lv_obj_t *title = create_label(
        tile, "SYSTEM MONITOR", COLOR_TEXT,
        &lv_font_montserrat_16);

    lv_obj_set_width(title, LV_PCT(100));
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    lv_obj_t *cpu_panel = create_panel(tile, 8, 32, 96, 58);
    lv_obj_t *ram_panel = create_panel(tile, 112, 32, 96, 58);
    lv_obj_t *temp_panel = create_panel(tile, 216, 32, 96, 58);

    create_label(cpu_panel, "CPU", COLOR_MUTED,
                 &lv_font_montserrat_12);
    cpu_value = create_label(cpu_panel, "--%",
                             COLOR_ACCENT,
                             &lv_font_montserrat_20);
    lv_obj_align(cpu_value, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    create_label(ram_panel, "RAM", COLOR_MUTED,
                 &lv_font_montserrat_12);
    ram_value = create_label(ram_panel, "--",
                             COLOR_GREEN,
                             &lv_font_montserrat_20);
    lv_obj_align(ram_value, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    create_label(temp_panel, "TEMP", COLOR_MUTED,
                 &lv_font_montserrat_12);
    temp_value = create_label(temp_panel, "--.- C",
                              COLOR_ORANGE,
                              &lv_font_montserrat_16);
    lv_obj_align(temp_value, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    lv_obj_t *status = create_panel(tile, 8, 100, 304, 48);

    create_label(status, "SYSTEM STATUS", COLOR_MUTED,
                 &lv_font_montserrat_12);

    wifi_value = create_label(status, "Wi-Fi: --",
                              COLOR_TEXT,
                              &lv_font_montserrat_14);
    lv_obj_align(wifi_value, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    uptime_value = create_label(status, "Uptime: --",
                                COLOR_TEXT,
                                &lv_font_montserrat_14);
    lv_obj_align(uptime_value, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
}


static void create_network(lv_obj_t *tile) {
    lv_obj_t *title = create_label(
        tile, "NETWORK", COLOR_TEXT,
        &lv_font_montserrat_16);

    lv_obj_set_width(title, LV_PCT(100));
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    lv_obj_t *panel = create_panel(tile, 8, 40, 304, 108);

    network_status = create_label(
        panel, "Status: --", COLOR_GREEN,
        &lv_font_montserrat_14);
    lv_obj_align(network_status, LV_ALIGN_TOP_LEFT, 0, 0);

    ip_value = create_label(
        panel, "IP: --", COLOR_TEXT,
        &lv_font_montserrat_14);
    lv_obj_align(ip_value, LV_ALIGN_TOP_LEFT, 0, 26);

    network_speed = create_label(
        panel, "Down: --  Up: --", COLOR_ACCENT,
        &lv_font_montserrat_14);
    lv_obj_align(network_speed, LV_ALIGN_TOP_LEFT, 0, 54);
}


static void create_services(lv_obj_t *tile) {
    lv_obj_t *title = create_label(
        tile, "SERVICES", COLOR_TEXT,
        &lv_font_montserrat_16);

    lv_obj_set_width(title, LV_PCT(100));
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    lv_obj_t *panel = create_panel(tile, 8, 40, 304, 108);

    service_ha = create_label(
        panel, "Home Assistant: --", COLOR_TEXT,
        &lv_font_montserrat_12);
    lv_obj_align(service_ha, LV_ALIGN_TOP_LEFT, 0, 0);

    service_mqtt = create_label(
        panel, "MQTT: --", COLOR_TEXT,
        &lv_font_montserrat_12);
    lv_obj_align(service_mqtt, LV_ALIGN_TOP_LEFT, 0, 23);

    service_appium = create_label(
        panel, "Appium: --", COLOR_TEXT,
        &lv_font_montserrat_12);
    lv_obj_align(service_appium, LV_ALIGN_TOP_LEFT, 0, 46);
}


static void control_button_event(lv_event_t *e) {
    const char *command = lv_event_get_user_data(e);

    if (command_callback != NULL) {
        command_callback(command);
    }
}

static void create_controls(lv_obj_t *tile) {
    lv_obj_t *title = create_label(
        tile, "CONTROLS", COLOR_TEXT,
        &lv_font_montserrat_16);

    lv_obj_set_width(title, LV_PCT(100));
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    lv_obj_t *reboot = lv_btn_create(tile);
    lv_obj_set_size(reboot, 120, 100);
    lv_obj_align(reboot, LV_ALIGN_TOP_LEFT, 28, 38);
    lv_obj_set_style_bg_opa(reboot, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(reboot, 2, 0);
    lv_obj_set_style_border_color(reboot, lv_color_hex(0x3498DB), 0);
    lv_obj_set_style_radius(reboot, 16, 0);
    lv_obj_set_style_shadow_width(reboot, 0, 0);
    lv_obj_set_style_bg_color(reboot, lv_color_hex(0x3498DB), LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(reboot, LV_OPA_20, LV_STATE_PRESSED);

    lv_obj_t *reboot_icon = lv_label_create(reboot);
    lv_label_set_text(reboot_icon, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_font(reboot_icon, &lv_font_montserrat_38, 0);
    lv_obj_set_style_text_color(reboot_icon, lv_color_hex(0x3498DB), 0);
    lv_obj_align(reboot_icon, LV_ALIGN_TOP_MID, 0, 5);

    lv_obj_t *reboot_label = lv_label_create(reboot);
    lv_label_set_text(reboot_label, "REBOOT");
    lv_obj_set_style_text_font(reboot_label, &lv_font_montserrat_12, 0);

    lv_obj_set_style_text_color(reboot_label, lv_color_hex(COLOR_TEXT), 0);
    lv_obj_align(reboot_label, LV_ALIGN_BOTTOM_MID, 0, -8);

    lv_obj_add_event_cb(reboot, control_button_event, LV_EVENT_CLICKED, "REBOOT");

    lv_obj_t *shutdown = lv_btn_create(tile);
    lv_obj_set_size(shutdown, 120, 100);
    lv_obj_align(shutdown, LV_ALIGN_TOP_RIGHT, -28, 38);
    lv_obj_set_style_bg_opa(shutdown, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(shutdown, 2, 0);
    lv_obj_set_style_border_color(shutdown, lv_color_hex(0xE74C3C), 0);
    lv_obj_set_style_radius(shutdown, 16, 0);
    lv_obj_set_style_shadow_width(shutdown, 0, 0);
    lv_obj_set_style_bg_color(shutdown, lv_color_hex(0xE74C3C), LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(shutdown, LV_OPA_20, LV_STATE_PRESSED);

    lv_obj_t *shutdown_icon = lv_label_create(shutdown);
    lv_label_set_text(shutdown_icon, LV_SYMBOL_POWER);
    lv_obj_set_style_text_font(shutdown_icon, &lv_font_montserrat_38, 0);
    lv_obj_set_style_text_color(shutdown_icon, lv_color_hex(0xE74C3C), 0);
    lv_obj_align(shutdown_icon, LV_ALIGN_TOP_MID, 0, 5);

    lv_obj_t *shutdown_label = lv_label_create(shutdown);
    lv_label_set_text(shutdown_label, "SHUTDOWN");
    lv_obj_set_style_text_font(shutdown_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(shutdown_label, lv_color_hex(COLOR_TEXT), 0);
    lv_obj_align(shutdown_label, LV_ALIGN_BOTTOM_MID, 0, -8);

    lv_obj_add_event_cb(shutdown, control_button_event, LV_EVENT_CLICKED, "SHUTDOWN");

    control_status = create_label(
        tile, "UART control not connected", COLOR_ORANGE,
        &lv_font_montserrat_12);

    lv_obj_set_width(control_status, LV_PCT(100));
    lv_obj_set_style_text_align(control_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(control_status, LV_ALIGN_BOTTOM_MID, 0, -8);
}


void lvgl_ui_update(const dashboard_data_t *data) {
    if (data == NULL || cpu_value == NULL) {
        return;
    }

    current_data = *data;

    char text[64];

    snprintf(text, sizeof(text), "%u%%",
             current_data.cpu_percent);
    lv_label_set_text(cpu_value, text);

    snprintf(text, sizeof(text), "%u%%",
             current_data.ram_percent);
    lv_label_set_text(ram_value, text);

    snprintf(text, sizeof(text), "%.1f C",
             (double)current_data.temperature_c);
    lv_label_set_text(temp_value, text);

    uint32_t hours = current_data.uptime_seconds / 3600;
    uint32_t minutes = (current_data.uptime_seconds % 3600) / 60;

    snprintf(text, sizeof(text), "Up: %luh %lum",
             (unsigned long)hours,
             (unsigned long)minutes);
    lv_label_set_text(uptime_value, text);

    snprintf(text, sizeof(text), "Wi-Fi: %s",
             current_data.network_online ? "Connected" : "Offline");
    lv_label_set_text(wifi_value, text);

    snprintf(text, sizeof(text), "Status: %s",
             current_data.network_online ? "Online" : "Offline");
    lv_label_set_text(network_status, text);

    snprintf(text, sizeof(text), "IP: %s",
             current_data.ip_address);
    lv_label_set_text(ip_value, text);

    snprintf(text, sizeof(text), "Down: %.1f  Up: %.1f KB/s",
             (double)current_data.rx_kbps,
             (double)current_data.tx_kbps);
    lv_label_set_text(network_speed, text);

    snprintf(text, sizeof(text), "Home Assistant: %s",
             current_data.ha_online ? "Online" : "Offline");
    lv_label_set_text(service_ha, text);

    snprintf(text, sizeof(text), "MQTT: %s",
             current_data.mqtt_online ? "Online" : "Offline");
    lv_label_set_text(service_mqtt, text);

    snprintf(text, sizeof(text), "Appium: %s",
             current_data.appium_online ? "Online" : "Offline");
    lv_label_set_text(service_appium, text);

    if (control_status != NULL) {
        if (data->uart_online) {
            lv_label_set_text(control_status, "UART connected");
            lv_obj_set_style_text_color(
                control_status, lv_color_hex(0x2ECC71), 0);
        } else {
            lv_label_set_text(control_status, "UART control not connected");
            lv_obj_set_style_text_color(
                control_status, lv_color_hex(COLOR_ORANGE), 0);
        }
    }
}

void lvgl_ui_init(void) {
    lv_obj_t *screen = lv_scr_act();

    lv_obj_set_style_bg_color(
        screen, lv_color_hex(COLOR_BG), 0);

    lv_obj_t *tileview = lv_tileview_create(screen);
    lv_obj_set_size(tileview, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(tileview, 0, 0);

    lv_obj_set_style_bg_color(tileview, lv_color_hex(COLOR_BG), 0);
    lv_obj_set_style_bg_opa(tileview, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(tileview, 0, 0);
    lv_obj_set_style_pad_all(tileview, 0, 0);

    lv_obj_t *overview = lv_tileview_add_tile(
        tileview, 0, 0, LV_DIR_RIGHT);

    lv_obj_t *network = lv_tileview_add_tile(
        tileview, 1, 0, LV_DIR_LEFT | LV_DIR_RIGHT);

    lv_obj_t *services = lv_tileview_add_tile(
        tileview, 2, 0, LV_DIR_LEFT | LV_DIR_RIGHT);

    lv_obj_t *controls = lv_tileview_add_tile(
        tileview, 3, 0, LV_DIR_LEFT);

    create_overview(overview);
    create_network(network);
    create_services(services);
    create_controls(controls);

    lvgl_ui_update(&current_data);
}
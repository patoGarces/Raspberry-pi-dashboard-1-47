#include "lvgl_ui.h"


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

static dashboard_data_t current_data = {
    .cpu_percent = 24,
    .ram_used_mb = 412,
    .ram_total_mb = 1024,
    .temperature_c = 48.5f,
    .uptime_seconds = 86432,
    .wifi_connected = true,
    .ip_address = "192.168.2.113"
};

static lv_obj_t *create_label(
    lv_obj_t *parent,
    const char *text,
    uint32_t color,
    const lv_font_t *font)
{
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
    int height)
{
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

static void create_overview(lv_obj_t *tile)
{
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

static void create_system(lv_obj_t *tile)
{
    lv_obj_t *title = create_label(
    tile, "SYSTEM DETAILS", COLOR_TEXT,
    &lv_font_montserrat_16);

    lv_obj_set_width(title, LV_PCT(100));
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    lv_obj_t *panel = create_panel(tile, 8, 32, 304, 116);

    create_label(panel, "NETWORK", COLOR_MUTED,
                 &lv_font_montserrat_12);

    ip_value = create_label(panel, "IP: --",
                            COLOR_GREEN,
                            &lv_font_montserrat_16);
    lv_obj_align(ip_value, LV_ALIGN_TOP_LEFT, 0, 24);
}

void lvgl_ui_update(const dashboard_data_t *data)
{
    if (data == NULL) {
        return;
    }

    current_data = *data;

    if (cpu_value == NULL) {
        return;
    }

    char text[48];

    snprintf(text, sizeof(text), "%u%%",
             current_data.cpu_percent);
    lv_label_set_text(cpu_value, text);

    // snprintf(text, sizeof(text), "%lu MB",(unsigned long)current_data.ram_used_mb);
    // lv_label_set_text(ram_value, text);

    uint32_t ram_percent = current_data.ram_total_mb > 0
    ? current_data.ram_used_mb * 100 / current_data.ram_total_mb : 0;

    snprintf(text, sizeof(text), "%lu%%", (unsigned long)ram_percent);

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
             current_data.wifi_connected ? "Connected" : "Offline");
    lv_label_set_text(wifi_value, text);

    snprintf(text, sizeof(text), "IP: %s",
             current_data.ip_address);
    lv_label_set_text(ip_value, text);
}

void lvgl_ui_init(void)
{
    lv_obj_t *screen = lv_scr_act();

    lv_obj_set_style_bg_color(
        screen, lv_color_hex(COLOR_BG), 0);

    lv_obj_t *tileview = lv_tileview_create(screen);
    lv_obj_set_size(tileview, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(tileview, 0, 0);

    lv_obj_set_style_bg_color(
        tileview, lv_color_hex(COLOR_BG), 0);
    lv_obj_set_style_bg_opa(tileview, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(tileview, 0, 0);
    lv_obj_set_style_pad_all(tileview, 0, 0);

    lv_obj_t *overview = lv_tileview_add_tile(
        tileview, 0, 0, LV_DIR_RIGHT);

    lv_obj_t *system = lv_tileview_add_tile(
        tileview, 1, 0, LV_DIR_LEFT);

    create_overview(overview);
    create_system(system);

    lvgl_ui_update(&current_data);
}
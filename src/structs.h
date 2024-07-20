#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdbool.h>
#include <xcb/xcb.h>
#include <xcb/xcb_util.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_keysyms.h>
#include <X11/keysym.h>

#define CORNER_MARGIN 15
#define EDGE_MARGIN 5

typedef enum {
    LEFT,
    RIGHT,
    TOP,
    BOTTOM
} Direction;

typedef enum {
    VERTICAL,
    HORIZONTAL
} Orientation;


typedef enum {
    FLOATING_MODE,
    MONOCLE_MODE,
    STACKING_MODE, // TODO: maybe split into HORIZONTAL_STACKING_MODE and VERTICAL_STACKING_MODE
    COLUMNS_MODE,
    ROWS_MODE,
    N_COLUMNS_MODE,
    N_ROWS_MODE
} LayoutMode;

typedef struct {
    char *path;
    time_t last_change;
    uint8_t workspace_count;
    uint8_t *workspace_layouts;
    uint8_t max_clients;
    bool focus_follows_mouse;
    uint16_t clients_gap;
    uint16_t border_width;
    uint32_t border_color;
    uint32_t focused_border_color;
    uint32_t unfocused_border_color;
    uint16_t *margin;
    uint16_t margin_left;
    uint16_t margin_top;
    uint16_t margin_right;
    uint16_t margin_bottom;
    uint16_t min_window_width;
    uint16_t min_window_height;
    uint32_t mod1_key;
    uint32_t mod4_key;
    float stacking_mode_master_window_ratio;
    Direction stacking_mode_master_window_position;
    Orientation stacking_mode_master_window_orientation;
    uint8_t n_columns_mode_number_of_columns;
    uint8_t n_rows_mode_number_of_rows;
} Config;

typedef struct {
    char name[32];
    char key[32];
    char value[32];
} Rules;

typedef struct {
    uint32_t stack_mode;
    int16_t x_pos;
    int16_t y_pos;
    uint16_t width;
    uint16_t height;
    uint16_t border_width;
    bool manual_resize;
    bool is_floating;
    int16_t floating_x_pos;
    int16_t floating_y_pos;
    uint16_t floating_width;
    uint16_t floating_height;
    bool is_fullscreen;
    int16_t before_fullscreen_x_pos;
    int16_t before_fullscreen_y_pos;
    uint16_t before_fullscreen_width;
    uint16_t before_fullscreen_height;
    char *class_name;
    char *instance_name;
} ClientProperties;

typedef struct {
    xcb_window_t window;
    ClientProperties properties;
    uint8_t workspace_id;
    bool is_mapped;
} Client;

typedef struct {
    Client *clients;
    Client **layout_clients;
    int num_clients;
    int num_clients_in_layout_clients;
    int available_slots;
    int focused_client_id;
    int last_focused_client_id;
    // int focused_client_id_before_fullscreen;
    LayoutMode layout_mode;
    // TODO: workspace configs (?)
} Workspace;

typedef struct {
    xcb_connection_t *connection;
    xcb_screen_t *screen;
    xcb_ewmh_connection_t ewmh;
    xcb_window_t wm_window;
    xcb_cursor_t cursor;
    xcb_void_cookie_t cookie;
    xcb_generic_error_t *error;
    xcb_key_symbols_t *key_symbols;
    Workspace *workspaces;
    int num_workspaces;
    int current_workspace_id;
    // bool running;
    Config config;
    Rules *rules;
    int num_rules;
    bool button_pressed;
    int16_t cursor_x;
    int16_t cursor_y;
} WindowManager;

#endif
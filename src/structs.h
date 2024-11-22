#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdbool.h>
#include <xcb/xcb.h>
#include <xcb/xcb_util.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_keysyms.h>

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
    int16_t x_pos;
    int16_t y_pos;
    uint16_t width;
    uint16_t height;
    uint16_t border_width;
    uint16_t depth;
    bool is_fullscreen;
    bool is_floating;
    int floating_x_pos;
    int floating_y_pos;
    int floating_width;
    int floating_height;
    bool override_redirect;
} ClientProperties;

typedef struct {
    xcb_window_t parent;
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
    LayoutMode layout_mode;
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
    //int16_t cursor_x;
    //int16_t cursor_y;
    int max_clients; // TODO: replace this (it should be set by configuration file)
} WindowManager;

#endif
#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdbool.h>
#include <xcb/xcb.h>
#include <xcb/xcb_util.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_keysyms.h>

typedef struct {
    xcb_window_t parent;
    xcb_window_t window;
    //ClientProperties properties;
    uint8_t workspace_id;
    bool is_mapped;
} Client;

typedef struct {
    Client *clients;
    Client **layout_clients;
    int num_clients;
    int num_clients_in_layout_clients;
    int available_slots;
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
} WindowManager;

#endif
#ifndef CLIENT_H
#define CLIENT_H

#include "structs.h"

Client* create_client();
Client* get_client(WindowManager *gwm, xcb_window_t window);
int destroy_client(WindowManager *gwm, Client *client);
int resize_client(WindowManager *gwm, Client *client, uint16_t width, uint16_t height);
int move_client(WindowManager *gwm, Client *client, int16_t x, int16_t y);
int get_client_index(WindowManager *gwm, xcb_window_t window);
int toggle_visibility(WindowManager *gwm, Client *client, int is_visible);
int toggle_floating(WindowManager *gwm, Client *client, bool is_floating);
int set_client_focus(WindowManager *gwm, Client *client, int client_index);
int set_client_border_width(WindowManager *gwm, Client *client, uint16_t border_width);
int set_client_border_color(WindowManager *gwm, Client *client, uint32_t color);
int apply_client_rules(WindowManager *gwm, Client *client);
int raise_client(WindowManager *gwm, Client *client);
#endif
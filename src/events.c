#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>
#include <xcb/xcb_util.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_icccm.h>
#include <X11/keysym.h>
#include <xcb/randr.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <string.h>
#include "utils.h"
#include "config.h"
#include "events.h"
#include "structs.h"
#include "client.h"
#include "layout.h"
#include "workspace.h"

// TODO: check if this is the best flag to keep the event loop and handling signals
volatile sig_atomic_t run = 1;

/* TODO: this will be useful when the root window receives events (new screen, etc)
int handle_configure_notify(WindowManager *gwm, xcb_generic_event_t *event){
    xcb_configure_notify_event_t *configure_notify_event = (xcb_configure_notify_event_t *)event;
    return 0;
}*/

int handle_motion_notify(WindowManager *gwm, xcb_generic_event_t *event){
    xcb_motion_notify_event_t *motion_notify_event = (xcb_motion_notify_event_t *)event;

    Client *client = get_client(gwm, motion_notify_event->event);
    if(client == NULL)
        return -1;

    /*if(motion_notify_event->event_x >= 0 && motion_notify_event->event_x <= 5 || motion_notify_event->event_x >= client->properties.width - 5 && motion_notify_event->event_x <= client->properties.width){ // left border
        setCursor(gwm->connection, gwm->screen, gwm->screen->root, 108);
    } else if(motion_notify_event->event_y >= 0 && motion_notify_event->event_y <= 5 || motion_notify_event->event_y >= client->properties.height - 5 && motion_notify_event->event_y <= client->properties.height){ // upper border
        setCursor(gwm->connection, gwm->screen, gwm->screen->root, 116);
    }*/

    if(motion_notify_event->state & XCB_BUTTON_MASK_1 && client->properties.is_floating){
        int16_t dragdelta_x = motion_notify_event->root_x - gwm->cursor_x;
        int16_t dragdelta_y = motion_notify_event->root_y - gwm->cursor_y;
        move_client(gwm, client, client->properties.floating_x_pos + dragdelta_x, client->properties.floating_y_pos + dragdelta_y);
    } else if (motion_notify_event->state & XCB_BUTTON_MASK_3 && client->properties.is_floating) {
        int16_t resize_delta_x = motion_notify_event->root_x - gwm->cursor_x;
        int16_t resize_delta_y = motion_notify_event->root_y - gwm->cursor_y;

        bool resize_horizontal = false;
        bool resize_vertical = false;
        bool move_horizontal = false;
        bool move_vertical = false;

        if (gwm->cursor_x >= client->properties.x_pos - CORNER_MARGIN && gwm->cursor_x <= client->properties.x_pos + CORNER_MARGIN) {
            // Left edge or top-left/bottom-left corner
            resize_horizontal = true;
            move_horizontal = true;
        } else if (gwm->cursor_x >= client->properties.x_pos + client->properties.width - CORNER_MARGIN && gwm->cursor_x <= client->properties.x_pos + client->properties.width + CORNER_MARGIN) {
            // Right edge or top-right/bottom-right corner
            resize_horizontal = true;
        }

        if (gwm->cursor_y >= client->properties.y_pos - CORNER_MARGIN && gwm->cursor_y <= client->properties.y_pos + CORNER_MARGIN) {
            // Top edge or top-left/top-right corner
            resize_vertical = true;
            move_vertical = true;
        } else if (gwm->cursor_y >= client->properties.y_pos + client->properties.height - CORNER_MARGIN && gwm->cursor_y <= client->properties.y_pos + client->properties.height + CORNER_MARGIN) {
            // Bottom edge or bottom-left/bottom-right corner
            resize_vertical = true;
        }

        if (resize_horizontal || resize_vertical) {
            int new_width = client->properties.width;
            int new_height = client->properties.height;
            int new_x_pos = client->properties.floating_x_pos;
            int new_y_pos = client->properties.floating_y_pos;

            if (resize_horizontal) {
                new_width = client->properties.width + (move_horizontal ? -resize_delta_x : resize_delta_x);
                if (move_horizontal) new_x_pos += resize_delta_x;
            }

            if (resize_vertical) {
                new_height = client->properties.height + (move_vertical ? -resize_delta_y : resize_delta_y);
                if (move_vertical) new_y_pos += resize_delta_y;
            }

            if(new_width < 300) new_width = 300;
            if(new_height < 150) new_height = 150;

            if (move_horizontal || move_vertical) {
                move_client(gwm, client, new_x_pos, new_y_pos);
            }

            resize_client(gwm, client, new_width, new_height);
        }
    }

    gwm->cursor_x = motion_notify_event->root_x;
    gwm->cursor_y = motion_notify_event->root_y;
    return 0;
}

int handle_enter_notify(WindowManager *gwm, xcb_generic_event_t *event){
    xcb_enter_notify_event_t *enter_notify_event = (xcb_enter_notify_event_t *)event;

    int client_index = get_client_index(gwm, enter_notify_event->event);
    if(client_index == -1)
        return -1;

    Client *client = &gwm->workspaces[gwm->current_workspace_id].clients[client_index];
    if(gwm->config.focus_follows_mouse){
        set_client_focus(gwm, client, client_index);
    }

    xcb_flush(gwm->connection);
    return 0;
}

int handle_button_press(WindowManager *gwm, xcb_generic_event_t *event){
    xcb_button_press_event_t *button_press_event = (xcb_button_press_event_t *)event;

    Client *client = get_client(gwm, button_press_event->event);
    if(client == NULL)
        return -1;

    if(button_press_event->detail == 1 || button_press_event->detail == 3){
        gwm->button_pressed = true;
        gwm->cursor_x = button_press_event->root_x;
        gwm->cursor_y = button_press_event->root_y;

        if(client->properties.is_floating)
            raise_client(gwm, client);

        set_client_focus(gwm, client, get_client_index(gwm, button_press_event->event));
    }

    xcb_flush(gwm->connection);
    return 0;
}

int handle_configure_request(WindowManager *gwm, xcb_generic_event_t *event){
    xcb_configure_request_event_t *configure_request_event = (xcb_configure_request_event_t *)event;

    xcb_configure_window_value_list_t changes;
    changes.x = configure_request_event->x;
    changes.y = configure_request_event->y;
    changes.width = configure_request_event->width;
    changes.height = configure_request_event->height;
    changes.border_width = configure_request_event->border_width;
    changes.sibling = configure_request_event->sibling;
    changes.stack_mode = configure_request_event->stack_mode;

    Client *client = get_client(gwm, configure_request_event->window);
    if(client != NULL){
        client->properties.x_pos = configure_request_event->x;
        client->properties.y_pos = configure_request_event->y;
        client->properties.width = configure_request_event->width;
        client->properties.height = configure_request_event->height;
        client->properties.border_width = configure_request_event->border_width;
        client->properties.stack_mode = configure_request_event->stack_mode;

        xcb_configure_window(gwm->connection, client->window, configure_request_event->value_mask, &changes);
    } else {
        xcb_configure_window(gwm->connection, configure_request_event->window, configure_request_event->value_mask, &changes);
    }
    return 0;
}

int handle_destroy_notify(WindowManager *gwm, xcb_generic_event_t *event){
    xcb_destroy_notify_event_t *destroy_notify_event = (xcb_destroy_notify_event_t *)event;

    Client *client = get_client(gwm, destroy_notify_event->window);
    if(client == NULL)
        return -1;

    remove_client_from_workspace(&gwm->workspaces[gwm->current_workspace_id], client);
    /*if(gwm->workspaces[gwm->current_workspace_id].focused_client_id != -1){
        xcb_ewmh_set_active_window(&gwm->ewmh, 0, gwm->workspaces[gwm->current_workspace_id].clients[gwm->workspaces[gwm->current_workspace_id].focused_client_id].window);
        xcb_set_input_focus(gwm->connection, XCB_INPUT_FOCUS_POINTER_ROOT, gwm->workspaces[gwm->current_workspace_id].clients[gwm->workspaces[gwm->current_workspace_id].focused_client_id].window, XCB_CURRENT_TIME);
    }*/

    establish_layout(gwm);
    xcb_flush(gwm->connection);
    return 0;
}

bool is_key_pressed(xcb_key_symbols_t *key_symbols, xcb_keycode_t key_code, xcb_keycode_t target_key_code) {
    xcb_keycode_t *keycodes = xcb_key_symbols_get_keycode(key_symbols, target_key_code);
    if(keycodes == NULL)
        return false;
    
    xcb_keycode_t keycode = keycodes[0];
    free(keycodes);
    return key_code == keycode;
}

// KeyPress with MOD1 (Alt)
void handle_mod1_key_press(WindowManager *gwm, xcb_key_press_event_t *key_event){
    if(is_key_pressed(gwm->key_symbols, key_event->detail, XK_Q)){
        log_message(LOG_DEBUG, "ALT + Q pressed");
        // running = false;
        if(fork() == 0){ // TODO: implement a better spawn keybind
            setsid();
            char *const argv[] = {"kitty", NULL};
            execvp(argv[0], argv);
        }
    } else if(is_key_pressed(gwm->key_symbols, key_event->detail, XK_L)){ // reset workspace layout
        for(int i = 0; i < gwm->workspaces[gwm->current_workspace_id].num_clients; i++){
            if(gwm->workspaces[gwm->current_workspace_id].clients[i].properties.manual_resize){
                gwm->workspaces[gwm->current_workspace_id].clients[i].properties.manual_resize = false;
            }
        }
        establish_layout(gwm);
    } else if(is_key_pressed(gwm->key_symbols, key_event->detail, (xcb_keycode_t)XK_Tab)){
        cycle_through_workspaces(gwm);
    }


}

// KeyPress with MOD4 + Shift
void handle_mod4_shift_key_press(WindowManager *gwm, xcb_key_press_event_t *key_event, Client *client) {
    if(is_key_pressed(gwm->key_symbols, key_event->detail, (xcb_keycode_t)XK_Up)) {
        // Mod4 + Shift + Up
    } else if(is_key_pressed(gwm->key_symbols, key_event->detail, (xcb_keycode_t)XK_Down)){
        // Mod4 + Shift + Down
    } else if(is_key_pressed(gwm->key_symbols, key_event->detail, (xcb_keycode_t)XK_Left)){
        // TODO: set decrement to config property
        resize_client(gwm, client, client->properties.width - 15, client->properties.height);
        establish_layout(gwm);
    } else if(is_key_pressed(gwm->key_symbols, key_event->detail, (xcb_keycode_t)XK_Right)){
        // TODO: fix bug - if there are 3 clients (1 master and 2 other), if master is resized the others stay out of the screen
        resize_client(gwm, client, client->properties.width + 15, client->properties.height);
        establish_layout(gwm);
    }
}

void kill_client(WindowManager *gwm, xcb_window_t window) {
    xcb_intern_atom_cookie_t wm_protocols_cookie, wm_delete_window_cookie;
    xcb_intern_atom_reply_t *wm_protocols_reply, *wm_delete_window_reply;
    xcb_client_message_event_t ev;

    wm_protocols_cookie = xcb_intern_atom(gwm->connection, 1, strlen("WM_PROTOCOLS"), "WM_PROTOCOLS");
    wm_protocols_reply = xcb_intern_atom_reply(gwm->connection, wm_protocols_cookie, NULL);
    if (!wm_protocols_reply) {
        fprintf(stderr, "Falha ao obter WM_PROTOCOLS atom\n");
        return;
    }

    wm_delete_window_cookie = xcb_intern_atom(gwm->connection, 0, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");
    wm_delete_window_reply = xcb_intern_atom_reply(gwm->connection, wm_delete_window_cookie, NULL);
    if (!wm_delete_window_reply) {
        fprintf(stderr, "Falha ao obter WM_DELETE_WINDOW atom\n");
        free(wm_protocols_reply);
        return;
    }

    ev.response_type = XCB_CLIENT_MESSAGE;
    ev.window = window;
    ev.type = wm_protocols_reply->atom;
    ev.format = 32;
    ev.data.data32[0] = wm_delete_window_reply->atom;
    ev.data.data32[1] = XCB_CURRENT_TIME;

    xcb_send_event(gwm->connection, 0, window, XCB_EVENT_MASK_NO_EVENT, (const char *)&ev);
    xcb_flush(gwm->connection);

    free(wm_protocols_reply);
    free(wm_delete_window_reply);
}

// KeyPress with MOD4 (Super)
void handle_mod4_key_press(WindowManager *gwm, xcb_key_press_event_t *key_event) {
    int client_index = get_client_index(gwm, key_event->child);
    if(client_index == -1)
        return;

    Client *client = &gwm->workspaces[gwm->current_workspace_id].clients[client_index];
    
    if(is_key_pressed(gwm->key_symbols, key_event->detail, XK_C)){ // kill client
        kill_client(gwm, client->window); // xcb_client_kill was the previous function
    } else if(is_key_pressed(gwm->key_symbols, key_event->detail, XK_F)){ // toggle floating
        log_message(LOG_DEBUG, "toggled floating");
        toggle_floating(gwm, client, !client->properties.is_floating);
        establish_layout(gwm);
    } else if(is_key_pressed(gwm->key_symbols, key_event->detail, XK_L)){ // reset client layout
        client->properties.manual_resize = false;
        establish_layout(gwm);
    } else if(is_key_pressed(gwm->key_symbols, key_event->detail, XK_space)){ // swap with master
        // TODO: stop using clients[0], use layout_clients[0]
        Client temp = gwm->workspaces[gwm->current_workspace_id].clients[0];
        gwm->workspaces[gwm->current_workspace_id].clients[0] = gwm->workspaces[gwm->current_workspace_id].clients[client_index];
        gwm->workspaces[gwm->current_workspace_id].clients[client_index] = temp;
        establish_layout(gwm);
    }

    if(key_event->state & XCB_MOD_MASK_SHIFT){
        handle_mod4_shift_key_press(gwm, key_event, client);
    }
}

int handle_key_press(WindowManager *gwm, xcb_generic_event_t *event){
    xcb_key_press_event_t *key_event = (xcb_key_press_event_t *)event;
    if(key_event->state & XCB_MOD_MASK_1){
        handle_mod1_key_press(gwm, key_event);
    } else if(key_event->state & XCB_MOD_MASK_4){
        handle_mod4_key_press(gwm, key_event);
    }

    return 0;
}

int handle_map_request(WindowManager *gwm, xcb_generic_event_t *event){
    xcb_map_request_event_t *map_request_event = (xcb_map_request_event_t *)event;

    xcb_get_window_attributes_cookie_t window_attributes_cookie = xcb_get_window_attributes(gwm->connection, map_request_event->window);
    xcb_get_window_attributes_reply_t *window_attributes_reply = xcb_get_window_attributes_reply(gwm->connection, window_attributes_cookie, NULL);
    if(window_attributes_reply->override_redirect){
        free(window_attributes_reply);
        return -1;
    }

    /*bool ignore_window = false;
    xcb_ewmh_get_atoms_reply_t win_type;
    if (xcb_ewmh_get_wm_window_type_reply(&gwm->ewmh, xcb_ewmh_get_wm_window_type(&gwm->ewmh, map_request_event->window), &win_type, NULL) == 1) {
        for (unsigned int i = 0; i < win_type.atoms_len; i++) {
            xcb_atom_t a = win_type.atoms[i];
            if (a == gwm->ewmh._NET_WM_WINDOW_TYPE_DESKTOP || a == gwm->ewmh._NET_WM_WINDOW_TYPE_DOCK || a == gwm->ewmh._NET_WM_WINDOW_TYPE_TOOLBAR || a == gwm->ewmh._NET_WM_WINDOW_TYPE_MENU || a == gwm->ewmh._NET_WM_WINDOW_TYPE_SPLASH || a == gwm->ewmh._NET_WM_WINDOW_TYPE_UTILITY) {
                ignore_window = true;
                break;
            }
        }
        xcb_ewmh_get_atoms_reply_wipe(&win_type);
    } else {
        log_message(LOG_WARNING, "failed to get _NET_WM_WINDOW_TYPE property of window: %d", map_request_event->window);
        //ignore_window = true;
    }

    if(ignore_window){
        free(window_attributes_reply);
        xcb_map_window(gwm->connection, map_request_event->window);
        xcb_flush(gwm->connection);
        return -1;
    }*/

    if(get_client(gwm, map_request_event->window) != NULL){
        free(window_attributes_reply);
        return -1;
    }

    free(window_attributes_reply);

    Client *client = create_client();
    if(client == NULL){
        fprintf(stderr, "failed to create client\n");
        exit(EXIT_FAILURE); // TODO: revise this exit() call
    }

    client->window = map_request_event->window;
    add_client_to_workspace(gwm, &gwm->workspaces[gwm->current_workspace_id], client);
    set_client_border_width(gwm, client, 1);

    //apply_client_rules(gwm, client);
    establish_layout(gwm); // TODO: check error code before
    xcb_map_window(gwm->connection, client->window);
    set_client_focus(gwm, client, gwm->workspaces[gwm->current_workspace_id].num_clients - 1);
    xcb_flush(gwm->connection);
    free(client);
    return 0;
}

int handle_unmap_notify(WindowManager *gwm, xcb_generic_event_t *event){
    xcb_unmap_notify_event_t *unmap_notify_event = (xcb_unmap_notify_event_t *)event;

    Client *client = get_client(gwm, unmap_notify_event->window);
    if(client == NULL)
        return -1;

    establish_layout(gwm);
    xcb_flush(gwm->connection);
    return 0;
}

void sigint_handler(){
    run = 0;
}

int handle_events(WindowManager *gwm){
    xcb_generic_event_t *event;
    signal(SIGINT, sigint_handler);

    while(run){
        if((event = xcb_wait_for_event(gwm->connection)) != NULL){
            //printf("Received event: %d\n", event->response_type & ~0x80);
            switch(event->response_type & ~0x80){
                case XCB_BUTTON_PRESS:
                    handle_button_press(gwm, event);
                    break;
                case XCB_ENTER_NOTIFY:
                    handle_enter_notify(gwm, event);
                    break;
                case XCB_CONFIGURE_REQUEST:
                    handle_configure_request(gwm, event);
                    break;
                case XCB_DESTROY_NOTIFY:
                    handle_destroy_notify(gwm, event);          
                    break;
                case XCB_KEY_PRESS:
                    handle_key_press(gwm, event);
                    break;
                case XCB_MAP_REQUEST:
                    handle_map_request(gwm, event);
                    break;
                case XCB_UNMAP_NOTIFY:
                    handle_unmap_notify(gwm, event);
                    break;
                case XCB_MOTION_NOTIFY:
                    handle_motion_notify(gwm, event);
                    break;
                default:
                    break;
            }
        }

        free(event);
    }
    return 0;
}

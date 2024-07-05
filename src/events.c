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

// TODO: implement movement and resize of floating windows
int handle_motion_notify(WindowManager *gwm, xcb_generic_event_t *event){
    xcb_motion_notify_event_t *motion_notify_event = (xcb_motion_notify_event_t *)event;

    Client *client = get_client(gwm, motion_notify_event->event);
    if(client == NULL)
        return -1;

    return 0;
}

int handle_enter_notify(WindowManager *gwm, xcb_generic_event_t *event){
    xcb_enter_notify_event_t *enter_notify_event = (xcb_enter_notify_event_t *)event;

    int client_index = get_client_index(gwm, enter_notify_event->event);
    if(client_index == -1)
        return -1;

    if(gwm->config.focus_follows_mouse){
        xcb_set_input_focus(gwm->connection, XCB_INPUT_FOCUS_POINTER_ROOT, enter_notify_event->event, XCB_CURRENT_TIME);
        xcb_ewmh_set_active_window(&gwm->ewmh, 0, enter_notify_event->event);
        gwm->workspaces[gwm->current_workspace_id].focused_client_id = client_index;
    }

    xcb_flush(gwm->connection);
    return 0;
}

int handle_button_press(WindowManager *gwm, xcb_generic_event_t *event){
    xcb_button_press_event_t *button_press_event = (xcb_button_press_event_t *)event;

    Client *client = get_client(gwm, button_press_event->child);
    if(client == NULL)
        return -1;

    log_message(LOG_DEBUG, "handle_button_press");

    if(button_press_event->detail == 1){
        log_message(LOG_DEBUG, "Button 1 pressed");
        gwm->button_pressed = true;
        xcb_set_input_focus(gwm->connection, XCB_INPUT_FOCUS_POINTER_ROOT, client->window, XCB_CURRENT_TIME);
        xcb_ewmh_set_active_window(&gwm->ewmh, 0, client->window);
        xcb_ewmh_set_wm_state(&gwm->ewmh, client->window, 1, &gwm->ewmh._NET_WM_STATE_ABOVE);
    } else if(button_press_event->detail == 3){
        log_message(LOG_DEBUG, "Button 3 pressed");
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
    if(gwm->workspaces[gwm->current_workspace_id].focused_client_id != -1){
        xcb_ewmh_set_active_window(&gwm->ewmh, 0, gwm->workspaces[gwm->current_workspace_id].clients[gwm->workspaces[gwm->current_workspace_id].focused_client_id].window);
        xcb_set_input_focus(gwm->connection, XCB_INPUT_FOCUS_POINTER_ROOT, gwm->workspaces[gwm->current_workspace_id].clients[gwm->workspaces[gwm->current_workspace_id].focused_client_id].window, XCB_CURRENT_TIME);
    }

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
        resize_client(gwm, client, client->properties.width - 15, client->properties.height, true);
        establish_layout(gwm);
    } else if(is_key_pressed(gwm->key_symbols, key_event->detail, (xcb_keycode_t)XK_Right)){
        // TODO: fix bug - if there are 3 clients (1 master and 2 other), if master is resized the others stay out of the screen
        resize_client(gwm, client, client->properties.width + 15, client->properties.height, true);
        establish_layout(gwm);
    }
}

// KeyPress with MOD4 (Super)
void handle_mod4_key_press(WindowManager *gwm, xcb_key_press_event_t *key_event) {
    int client_index = get_client_index(gwm, key_event->child);
    if(client_index == -1)
        return;

    Client *client = &gwm->workspaces[gwm->current_workspace_id].clients[client_index];
    
    if(is_key_pressed(gwm->key_symbols, key_event->detail, XK_C)){ // kill client
        xcb_kill_client(gwm->connection, client->window); // TODO: needs to be implemented in a better way, xcb_kill_client might be too abruptly
    } else if(is_key_pressed(gwm->key_symbols, key_event->detail, XK_F)){ // toggle floating
        toggle_floating(gwm, client, !client->properties.is_floating);
        establish_layout(gwm);
    } else if(is_key_pressed(gwm->key_symbols, key_event->detail, XK_L)){ // reset client layout
        client->properties.manual_resize = false;
        establish_layout(gwm);
    } else if(is_key_pressed(gwm->key_symbols, key_event->detail, XK_space)){ // swap with master
        // TODO: stop using clients[0], use clients_stack[0]
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
    //apply_client_rules(gwm, client);
    establish_layout(gwm); // TODO: check error code before
    xcb_map_window(gwm->connection, client->window);

    xcb_set_input_focus(gwm->connection, XCB_INPUT_FOCUS_POINTER_ROOT, client->window, XCB_CURRENT_TIME);
    xcb_ewmh_set_active_window(&gwm->ewmh, 0, client->window);

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
                case XCB_BUTTON_RELEASE:
                    gwm->button_pressed = false;
                    break;
                case XCB_BUTTON_PRESS:
                    handle_button_press(gwm, event);
                    break;
                case XCB_FOCUS_IN:
                    // TODO: set workspace->focused_client_id the index of this client's event
                    //printf("focus in - response_type: %d, detail: %d, sequence: %d, event: %d, mode: %d\n", event->response_type, ((xcb_focus_in_event_t *)event)->detail, ((xcb_focus_in_event_t *)event)->sequence, ((xcb_focus_in_event_t *)event)->event, ((xcb_focus_in_event_t *)event)->mode);
                    break;
                case XCB_FOCUS_OUT:
                    // TODO: set workspace->last_focused_client_id the index of this client's event
                    //printf("focus out - response_type: %d, detail: %d, sequence: %d, event: %d, mode: %d\n", event->response_type, ((xcb_focus_in_event_t *)event)->detail, ((xcb_focus_in_event_t *)event)->sequence, ((xcb_focus_in_event_t *)event)->event, ((xcb_focus_in_event_t *)event)->mode);
                    break;
                case XCB_ENTER_NOTIFY:
                    //handle_enter_notify(gwm, event);
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

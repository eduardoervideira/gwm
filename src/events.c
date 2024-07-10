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

void adjust_dimensions(WindowManager *gwm, Client *client, uint16_t client_x_pos, uint16_t client_y_pos, uint16_t *width, uint16_t *height, uint16_t width_change, uint16_t height_change, uint16_t move_x, uint16_t move_y) {
    *width += width_change;
    *height += height_change;
    move_client(gwm, client, client_x_pos + move_x, client_y_pos + move_y);
}

int handle_motion_notify(WindowManager *gwm, xcb_generic_event_t *event){
    xcb_motion_notify_event_t *motion_notify_event = (xcb_motion_notify_event_t *)event;

    Client *client = get_client(gwm, motion_notify_event->event);
    if(client == NULL)
        return -1;

    if(motion_notify_event->state & XCB_BUTTON_MASK_1 && client->properties.is_floating){
        uint16_t dragdelta_x = motion_notify_event->root_x - gwm->cursor_x;
        uint16_t dragdelta_y = motion_notify_event->root_y - gwm->cursor_y;
        move_client(gwm, client, client->properties.x_pos + dragdelta_x, client->properties.y_pos + dragdelta_y);
    } else if(motion_notify_event->state & XCB_BUTTON_MASK_3 && client->properties.is_floating){
        uint16_t new_width = client->properties.width;
        uint16_t new_height = client->properties.height;

        bool top_left = (motion_notify_event->root_x >= client->properties.x_pos - 65) && 
                        (motion_notify_event->root_x <= client->properties.x_pos + 65) && 
                        (motion_notify_event->root_y >= client->properties.y_pos - 65) && 
                        (motion_notify_event->root_y <= client->properties.y_pos + 65);

        bool top_right = (motion_notify_event->root_x >= client->properties.x_pos + client->properties.width - 65) && 
                        (motion_notify_event->root_x <= client->properties.x_pos + client->properties.width + 65) && 
                        (motion_notify_event->root_y >= client->properties.y_pos - 65) && 
                        (motion_notify_event->root_y <= client->properties.y_pos + 65);

        bool bottom_right = (motion_notify_event->root_x >= client->properties.x_pos + client->properties.width - 65) && 
                        (motion_notify_event->root_x <= client->properties.x_pos + client->properties.width + 65) && 
                        (motion_notify_event->root_y >= client->properties.y_pos + client->properties.height - 65) && 
                        (motion_notify_event->root_y <= client->properties.y_pos + client->properties.height + 65);

        bool bottom_left = (motion_notify_event->root_x >= client->properties.x_pos - 65) && 
                        (motion_notify_event->root_x <= client->properties.x_pos + 65) && 
                        (motion_notify_event->root_y >= client->properties.y_pos + client->properties.height - 65) && 
                        (motion_notify_event->root_y <= client->properties.y_pos + client->properties.height + 65);

        if (top_left) {
            if (motion_notify_event->event_x < 0 && motion_notify_event->event_y < 0) {
                adjust_dimensions(gwm, client, client->properties.x_pos, client->properties.y_pos, &new_width, &new_height, abs(motion_notify_event->event_x), abs(motion_notify_event->event_y), motion_notify_event->event_x, motion_notify_event->event_y);
            } else if (motion_notify_event->event_x > 0 && motion_notify_event->event_y > 0) {
                adjust_dimensions(gwm, client, client->properties.x_pos, client->properties.y_pos, &new_width, &new_height, -abs(motion_notify_event->event_x), -abs(motion_notify_event->event_y), motion_notify_event->event_x, motion_notify_event->event_y);
            }
        } else if (top_right) {
            if (motion_notify_event->event_x > 0 && motion_notify_event->event_y < 0) {
                adjust_dimensions(gwm, client, client->properties.x_pos, client->properties.y_pos, &new_width, &new_height, motion_notify_event->event_x - new_width, abs(motion_notify_event->event_y), 0, motion_notify_event->event_y);
            } else if (motion_notify_event->event_x > 0 && motion_notify_event->event_y > 0) {
                adjust_dimensions(gwm, client, client->properties.x_pos, client->properties.y_pos, &new_width, &new_height, motion_notify_event->event_x - new_width, -abs(motion_notify_event->event_y), 0, motion_notify_event->event_y);
            }
        } else if (bottom_left) {
            if (motion_notify_event->event_x < 0 && motion_notify_event->event_y > 0) {
                adjust_dimensions(gwm, client, client->properties.x_pos, client->properties.y_pos, &new_width, &new_height, abs(motion_notify_event->event_x), motion_notify_event->event_y - new_height, motion_notify_event->event_x, 0);
            } else if (motion_notify_event->event_x > 0 && motion_notify_event->event_y > 0) {
                adjust_dimensions(gwm, client, client->properties.x_pos, client->properties.y_pos, &new_width, &new_height, -abs(motion_notify_event->event_x), motion_notify_event->event_y - new_height, motion_notify_event->event_x, 0);
            }
        } else if (bottom_right) {
            if (motion_notify_event->event_x > 0 && motion_notify_event->event_y > 0) {
                new_width = motion_notify_event->event_x;
                new_height = motion_notify_event->event_y;
            } else if (motion_notify_event->event_x < 0 && motion_notify_event->event_y > 0) {
                adjust_dimensions(gwm, client, client->properties.x_pos, client->properties.y_pos, &new_width, &new_height, abs(motion_notify_event->event_x), motion_notify_event->event_y - new_height, 0, 0);
            }
        }

        if(!top_left && !top_right && !bottom_left && !bottom_right){
             // TODO: resize growing working but shrinking is not -- it's a little bit buggy
            if(motion_notify_event->root_y > client->properties.y_pos + client->properties.height) {
                new_height = motion_notify_event->root_y - client->properties.y_pos;
            } else if(motion_notify_event->event_y < 0) {
                new_height = client->properties.height + abs(motion_notify_event->event_y);
                move_client(gwm, client, client->properties.x_pos, client->properties.y_pos - abs(motion_notify_event->event_y));
            } else if(motion_notify_event->event_x < 0){
                new_width = client->properties.width + abs(motion_notify_event->event_x);
                move_client(gwm, client, client->properties.x_pos - abs(motion_notify_event->event_x), client->properties.y_pos);
            } else if(motion_notify_event->root_x > client->properties.x_pos + client->properties.width) {
                new_width = motion_notify_event->root_x - client->properties.x_pos;
            }
        }

        if (new_width < 150) new_width = 150;
        if (new_height < 150) new_height = 150;

        resize_client(gwm, client, new_width, new_height, true);
    }
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

    Client *client = get_client(gwm, button_press_event->event);
    if(client == NULL)
        return -1;

    if(button_press_event->detail == 1){
        gwm->button_pressed = true;
        gwm->cursor_x = button_press_event->root_x;
        gwm->cursor_y = button_press_event->root_y;

        /*if(client->properties.is_floating){
            gwm->client_x = client->properties.floating_x_pos;
            gwm->client_y = client->properties.floating_y_pos;
        } else {
            gwm->client_x = client->properties.x_pos;
            gwm->client_y = client->properties.y_pos;
        }*/

        xcb_set_input_focus(gwm->connection, XCB_INPUT_FOCUS_POINTER_ROOT, client->window, XCB_CURRENT_TIME);
        xcb_ewmh_set_active_window(&gwm->ewmh, 0, client->window);
        raise_client(gwm, client);
        //xcb_configure_window(gwm->connection, client->window, XCB_CONFIG_WINDOW_STACK_MODE, (uint32_t[]){XCB_STACK_MODE_ABOVE});
        //xcb_ewmh_set_wm_state(&gwm->ewmh, client->window, 1, &gwm->ewmh._NET_WM_STATE_ABOVE);
    } else if(button_press_event->detail == 3){
        log_message(LOG_DEBUG, "Button 3 pressed");
        gwm->button_pressed = true;
        gwm->cursor_x = button_press_event->event_x;
        gwm->cursor_y = button_press_event->event_y;

        /*printf("Client X: %d, Y: %d\n", client->properties.x_pos, client->properties.y_pos);
        printf("Client Width: %d, Height: %d\n", client->properties.width, client->properties.height);
        printf("Cursor X: %d, Y: %d\n", button_press_event->root_x, button_press_event->root_y);
        printf("root_x %d, root_y %d\n", button_press_event->root_x, button_press_event->root_y);
        printf("event_x %d, event_y %d\n", button_press_event->event_x, button_press_event->event_y);
        printf("client X %d, Y %d\n", client->properties.x_pos, client->properties.y_pos);
        printf("client width %d, height %d\n", client->properties.width, client->properties.height);*/
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

    /*printf("Client window: %d\n", client->window);
    printf("Client workspace_id: %d\n", client->workspace_id);
    printf("Client x_pos: %d\n", client->properties.x_pos);
    printf("Client y_pos: %d\n", client->properties.y_pos);
    printf("Client floating_x_pos: %d\n", client->properties.floating_x_pos);
    printf("Client floating_y_pos: %d\n", client->properties.floating_y_pos);
    printf("Client width: %d\n", client->properties.width);
    printf("Client height: %d\n", client->properties.height);
    printf("Client floating_width: %d\n", client->properties.floating_width);
    printf("Client floating_height: %d\n", client->properties.floating_height);
    printf("Client is_floating: %d\n", client->properties.is_floating);*/

    //apply_client_rules(gwm, client);
    establish_layout(gwm); // TODO: check error code before

    set_client_border_width(gwm, client, 1);
    set_client_border_color(gwm, client, 0xb6f474);
    
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

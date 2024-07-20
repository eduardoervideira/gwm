#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "client.h"
#include "workspace.h"
#include "structs.h"
#include "utils.h"
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_ewmh.h>

// TODO: implement a better rules system
int apply_client_rules(WindowManager *gwm, Client *client){
    for(int i=0; i<gwm->num_rules; i++){
        if(!strcmp(gwm->rules[i].name, client->properties.class_name) || !strcmp(gwm->rules[i].name, client->properties.instance_name)){
            if(!strcmp(gwm->rules[i].key, "state")){
                if(!strcmp(gwm->rules[i].value, "floating")){
                    toggle_floating(gwm, client, true);
                } else if(!strcmp(gwm->rules[i].value, "fullscreen")){
                    printf("TODO: toggle_fullscreen\n");
                }
            }
        }
    }

    printf("Client class: %s\n", client->properties.class_name);
    printf("Client instance: %s\n", client->properties.instance_name);
    return 0;
}

Client *create_client(){
    Client *client = (Client *)calloc(1, sizeof(Client));
    if(!client){
        log_message(LOG_ERROR, "failed to allocate memory for client\n");
        return NULL;
    }

    client->workspace_id = -1;    
    client->properties.floating_x_pos = 0;
    client->properties.floating_y_pos = 0;
    client->properties.floating_height = 0;    
    client->properties.floating_width = 0;

    return client;
}

Client *get_client(WindowManager *gwm, xcb_window_t window){
    Client *client = NULL;
    for(int i=0; i<gwm->workspaces[gwm->current_workspace_id].num_clients; i++){
        if(gwm->workspaces[gwm->current_workspace_id].clients[i].window == window){
            client = &gwm->workspaces[gwm->current_workspace_id].clients[i];
            break;
        }
    }

    return client;
}

int destroy_client(WindowManager *gwm, Client *client){
    if(client == NULL){
        log_message(LOG_ERROR, "client is NULL\n");
        return -1;
    }

    xcb_unmap_window(gwm->connection, client->window);
    xcb_destroy_window(gwm->connection, client->window);
    xcb_flush(gwm->connection);

    return 0;
}

int resize_client(WindowManager *gwm, Client *client, uint16_t width, uint16_t height){
    if(client == NULL){
        log_message(LOG_ERROR, "client is NULL\n");
        return -1;
    }

    /*if((width < gwm->config.min_window_width || height < gwm->config.min_window_height) && manual_resize){
        return -1;
    }*/

    uint32_t values[2];
    values[0] = width;
    values[1] = height;
    xcb_configure_window(gwm->connection, client->window, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);

    client->properties.width = width;
    client->properties.height = height;

    if(client->properties.is_floating){
        client->properties.floating_width = width;
        client->properties.floating_height = height;
    }

    xcb_flush(gwm->connection);
    return 0;
}

int move_client(WindowManager *gwm, Client *client, int16_t x, int16_t y){ // previously uint16_t
    if(client == NULL){
        log_message(LOG_ERROR, "client is NULL\n");
        return -1;
    }

    uint32_t values[2];
    values[0] = x;
    values[1] = y;
    xcb_configure_window(gwm->connection, client->window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values);

    if(client->properties.is_floating){
        client->properties.floating_x_pos = client->properties.x_pos = x;
        client->properties.floating_y_pos = client->properties.y_pos = y;
    } else {
        client->properties.x_pos = x;
        client->properties.y_pos = y;
    }

    xcb_flush(gwm->connection);
    return 0;
}

int raise_client(WindowManager *gwm, Client *client){
    if(client == NULL){
        log_message(LOG_ERROR, "client is NULL\n");
        return -1;
    }

    uint32_t values[1];
    values[0] = XCB_STACK_MODE_ABOVE;
    xcb_configure_window(gwm->connection, client->window, XCB_CONFIG_WINDOW_STACK_MODE, values);
    xcb_ewmh_set_wm_state(&gwm->ewmh, client->window, 1, &gwm->ewmh._NET_WM_STATE_ABOVE);
    xcb_flush(gwm->connection);
    return 0;
}

int set_client_focus(WindowManager *gwm, Client *client, int client_index){
    if(client == NULL || client_index < 0 || client_index >= gwm->workspaces[gwm->current_workspace_id].num_clients){
        log_message(LOG_ERROR, "client is NULL\n");
        return -1;
    }

    if(gwm->workspaces[gwm->current_workspace_id].focused_client_id != -1){
        set_client_border_color(gwm, &gwm->workspaces[gwm->current_workspace_id].clients[gwm->workspaces[gwm->current_workspace_id].focused_client_id], 0x868686); // TODO: error code 3 sometimes
    }

    gwm->workspaces[gwm->current_workspace_id].focused_client_id = client_index;
    xcb_set_input_focus(gwm->connection, XCB_INPUT_FOCUS_POINTER_ROOT, client->window, XCB_CURRENT_TIME);
    xcb_ewmh_set_active_window(&gwm->ewmh, 0, client->window);
    set_client_border_color(gwm, client, 0xb6f474);

    xcb_flush(gwm->connection);
    return 0;
}

// TODO: refactor this function
int toggle_floating(WindowManager *gwm, Client *client, bool is_floating){
    if(client == NULL){
        log_message(LOG_ERROR, "client is NULL\n");
        return -1;
    }

    if(is_floating){
        if(client->properties.floating_width == 0 || client->properties.floating_height == 0){
            client->properties.floating_width = gwm->screen->width_in_pixels / 2;
            client->properties.floating_height = gwm->screen->height_in_pixels / 2;
            client->properties.floating_x_pos = (gwm->screen->width_in_pixels - client->properties.floating_width) / 2;
            client->properties.floating_y_pos = (gwm->screen->height_in_pixels - client->properties.floating_height) / 2;
        }
        
        client->properties.is_floating = true;
        raise_client(gwm, client);
    } else {
        client->properties.is_floating = false;
    }

    
    xcb_flush(gwm->connection);
    return 0;
}

int set_client_border_width(WindowManager *gwm, Client *client, uint16_t border_width){
    if(client == NULL){
        log_message(LOG_ERROR, "client is NULL\n");
        return -1;
    }

    uint32_t values[1];
    values[0] = border_width;
    xcb_configure_window(gwm->connection, client->window, XCB_CONFIG_WINDOW_BORDER_WIDTH, values);
    xcb_flush(gwm->connection);
    return 0;
}

int set_client_border_color(WindowManager *gwm, Client *client, uint32_t color){
    if(client == NULL){
        log_message(LOG_ERROR, "client is NULL\n");
        return -1;
    }

    uint32_t values[1];
    values[0] = color;
    //xcb_change_window_attributes(gwm->connection, client->window, XCB_CW_BORDER_PIXEL, values);
    xcb_void_cookie_t cookie = xcb_change_window_attributes_checked(gwm->connection, client->window, XCB_CW_BORDER_PIXEL, values);
    xcb_generic_error_t *error = xcb_request_check(gwm->connection, cookie);
    if(error){
        log_message(LOG_ERROR, "failed to change window attributes\n");
        printf("ERROR: %d\n", error->error_code);
        free(error);
        return -1;
    }

    xcb_flush(gwm->connection);
    return 0;
}

int get_client_index(WindowManager *gwm, xcb_window_t window){
    for(int i=0; i<gwm->workspaces[gwm->current_workspace_id].num_clients; i++){
        if(gwm->workspaces[gwm->current_workspace_id].clients[i].window == window){
            return i;
        }
    }

    return -1;
}
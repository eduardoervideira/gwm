#include <stdio.h>
#include <stdlib.h>
#include "workspace.h"
#include "structs.h"

Workspace *create_workspace(WindowManager *gwm, LayoutMode layout_mode){
    Workspace *workspace = (Workspace *)calloc(1, sizeof(Workspace));
    if(!workspace){
        printf("[fatal]: failed to allocate memory for workspace\n");
        return NULL;
    }

    workspace->clients = calloc(gwm->max_clients, sizeof(Client));
    if(!workspace->clients){
        printf("[fatal]: failed to allocate memory for clients\n");
        return NULL;
    }

    workspace->layout_clients = calloc(gwm->max_clients, sizeof(Client *));
    if(!workspace->layout_clients){
        printf("[fatal]: failed to allocate memory for clients stack\n");
        return NULL;
    }
    
    workspace->num_clients = 0;
    workspace->available_slots = gwm->max_clients;
    workspace->focused_client_id = -1;
    workspace->last_focused_client_id = -1;
    workspace->layout_mode = layout_mode;

    return workspace;
}

void realloc_workspace(WindowManager *gwm, Workspace *workspace){
    Client *new_clients = (Client *)realloc(workspace->clients, (workspace->num_clients + gwm->max_clients) * sizeof(Client));
    if(new_clients == NULL){
        printf("[fatal]: failed to reallocate memory for clients");
        return;
    }

    workspace->clients = new_clients;
    workspace->available_slots += gwm->max_clients;

    printf("[debug]: workspace available slots after update: %d\n", workspace->available_slots);

    // realoc new clients stack
    Client **new_layout_clients = (Client **)realloc(workspace->layout_clients, (workspace->num_clients + gwm->max_clients) * sizeof(Client *));
    if(new_layout_clients == NULL){
        printf("[fatal]: failed to reallocate memory for clients stack\n");
        return;
    }

    workspace->layout_clients = new_layout_clients;
}

int add_client_to_workspace(WindowManager *gwm, Workspace *workspace, Client *client){
    if(gwm == NULL || workspace == NULL || client == NULL)
        return -1;

    if(workspace->available_slots == 0){
        printf("[info]: max number of clients reached, reallocating...\n");
        realloc_workspace(gwm, workspace);
    }

    if(workspace->layout_mode == FLOATING_MODE){
        client->properties.width = client->properties.floating_width = gwm->screen->width_in_pixels / 2;
        client->properties.height = client->properties.floating_height =  gwm->screen->height_in_pixels / 2;
        client->properties.x_pos = client->properties.floating_x_pos = (gwm->screen->width_in_pixels - client->properties.width) / 2;
        client->properties.y_pos = client->properties.floating_y_pos = (gwm->screen->height_in_pixels - client->properties.height) / 2;
        client->properties.is_floating = true;
    }

    workspace->clients[workspace->num_clients] = *client;
    workspace->num_clients++;
    workspace->available_slots--;

    client->workspace_id = gwm->current_workspace_id;

    // TODO: it might not be necessary all these masks -- needs more research
    uint32_t event_mask =   XCB_EVENT_MASK_STRUCTURE_NOTIFY |
                            XCB_EVENT_MASK_PROPERTY_CHANGE |
                            XCB_EVENT_MASK_FOCUS_CHANGE |
                            XCB_EVENT_MASK_POINTER_MOTION |
                            XCB_EVENT_MASK_ENTER_WINDOW |
                            XCB_EVENT_MASK_KEY_PRESS | 
                            XCB_EVENT_MASK_KEY_RELEASE;

    gwm->cookie = xcb_change_window_attributes_checked(gwm->connection, client->window, XCB_CW_EVENT_MASK, &event_mask);
    gwm->error = xcb_request_check(gwm->connection, gwm->cookie);
    if(gwm->error){
        printf("[error]: failed to change window attributes. error code: %d", gwm->error->error_code);
        free(gwm->error);
    }

    uint32_t button_mask = XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_BUTTON_MOTION;
    gwm->cookie = xcb_grab_button_checked(gwm->connection, 0, client->window, button_mask, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, gwm->screen->root, XCB_NONE, 1, XCB_NONE); // XCB_MOD_MASK_1
    gwm->error = xcb_request_check(gwm->connection, gwm->cookie);
    if(gwm->error){
        printf("[error]: failed to grab button. error code: %d\n", gwm->error->error_code);
        free(gwm->error);
    }
    
    gwm->cookie = xcb_grab_button_checked(gwm->connection, 0, client->window, button_mask, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, gwm->screen->root, XCB_NONE, 3, XCB_NONE); 
    gwm->error = xcb_request_check(gwm->connection, gwm->cookie);
    if(gwm->error){
        printf("[error]: failed to grab button. error code: %d\n", gwm->error->error_code);
        free(gwm->error);
    }
}
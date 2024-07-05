#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "client.h"
#include "workspace.h"
#include "structs.h"
#include "layout.h"
#include "utils.h"

int cycle_through_clients(WindowManager *gwm, Workspace *workspace){
    if(gwm == NULL || workspace == NULL){
        log_message(LOG_ERROR, "gwm or workspace is NULL");
        return -1;
    }

    if(workspace->num_clients == 0 || workspace->num_clients == 1){
        log_message(LOG_INFO, "no clients to cycle through");
        return -1;
    }

    if(workspace->focused_client_id == -1){
        log_message(LOG_INFO, "no focused client set");
        return -1;
    }

    workspace->last_focused_client_id = workspace->focused_client_id;

    if(workspace->focused_client_id == workspace->num_clients - 1){
        workspace->focused_client_id = 0;
    } else {
        workspace->focused_client_id++;
    }
    
    if(workspace->layout_mode == MONOCLE_MODE){
        map_clients(gwm, workspace);
    }

    //set_client_focused(gwm, workspace, workspace->focused_client_id);
    //set_client_unfocused(gwm, workspace, workspace->last_focused_client_id);
    xcb_flush(gwm->connection);
    return 0;
}

int cycle_through_workspaces(WindowManager *gwm){
    if(gwm == NULL || gwm->workspaces == NULL)
        return -1;

    int old_workspace_id = gwm->current_workspace_id;
    unmap_clients(gwm, &gwm->workspaces[gwm->current_workspace_id]);

    if(gwm->current_workspace_id == gwm->config.workspace_count - 1){
        gwm->current_workspace_id = 0;
    } else {
        gwm->current_workspace_id++;
    }

    log_message(LOG_DEBUG, "current workspace: %d -> changing workspace to %d", old_workspace_id, gwm->current_workspace_id);
    establish_layout(gwm);
    map_clients(gwm, &gwm->workspaces[gwm->current_workspace_id]);
    xcb_ewmh_set_current_desktop(&gwm->ewmh, 0, gwm->current_workspace_id);

    return 0;
}

Workspace *create_workspace(WindowManager *gwm, LayoutMode layout_mode){
    Workspace *workspace = (Workspace *)calloc(1, sizeof(Workspace));
    if(!workspace){
        log_message(LOG_FATAL, "failed to allocate memory for workspace");
        return NULL;
    }

    workspace->clients = calloc(gwm->config.max_clients, sizeof(Client));
    if(!workspace->clients){
        log_message(LOG_FATAL, "failed to allocate memory for clients");
        return NULL;
    }

    workspace->clients_stack = calloc(gwm->config.max_clients, sizeof(Client *));
    if(!workspace->clients_stack){
        log_message(LOG_FATAL, "failed to allocate memory for clients stack");
        return NULL;
    }
    
    workspace->num_clients = 0;
    workspace->available_slots = gwm->config.max_clients;
    workspace->focused_client_id = -1;
    workspace->layout_mode = layout_mode;

    return workspace;
}

void realloc_workspace(WindowManager *gwm, Workspace *workspace){
    Client *new_clients = (Client *)realloc(workspace->clients, (workspace->num_clients + gwm->config.max_clients) * sizeof(Client));
    if(new_clients == NULL){
        log_message(LOG_FATAL, "failed to reallocate memory for clients");
        return;
    }

    workspace->clients = new_clients;
    workspace->available_slots += gwm->config.max_clients;

    log_message(LOG_DEBUG, "workspace available slots after update: %d", workspace->available_slots);

    // realoc new clients stack
    Client **new_clients_stack = (Client **)realloc(workspace->clients_stack, (workspace->num_clients + gwm->config.max_clients) * sizeof(Client *));
    if(new_clients_stack == NULL){
        log_message(LOG_FATAL, "failed to reallocate memory for clients stack");
        return;
    }

    workspace->clients_stack = new_clients_stack;
}

int add_client_to_workspace(WindowManager *gwm, Workspace *workspace, Client *client){
    if(gwm == NULL || workspace == NULL || client == NULL)
        return -1;

    if(workspace->available_slots == 0){
        log_message(LOG_INFO, "max number of clients reached, reallocating...");
        realloc_workspace(gwm, workspace);
    }

    workspace->clients[workspace->num_clients] = *client;
    workspace->num_clients++;
    workspace->available_slots--;

    client->workspace_id = gwm->current_workspace_id;
    workspace->last_focused_client_id = workspace->focused_client_id;
    workspace->focused_client_id = workspace->num_clients - 1;

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
        log_message(LOG_ERROR, "failed to change window attributes. error code: %d", gwm->error->error_code);
        free(gwm->error);
    }

    xcb_keycode_t *keycode_c = xcb_key_symbols_get_keycode(gwm->key_symbols, XK_C);
    xcb_keycode_t *keycode_f = xcb_key_symbols_get_keycode(gwm->key_symbols, XK_F);
    xcb_keycode_t *keycode_l = xcb_key_symbols_get_keycode(gwm->key_symbols, XK_L);
    xcb_grab_key(gwm->connection, 1, gwm->screen->root, XCB_MOD_MASK_4, *keycode_c, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    xcb_grab_key(gwm->connection, 1, gwm->screen->root, XCB_MOD_MASK_4, *keycode_f, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    xcb_grab_key(gwm->connection, 1, gwm->screen->root, XCB_MOD_MASK_4, *keycode_l, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

    xcb_keycode_t *keycode_shift = xcb_key_symbols_get_keycode(gwm->key_symbols, XK_Shift_L);
    xcb_keycode_t *keycode_up = xcb_key_symbols_get_keycode(gwm->key_symbols, XK_Up);
    xcb_keycode_t *keycode_down = xcb_key_symbols_get_keycode(gwm->key_symbols, XK_Down);
    xcb_keycode_t *keycode_left = xcb_key_symbols_get_keycode(gwm->key_symbols, XK_Left);
    xcb_keycode_t *keycode_right = xcb_key_symbols_get_keycode(gwm->key_symbols, XK_Right);
    xcb_keycode_t *keycode_space = xcb_key_symbols_get_keycode(gwm->key_symbols, XK_space);
    xcb_grab_key(gwm->connection, 1, gwm->screen->root, XCB_MOD_MASK_4 | XCB_MOD_MASK_SHIFT, *keycode_up, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    xcb_grab_key(gwm->connection, 1, gwm->screen->root, XCB_MOD_MASK_4 | XCB_MOD_MASK_SHIFT, *keycode_down, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    xcb_grab_key(gwm->connection, 1, gwm->screen->root, XCB_MOD_MASK_4 | XCB_MOD_MASK_SHIFT, *keycode_left, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    xcb_grab_key(gwm->connection, 1, gwm->screen->root, XCB_MOD_MASK_4 | XCB_MOD_MASK_SHIFT, *keycode_right, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

    xcb_grab_key(gwm->connection, 1, gwm->screen->root, XCB_MOD_MASK_4, *keycode_up, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    xcb_grab_key(gwm->connection, 1, gwm->screen->root, XCB_MOD_MASK_4, *keycode_down, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    xcb_grab_key(gwm->connection, 1, gwm->screen->root, XCB_MOD_MASK_4, *keycode_left, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    xcb_grab_key(gwm->connection, 1, gwm->screen->root, XCB_MOD_MASK_4, *keycode_right, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    xcb_grab_key(gwm->connection, 1, gwm->screen->root, XCB_MOD_MASK_4, *keycode_space, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

    free(keycode_c);
    free(keycode_f);
    free(keycode_l);
    free(keycode_shift);
    free(keycode_up);
    free(keycode_down);
    free(keycode_left);
    free(keycode_right);
    free(keycode_space);
    return 0;
}

void remove_client_from_workspace(Workspace *workspace, Client *client){
    if(workspace->num_clients == 0){
        log_message(LOG_INFO, "no clients to remove");
        return;
    }

    if(client == NULL){
        log_message(LOG_ERROR, "client is NULL");
        return;
    }

    for(int i=0; i<workspace->num_clients; i++){
        if(workspace->clients[i].window == client->window){
            for(int j=i; j<workspace->num_clients-1; j++){
                workspace->clients[j] = workspace->clients[j+1];
            }

            break;
        }
    }

    workspace->num_clients--;
    workspace->num_clients_in_clients_stack--;
    workspace->available_slots++;
    workspace->last_focused_client_id = workspace->focused_client_id;
    workspace->focused_client_id = workspace->num_clients - 1;
}

void swap_clients(Workspace *workspace, Client *client1, Client *client2){
    for(int i=0; i<workspace->num_clients; i++){
        if(workspace->clients[i].window == client1->window){
            workspace->clients[i] = *client1;
        } else if(workspace->clients[i].window == client2->window){
            workspace->clients[i] = *client2;
        }
    }
}

void map_clients(WindowManager *gwm, Workspace *workspace){
    if(gwm == NULL || workspace == NULL)
        return;
    
    if(workspace->num_clients == 0)
        return;

    if(workspace->layout_mode == MONOCLE_MODE){
        if(workspace->num_clients >= 2){
            xcb_map_window(gwm->connection, workspace->clients[workspace->focused_client_id].window);
            workspace->clients[workspace->focused_client_id].is_mapped = 1;

            xcb_unmap_window(gwm->connection, workspace->clients[workspace->last_focused_client_id].window);
            workspace->clients[workspace->last_focused_client_id].is_mapped = 0;
        } else {
            xcb_map_window(gwm->connection, workspace->clients[0].window);
            workspace->clients[0].is_mapped = 1;
        }
    } else {
        for(int i = 0; i < workspace->num_clients; i++){
            if(!workspace->clients[i].is_mapped){
                xcb_map_window(gwm->connection, workspace->clients[i].window);
                workspace->clients[i].is_mapped = 1;
            }
        }
    }

    xcb_flush(gwm->connection);
}

void unmap_clients(WindowManager *gwm, Workspace *workspace){
    for(int i = 0; i < workspace->num_clients; i++){
        if(workspace->clients[i].is_mapped){
            xcb_unmap_window(gwm->connection, workspace->clients[i].window);
            workspace->clients[i].is_mapped = 0;
        }
    }

    xcb_flush(gwm->connection);
}

int change_workspace_layout(WindowManager *gwm, Workspace *workspace, LayoutMode layout_mode){
    if(gwm == NULL || workspace == NULL)
        return -1;

    if(workspace->num_clients == 0)
        return -1;

    workspace->layout_mode = layout_mode;
    establish_layout(gwm);

    return 0;
}
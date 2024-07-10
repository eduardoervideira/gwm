#include <stdio.h>
#include <stdlib.h>
#include "structs.h"
#include "layout.h"
#include "config.h"
#include "client.h"
#include "events.h"
#include "utils.h"

// todo: eventualmente passar a ser maximize em vez de fullsize mas ok
int establish_master_client_fullsize(WindowManager *gwm, Workspace *workspace){
    // int master_client_width = gwm->screen->width_in_pixels - (2 * gwm->config.clients_gap);
    // int master_client_height = gwm->screen->height_in_pixels - (2 * gwm->config.clients_gap);

    // get first client from layout_clients
    Client *master_client = workspace->layout_clients[0];
    if(master_client == NULL){
        log_message(LOG_WARNING, "master client is FLOATING so it will not be resized\n");
        return -1;
    }

    int master_client_width = 0, master_client_height = 0, master_client_x_pos = 0, master_client_y_pos = 0;
    master_client_width = master_client->properties.manual_resize ? master_client->properties.width : gwm->screen->width_in_pixels - (2 * gwm->config.clients_gap) - (gwm->config.margin_left) - (gwm->config.margin_right);
    master_client_height = master_client->properties.manual_resize ? master_client->properties.height : gwm->screen->height_in_pixels - (2 * gwm->config.clients_gap) - (gwm->config.margin_top) - (gwm->config.margin_bottom);
    master_client_x_pos = gwm->config.margin_left + gwm->config.clients_gap;
    master_client_y_pos = gwm->config.margin_top + gwm->config.clients_gap;
    
    // TODO: adicionar um if para verificar se a propriedade FORCE_RESIZE está ativa -> se sim, força o resize
    // ideia: no unmap_notify passar uma flag para o establish_layout
    
    move_client(gwm, master_client, master_client_x_pos, master_client_y_pos);
    resize_client(gwm, master_client, master_client_width, master_client_height, 0);
    
    return 0;
}

int establish_monocle_mode(WindowManager *gwm, Workspace *workspace){
    for(int i=0; i<workspace->num_clients; i++){
        move_client(gwm, &workspace->clients[i], gwm->config.clients_gap, gwm->config.clients_gap);
        resize_client(gwm, &workspace->clients[i], gwm->screen->width_in_pixels - (2 * gwm->config.clients_gap), gwm->screen->height_in_pixels - (2 * gwm->config.clients_gap), 0);
    }
    return 0;
}

// TODO: fix margins (ter em conta a border_width) -> ainda não sei fazer
int establish_stacking_mode(WindowManager *gwm, Workspace *workspace){
    int num_clients = workspace->num_clients_in_layout_clients;
    if(num_clients <= 0)
        return -1;

    int master_client_width = 0, master_client_height = 0, master_client_x_pos = 0, master_client_y_pos = 0;
    if(num_clients == 1){
        establish_master_client_fullsize(gwm, workspace);
    } else {
        int slave_client_width, slave_client_height, slave_client_x_pos, slave_client_y_pos;
        if(gwm->config.stacking_mode_master_window_orientation == HORIZONTAL){
            int total_height = gwm->screen->height_in_pixels;
            master_client_width = gwm->screen->width_in_pixels - (2 * gwm->config.clients_gap);
            master_client_height = workspace->layout_clients[0]->properties.manual_resize ? workspace->layout_clients[0]->properties.height : total_height * gwm->config.stacking_mode_master_window_ratio;
            slave_client_height = workspace->layout_clients[1]->properties.manual_resize ? workspace->layout_clients[1]->properties.height : total_height - master_client_height - (3 * gwm->config.clients_gap);
            slave_client_width = gwm->screen->width_in_pixels - (2 * gwm->config.clients_gap);

            master_client_x_pos = gwm->config.clients_gap;
            master_client_y_pos = gwm->config.stacking_mode_master_window_position == TOP ? gwm->config.clients_gap : slave_client_height + (2 * gwm->config.clients_gap);

            slave_client_x_pos = gwm->config.clients_gap;
            slave_client_y_pos = gwm->config.stacking_mode_master_window_position == TOP ? master_client_height + (2 * gwm->config.clients_gap) : gwm->config.clients_gap;
        } else {
            int total_width = gwm->screen->width_in_pixels;
            master_client_width = workspace->layout_clients[0]->properties.manual_resize ? workspace->layout_clients[0]->properties.width : total_width * gwm->config.stacking_mode_master_window_ratio;
            master_client_height = gwm->screen->height_in_pixels - (2 * gwm->config.clients_gap) - (gwm->config.margin_top);
            slave_client_height = gwm->screen->height_in_pixels - (2 * gwm->config.clients_gap) - (gwm->config.margin_top);
            slave_client_width = workspace->layout_clients[1]->properties.manual_resize ? workspace->layout_clients[1]->properties.width : total_width - master_client_width - (3 * gwm->config.clients_gap);
            
            master_client_x_pos = gwm->config.stacking_mode_master_window_position == LEFT ? gwm->config.clients_gap : slave_client_width + (2 * gwm->config.clients_gap);
            master_client_y_pos = gwm->config.margin_top + gwm->config.clients_gap;

            slave_client_x_pos = gwm->config.stacking_mode_master_window_position == LEFT ? master_client_width + (2 * gwm->config.clients_gap) : gwm->config.clients_gap;
            slave_client_y_pos = gwm->config.margin_top + gwm->config.clients_gap;
        }

        move_client(gwm, workspace->layout_clients[0], master_client_x_pos, master_client_y_pos);
        resize_client(gwm, workspace->layout_clients[0], master_client_width, master_client_height, 0);

        if(gwm->config.stacking_mode_master_window_orientation == HORIZONTAL){
            int total_width = (gwm->screen->width_in_pixels - (num_clients * gwm->config.clients_gap)) / (num_clients - 1);
            int current_x = gwm->config.clients_gap;
            for(int i = 1; i < num_clients; i++){
                move_client(gwm, workspace->layout_clients[i], current_x, slave_client_y_pos);
                resize_client(gwm, workspace->layout_clients[i], total_width, slave_client_height, 0);
                current_x += total_width + gwm->config.clients_gap;
            }
        } else {
            int total_height = (gwm->screen->height_in_pixels - (num_clients * gwm->config.clients_gap)) / (num_clients - 1);
            int current_y = gwm->config.clients_gap;
            for(int i = 1; i < num_clients; i++){
                int client_width = workspace->layout_clients[i]->properties.manual_resize ? workspace->layout_clients[i]->properties.width : gwm->screen->width_in_pixels - master_client_width - (3 * gwm->config.clients_gap);
                move_client(gwm, workspace->layout_clients[i], slave_client_x_pos, current_y);
                resize_client(gwm, workspace->layout_clients[i], client_width, total_height, 0);
                current_y += total_height + gwm->config.clients_gap;
            }
        }
    }

    return 0;
}

int establish_layout(WindowManager *gwm){
    // TODO: eventualmente irei passar parametro para esta função para poder estabelecer o layout de uma workspace específica
    Workspace *workspace = &gwm->workspaces[gwm->current_workspace_id];
    if(gwm == NULL || workspace == NULL)
        return -1;

    if(workspace->num_clients == 0)
        return -1;

    if(workspace->layout_mode == FLOATING_MODE){
        for(int i=0; i<workspace->num_clients; i++){
            move_client(gwm, &workspace->clients[i], workspace->clients[i].properties.floating_x_pos, workspace->clients[i].properties.floating_y_pos);
            resize_client(gwm, &workspace->clients[i], workspace->clients[i].properties.floating_width, workspace->clients[i].properties.floating_height, 0);
        }
        return 0;
    }

    workspace->num_clients_in_layout_clients = 0;
    for(int i=0; i<workspace->num_clients; i++){
        if(!workspace->clients[i].properties.is_floating){
            workspace->layout_clients[workspace->num_clients_in_layout_clients] = &workspace->clients[i];
            workspace->num_clients_in_layout_clients++;
        }
    }

    switch(workspace->layout_mode){
        case MONOCLE_MODE:
            establish_monocle_mode(gwm, workspace);
            break;
        case STACKING_MODE:
            establish_stacking_mode(gwm, workspace);
            break;
        default:
            log_message(LOG_WARNING, "establish_layout: layout mode not supported\n");
            break;
    }

    // also need to handle floating clients so they are not affected by the layout
    for(int i=0; i<workspace->num_clients; i++){
        if(workspace->clients[i].properties.is_floating){
            move_client(gwm, &workspace->clients[i], workspace->clients[i].properties.floating_x_pos, workspace->clients[i].properties.floating_y_pos);
            resize_client(gwm, &workspace->clients[i], workspace->clients[i].properties.floating_width, workspace->clients[i].properties.floating_height, 0);
        }
    }

    for(int i=0; i<workspace->num_clients_in_layout_clients; i++){
        workspace->layout_clients[i] = NULL;
    }
    return 0;
}
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

    // get first client from clients_stack
    Client *master_client = workspace->clients_stack[0];
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

int establish_n_rows_mode(WindowManager *gwm, Workspace *workspace, uint8_t num_rows){
    // TODO: remover os comentarios
    int num_clients = workspace->num_clients_in_clients_stack;

    if(num_clients == 1){
        establish_master_client_fullsize(gwm, workspace);
    } else {
        int total_height = gwm->screen->height_in_pixels - (2 * gwm->config.clients_gap);
        int rows = num_rows;  // Defina o número de linhas desejado
        int client_height = (total_height - (rows - 1) * gwm->config.clients_gap) / rows;  // Inclui espaço entre os clientes
        int total_width = gwm->screen->width_in_pixels - (2 * gwm->config.clients_gap);
        int columns = (num_clients + rows - 1) / rows;  // Calcula o número de colunas
        int client_width = (total_width - (columns - 1) * gwm->config.clients_gap) / columns;  // Inclui espaço entre os clientes
        int x = gwm->config.clients_gap;  // Inicialize x com gwm->config.clients_gap

        for(int i = 0; i < num_clients; i++){
            int row = i / columns;  // Calcula a linha atual
            int column = i % columns;  // Calcula a coluna atual
            int y = gwm->config.clients_gap + row * (client_height + gwm->config.clients_gap);
            int client_x = x + column * (client_width + gwm->config.clients_gap);
            int client_y = y;  // Declare client_y aqui

            if(num_clients % 2 == 1 && i == num_clients - 1 && num_rows != 1){  // Verifica se é a última janela e o número de janelas é ímpar
                client_width *= 2;
                client_width += gwm->config.clients_gap;
            }
            
            move_client(gwm, gwm->workspaces[gwm->current_workspace_id].clients_stack[i], client_x, client_y);
            resize_client(gwm, gwm->workspaces[gwm->current_workspace_id].clients_stack[i], client_width, client_height, 0);
        }
    }
    
    return 0;
}

// TODO: rever o codigo e otimizar tanto do n_columns como do n_rows
int establish_n_columns_mode(WindowManager *gwm, Workspace *workspace, uint8_t num_columns){
    // TODO: seja o layout grid, column, row, stacking - o primeiro cliente é sempre o master ou seja 100% da width - 2*gwm->config.clients_gap
    int num_clients = workspace->num_clients;

    if(num_clients == 1){
        establish_master_client_fullsize(gwm, workspace);
    } else {
        int total_width = gwm->screen->width_in_pixels - (2 * gwm->config.clients_gap);
        int columns = num_columns;  // Defina o número de colunas desejado
        int client_width = (total_width - (columns - 1) * gwm->config.clients_gap) / columns;  // Inclui espaço entre os clientes
        int total_height = gwm->screen->height_in_pixels - (2 * gwm->config.clients_gap);
        int rows = (num_clients + columns - 1) / columns;  // Calcula o número de linhas
        int client_height = (total_height - (rows - 1) * gwm->config.clients_gap) / rows;  // Inclui espaço entre os clientes
        int y = gwm->config.clients_gap;  // Inicialize y com gwm->config.clients_gap

        for(int i = 0; i < num_clients; i++){
            int column = i % columns;  // Calcula a coluna atual
            int row = i / columns;  // Calcula a linha atual
            int x = gwm->config.clients_gap + column * (client_width + gwm->config.clients_gap);
            int client_x = x;
            int client_y = y + row * (client_height + gwm->config.clients_gap);

            if(num_clients % 2 == 1 && i == num_clients - 1 && num_columns != 1){
                client_width = gwm->screen->width_in_pixels - (2 * gwm->config.clients_gap);
            }
            
            move_client(gwm, gwm->workspaces[gwm->current_workspace_id].clients_stack[i], client_x, client_y);
            resize_client(gwm, gwm->workspaces[gwm->current_workspace_id].clients_stack[i], client_width, client_height, 0);
        }
    }

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
    int num_clients = workspace->num_clients_in_clients_stack;
    if(num_clients == 0)
        return -1;

    int master_client_width = 0, master_client_height = 0, master_client_x_pos = 0, master_client_y_pos = 0;
    if(num_clients == 1){
        establish_master_client_fullsize(gwm, workspace);
    } else if(num_clients == 2){
        int slave_client_width = 0, slave_client_height = 0, slave_client_x_pos = 0, slave_client_y_pos = 0;
        // TODO: remover código repetido daqui...
        if(gwm->config.stacking_mode_master_window_orientation == HORIZONTAL){
            int total_height = gwm->screen->height_in_pixels;
            master_client_width = gwm->screen->width_in_pixels - (2 * gwm->config.clients_gap);
            master_client_height = workspace->clients_stack[0]->properties.manual_resize ? workspace->clients_stack[0]->properties.height : total_height * gwm->config.stacking_mode_master_window_ratio;
            slave_client_height = workspace->clients_stack[1]->properties.manual_resize ? workspace->clients_stack[1]->properties.height : total_height - master_client_height - (3 * gwm->config.clients_gap);
            slave_client_width = gwm->screen->width_in_pixels - (2 * gwm->config.clients_gap);

            master_client_x_pos = gwm->config.clients_gap;
            master_client_y_pos = gwm->config.stacking_mode_master_window_position == TOP ? gwm->config.clients_gap : slave_client_height + (2 * gwm->config.clients_gap);

            slave_client_x_pos = gwm->config.clients_gap;
            slave_client_y_pos = gwm->config.stacking_mode_master_window_position == TOP ? master_client_height + (2 * gwm->config.clients_gap) : gwm->config.clients_gap;
        } else {
            int total_width = gwm->screen->width_in_pixels;
            master_client_width = workspace->clients_stack[0]->properties.manual_resize ? workspace->clients_stack[0]->properties.width : total_width * gwm->config.stacking_mode_master_window_ratio;
            master_client_height = gwm->screen->height_in_pixels - (2 * gwm->config.clients_gap) - (gwm->config.margin_top);
            slave_client_height = gwm->screen->height_in_pixels - (2 * gwm->config.clients_gap) - (gwm->config.margin_top);
            slave_client_width = workspace->clients_stack[1]->properties.manual_resize ? workspace->clients_stack[1]->properties.width : total_width - master_client_width - (3 * gwm->config.clients_gap);
            
            master_client_x_pos = gwm->config.stacking_mode_master_window_position == LEFT ? gwm->config.clients_gap : slave_client_width + (2 * gwm->config.clients_gap);
            master_client_y_pos = gwm->config.margin_top + gwm->config.clients_gap;

            slave_client_x_pos = gwm->config.stacking_mode_master_window_position == LEFT ? master_client_width + (2 * gwm->config.clients_gap) : gwm->config.clients_gap;
            slave_client_y_pos = gwm->config.margin_top + gwm->config.clients_gap;
        }

        move_client(gwm, workspace->clients_stack[0], master_client_x_pos, master_client_y_pos);
        resize_client(gwm, workspace->clients_stack[0], master_client_width, master_client_height, 0);
        
        move_client(gwm, workspace->clients_stack[1], slave_client_x_pos, slave_client_y_pos);
        resize_client(gwm, workspace->clients_stack[1], slave_client_width, slave_client_height, 0);
    } else {
        int slave_client_width, slave_client_height, slave_client_x_pos, slave_client_y_pos;
        if(gwm->config.stacking_mode_master_window_orientation == HORIZONTAL){
            int total_height = gwm->screen->height_in_pixels;
            master_client_width = gwm->screen->width_in_pixels - (2 * gwm->config.clients_gap);
            master_client_height = workspace->clients_stack[0]->properties.manual_resize ? workspace->clients_stack[0]->properties.height : total_height * gwm->config.stacking_mode_master_window_ratio;
            slave_client_height = workspace->clients_stack[1]->properties.manual_resize ? workspace->clients_stack[1]->properties.height : total_height - master_client_height - (3 * gwm->config.clients_gap);
            slave_client_width = gwm->screen->width_in_pixels - (2 * gwm->config.clients_gap);

            master_client_x_pos = gwm->config.clients_gap;
            master_client_y_pos = gwm->config.stacking_mode_master_window_position == TOP ? gwm->config.clients_gap : slave_client_height + (2 * gwm->config.clients_gap);

            slave_client_x_pos = gwm->config.clients_gap;
            slave_client_y_pos = gwm->config.stacking_mode_master_window_position == TOP ? master_client_height + (2 * gwm->config.clients_gap) : gwm->config.clients_gap;
        } else {
            int total_width = gwm->screen->width_in_pixels;
            master_client_width = workspace->clients_stack[0]->properties.manual_resize ? workspace->clients_stack[0]->properties.width : total_width * gwm->config.stacking_mode_master_window_ratio;
            master_client_height = gwm->screen->height_in_pixels - (2 * gwm->config.clients_gap) - (gwm->config.margin_top);
            slave_client_height = gwm->screen->height_in_pixels - (2 * gwm->config.clients_gap) - (gwm->config.margin_top);
            slave_client_width = workspace->clients_stack[1]->properties.manual_resize ? workspace->clients_stack[1]->properties.width : total_width - master_client_width - (3 * gwm->config.clients_gap);
            
            master_client_x_pos = gwm->config.stacking_mode_master_window_position == LEFT ? gwm->config.clients_gap : slave_client_width + (2 * gwm->config.clients_gap);
            master_client_y_pos = gwm->config.margin_top + gwm->config.clients_gap;

            slave_client_x_pos = gwm->config.stacking_mode_master_window_position == LEFT ? master_client_width + (2 * gwm->config.clients_gap) : gwm->config.clients_gap;
            slave_client_y_pos = gwm->config.margin_top + gwm->config.clients_gap;
        }

        move_client(gwm, workspace->clients_stack[0], master_client_x_pos, master_client_y_pos);
        resize_client(gwm, workspace->clients_stack[0], master_client_width, master_client_height, 0);

        if(gwm->config.stacking_mode_master_window_orientation == HORIZONTAL){
            int total_width = (gwm->screen->width_in_pixels - (num_clients * gwm->config.clients_gap)) / (num_clients - 1);
            int current_x = gwm->config.clients_gap;
            for(int i = 1; i < num_clients; i++){
                move_client(gwm, workspace->clients_stack[i], current_x, slave_client_y_pos);
                resize_client(gwm, workspace->clients_stack[i], total_width, slave_client_height, 0);
                current_x += total_width + gwm->config.clients_gap;
            }
        } else {
            int total_height = (gwm->screen->height_in_pixels - (num_clients * gwm->config.clients_gap)) / (num_clients - 1);
            int current_y = gwm->config.clients_gap;
            for(int i = 1; i < num_clients; i++){
                int client_width = workspace->clients_stack[i]->properties.manual_resize ? workspace->clients_stack[i]->properties.width : gwm->screen->width_in_pixels - master_client_width - (3 * gwm->config.clients_gap);
                move_client(gwm, workspace->clients_stack[i], slave_client_x_pos, current_y);
                resize_client(gwm, workspace->clients_stack[i], client_width, total_height, 0);
                current_y += total_height + gwm->config.clients_gap;
            }
        }
    }

    return 0;
}

int establish_scrolling_mode(WindowManager *gwm, Workspace *workspace){
    int num_clients = workspace->num_clients;

    if(num_clients == 1){
        establish_master_client_fullsize(gwm, workspace);
    } else {
        int width = gwm->screen->width_in_pixels * gwm->config.stacking_mode_master_window_ratio;
        int height = gwm->screen->height_in_pixels - (2 * gwm->config.clients_gap);
        
        int x = gwm->config.clients_gap;
        int y = gwm->config.clients_gap;

        for(int i=0; i<num_clients; i++){
            move_client(gwm, &workspace->clients[i], x, y);
            resize_client(gwm, &workspace->clients[i], width, height, 0);
            x += width + gwm->config.clients_gap;
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
            if(workspace->clients[i].properties.is_floating){
                move_client(gwm, &workspace->clients[i], workspace->clients[i].properties.floating_x_pos, workspace->clients[i].properties.floating_y_pos);
                resize_client(gwm, &workspace->clients[i], workspace->clients[i].properties.floating_width, workspace->clients[i].properties.floating_height, 0);
                xcb_configure_window(gwm->connection, workspace->clients[i].window, XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE, (uint32_t[]){XCB_NONE, XCB_STACK_MODE_ABOVE});
                xcb_ewmh_set_wm_state(&gwm->ewmh, workspace->clients[i].window, 1, &gwm->ewmh._NET_WM_STATE_ABOVE);
            }
        }

        return 0;
    }

    workspace->num_clients_in_clients_stack = 0;
    for(int i=0; i<workspace->num_clients; i++){
        if(!workspace->clients[i].properties.is_floating){
            workspace->clients_stack[workspace->num_clients_in_clients_stack] = &workspace->clients[i];
            workspace->num_clients_in_clients_stack++;
        }
    }

    switch(workspace->layout_mode){
        case MONOCLE_MODE:
            establish_monocle_mode(gwm, workspace);
            break;
        case STACKING_MODE:
            establish_stacking_mode(gwm, workspace);
            // establish_scrolling_mode(gwm, workspace); TODO: nao esta pronto
            break;
        case COLUMNS_MODE:
            establish_n_rows_mode(gwm, workspace, 1); // Columns Mode é igual a 1 Row Mode
            break;
        case ROWS_MODE:
            establish_n_columns_mode(gwm, workspace, 1); // Rows Mode é igual a 1 Column Mode
            break;
        case N_COLUMNS_MODE:
            establish_n_columns_mode(gwm, workspace, gwm->config.n_columns_mode_number_of_columns);
            break;
        case N_ROWS_MODE:
            establish_n_rows_mode(gwm, workspace, gwm->config.n_rows_mode_number_of_rows);
            break;
        default:
            log_message(LOG_WARNING, "establish_layout: layout mode not supported\n");
            break;
    }

    for(int i=0; i<workspace->num_clients; i++){
        if(workspace->clients[i].properties.is_floating){
            move_client(gwm, &workspace->clients[i], workspace->clients[i].properties.floating_x_pos, workspace->clients[i].properties.floating_y_pos);
            resize_client(gwm, &workspace->clients[i], workspace->clients[i].properties.floating_width, workspace->clients[i].properties.floating_height, 0);
            //xcb_configure_window(gwm->connection, workspace->clients[i].window, XCB_CONFIG_WINDOW_STACK_MODE, (uint32_t[]){XCB_STACK_MODE_ABOVE});
            xcb_ewmh_set_wm_state(&gwm->ewmh, workspace->clients[i].window, 1, &gwm->ewmh._NET_WM_STATE_ABOVE);
        }
    }

    for(int i=0; i<workspace->num_clients_in_clients_stack; i++){
        workspace->clients_stack[i] = NULL;
    }
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>
#include <xcb/xcb_util.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_keysyms.h>
#include <X11/keysym.h>
#include <xcb/randr.h>
#include <unistd.h> 
#include <string.h>
#include "utils.h"
#include "config.h"
#include "workspace.h"
#include "events.h"
#include "structs.h"
#include "window_manager.h"

WindowManager *setup(){
    WindowManager *gwm = (WindowManager *)calloc(1, sizeof(WindowManager));

    if(!gwm){
        log_message(LOG_FATAL, "failed to allocate memory for the window manager");
        exit(EXIT_FAILURE);
    }

    if(load_configuration(&gwm->config) == -1){
        log_message(LOG_FATAL, "failed to load configuration");
        exit(EXIT_FAILURE);
    }

    gwm->connection = xcb_connect(NULL, NULL);
    if(xcb_connection_has_error(gwm->connection)){
        log_message(LOG_ERROR, "failed to connect to the X server");
        exit(EXIT_FAILURE);
    }

    // TODO: Check composite extension availability

    // Get the default screen and root window
    gwm->screen = xcb_setup_roots_iterator(xcb_get_setup(gwm->connection)).data;
    if(!gwm->screen){
        log_message(LOG_FATAL, "failed to get the default screen");
        xcb_disconnect(gwm->connection);
        exit(EXIT_FAILURE);
    }

    uint32_t values[1] = {
        XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT 
        | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY 
        | XCB_EVENT_MASK_STRUCTURE_NOTIFY 
        | XCB_EVENT_MASK_FOCUS_CHANGE
	};

    gwm->cookie = xcb_change_window_attributes_checked(gwm->connection, gwm->screen->root, XCB_CW_EVENT_MASK, values);
    gwm->error = xcb_request_check(gwm->connection, gwm->cookie);

    if(gwm->error){
        log_message(LOG_FATAL, "another window manager is already running");
        free(gwm->error);
        xcb_disconnect(gwm->connection);
        exit(EXIT_FAILURE);
    }

    gwm->wm_window = xcb_generate_id(gwm->connection);
	xcb_create_window(gwm->connection, XCB_COPY_FROM_PARENT, gwm->wm_window, gwm->screen->root, -1, -1, 1, 1, 0, XCB_WINDOW_CLASS_INPUT_ONLY, XCB_COPY_FROM_PARENT, XCB_NONE, NULL);

    // TODO: organize this code in another area
    // ICCCM and EWMH implementation
    xcb_intern_atom_cookie_t *cookies = xcb_ewmh_init_atoms(gwm->connection, &gwm->ewmh);
    xcb_ewmh_init_atoms_replies(&gwm->ewmh, cookies, NULL);

    // TODO: wrapper to change properties (verifies cookie and reply) and applies changes
    // xcb_ewmh_set_supporting_wm_check(&gwm->ewmh, gwm->screen->root, gwm->screen->root);
    // xcb_ewmh_set_wm_name(&gwm->ewmh, gwm->screen->root, strlen("gwm"), "gwm");
    // xcb_change_property(gwm->connection, XCB_PROP_MODE_REPLACE, gwm->screen->root, XCB_ATOM_WM_CLASS, XCB_ATOM_STRING, 8, strlen("gwm"), "gwm");
    xcb_icccm_set_wm_class(gwm->connection, gwm->wm_window, strlen("gwm"), "gwm");

    pid_t wm_pid = getpid();
    xcb_ewmh_set_supporting_wm_check(&gwm->ewmh, gwm->screen->root, gwm->wm_window);
    xcb_ewmh_set_supporting_wm_check(&gwm->ewmh, gwm->wm_window, gwm->wm_window);
    xcb_ewmh_set_wm_name(&gwm->ewmh, gwm->wm_window, strlen("gwm"), "gwm");
    xcb_ewmh_set_wm_pid(&gwm->ewmh, gwm->wm_window, wm_pid);

    // TODO: set XCB_ATOM_WM_PROTOCOLS (?) -- needs more research
    // TODO: set xcb_ewmh_set_desktop_layout (ewmh desktop layout orientation) (?) -- needs more research
    // TODO: set xcb_ewmh_set_desktop_names (?) -- needs more research
    xcb_ewmh_set_number_of_desktops(&gwm->ewmh, 0, gwm->config.workspace_count);
    gwm->num_workspaces = gwm->config.workspace_count;

    // set _NET_CURRENT_DESKTOP
    xcb_ewmh_set_current_desktop(&gwm->ewmh, 0, 0);
    gwm->current_workspace_id = 0;

    // TODO: set _NET_CLIENT_LIST (?) -- needs more research
    
    // TODO: organize this code in another area - updating number of screens -- not implemented yet
    // Query the RandR extension
    const xcb_query_extension_reply_t *extension_reply = xcb_get_extension_data(gwm->connection, &xcb_randr_id);
    if(!extension_reply || !extension_reply->present){
        log_message(LOG_FATAL, "RandR extension not available");
        xcb_disconnect(gwm->connection);
        exit(EXIT_FAILURE);
    }

    // Get the screen resources
    xcb_randr_get_screen_resources_cookie_t screen_resources_cookie = xcb_randr_get_screen_resources(gwm->connection, xcb_setup_roots_iterator(xcb_get_setup(gwm->connection)).data->root);
    xcb_randr_get_screen_resources_reply_t *screen_resources = xcb_randr_get_screen_resources_reply(gwm->connection, screen_resources_cookie, NULL);
    if(!screen_resources){
        log_message(LOG_FATAL, "failed to get screen resources");
        xcb_disconnect(gwm->connection);
        exit(EXIT_FAILURE);
    }

    // Get the number of CRTCs (physical monitors)
    int num_crtcs = xcb_randr_get_screen_resources_crtcs_length(screen_resources);
    log_message(LOG_DEBUG, "number of physical monitors connected: %d", num_crtcs);

    // Clean up and disconnect from X server
    free(screen_resources);

    // Ungrab all keys
    xcb_ungrab_key(gwm->connection, XCB_GRAB_ANY, gwm->screen->root, XCB_MOD_MASK_ANY);

    gwm->key_symbols = xcb_key_symbols_alloc(gwm->connection);
    if(!gwm->key_symbols){
        log_message(LOG_FATAL, "failed to allocate key symbols");
        xcb_disconnect(gwm->connection);
        exit(EXIT_FAILURE);
    }

    // TODO: implement configurable and flexible keybindings
    // MOD1 + Q
    xcb_keycode_t *keycode_q = xcb_key_symbols_get_keycode(gwm->key_symbols, XK_Q);
    xcb_grab_key(gwm->connection, 1, gwm->screen->root, XCB_MOD_MASK_1, *keycode_q,
        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

    // MOD1 + Tab - cycle through workspaces
    xcb_keycode_t *keycode_tab = xcb_key_symbols_get_keycode(gwm->key_symbols, XK_Tab);
    xcb_grab_key(gwm->connection, 1, gwm->screen->root, XCB_MOD_MASK_1, *keycode_tab,
        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

    // MOD1 + L - reset workspace layouts
    xcb_keycode_t *keycode_l = xcb_key_symbols_get_keycode(gwm->key_symbols, XK_L);
    xcb_grab_key(gwm->connection, 1, gwm->screen->root, XCB_MOD_MASK_1, *keycode_l,
        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

    free(keycode_q);
    free(keycode_l);
    free(keycode_tab);

    /*xcb_grab_button(gwm->connection, 0, gwm->screen->root, XCB_EVENT_MASK_BUTTON_PRESS |
        XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
        XCB_GRAB_MODE_ASYNC, gwm->screen->root, XCB_NONE, 1, XCB_MOD_MASK_1); // XCB_NONE to allow mouse events without Mod1 pressed

    xcb_grab_button(gwm->connection, 0, gwm->screen->root, XCB_EVENT_MASK_BUTTON_PRESS |
        XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
        XCB_GRAB_MODE_ASYNC, gwm->screen->root, XCB_NONE, 3, XCB_MOD_MASK_1);*/

    xcb_flush(gwm->connection);

    // Create workspaces
    gwm->workspaces = (Workspace *)calloc(gwm->config.workspace_count, sizeof(Workspace));
    if(!gwm->workspaces){
        log_message(LOG_ERROR, "failed to allocate memory for workspaces");
        xcb_disconnect(gwm->connection);
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < gwm->config.workspace_count; i++){
        Workspace *workspace = create_workspace(gwm, gwm->config.workspace_layouts[i]);
        if(!workspace){
            log_message(LOG_FATAL, "failed to create workspace");
            exit(EXIT_FAILURE);
        }

        gwm->workspaces[i] = *workspace;
        free(workspace);
    }

    log_message(LOG_INFO, "gwm running");
    log_message(LOG_DEBUG, "number of workspaces: %d", gwm->num_workspaces);
    log_message(LOG_DEBUG, "current workspace id: %d", gwm->current_workspace_id);
    log_message(LOG_DEBUG, "screen width: %d", gwm->screen->width_in_pixels);
    log_message(LOG_DEBUG, "screen height: %d", gwm->screen->height_in_pixels);
    log_message(LOG_DEBUG, "root window id: %d", gwm->screen->root);

    for(int i = 0; i < gwm->config.workspace_count; i++){
        char *layout_str = LAYOUT_MODE_TO_STRING(gwm->workspaces[i].layout_mode);
        log_message(LOG_DEBUG, "workspace %d layout: %s", i, layout_str);
    }

    // TODO: handle rules -- not implemented yet
    // TODO: handle previous opened windows before gwm launching -- not implemented yet
    return gwm;
}

void cleanup(WindowManager *gwm){
    for(int i = 0; i < gwm->num_workspaces; i++){
        for(int j = 0; j < gwm->workspaces[i].num_clients_in_layout_clients; j++){
            free(gwm->workspaces[i].layout_clients[j]);
        }

        free(gwm->workspaces[i].layout_clients);
        for(int j = 0; j < gwm->workspaces[i].num_clients; j++){
            xcb_unmap_window(gwm->connection, gwm->workspaces[i].clients[j].window);
            xcb_destroy_window(gwm->connection, gwm->workspaces[i].clients[j].window);
        }

        free(gwm->workspaces[i].clients);
    }

    free(gwm->config.workspace_layouts);
    free(gwm->workspaces);
    free(gwm->rules);
    xcb_key_symbols_free(gwm->key_symbols);
    xcb_ewmh_connection_wipe(&gwm->ewmh);
    xcb_disconnect(gwm->connection);
}
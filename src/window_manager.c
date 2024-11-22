#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>
#include <xcb/xcb_util.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/randr.h>
#include <unistd.h> 
#include <string.h>
#include "structs.h"
#include "window_manager.h"

// TODO: revisit these functions (get_keysym_from_name, get_special_keycode, get_keycode) and this struct (KeySymMap)
typedef struct {
    const char *name;
    xcb_keysym_t keysym;
} KeySymMap;

xcb_keysym_t get_keysym_from_name(xcb_connection_t *connection, const char *keyname) {
    xcb_get_keyboard_mapping_cookie_t cookie = xcb_get_keyboard_mapping(connection, 8, 255 - 8 + 1);
    xcb_get_keyboard_mapping_reply_t *reply = xcb_get_keyboard_mapping_reply(connection, cookie, NULL);
    
    if (!reply) {
        return XCB_NO_SYMBOL;
    }

    xcb_keysym_t *keysyms = xcb_get_keyboard_mapping_keysyms(reply);
    int keysyms_per_keycode = reply->keysyms_per_keycode;

    KeySymMap known_keys[] = {
        {"Tab", 0},
        {"Return", 0},
        {"Escape", 0},
    };

    for (xcb_keycode_t keycode = 8; keycode < 255; keycode++) {
        for (int i = 0; i < keysyms_per_keycode; i++) {
            xcb_keysym_t keysym = keysyms[(keycode - 8) * keysyms_per_keycode + i];
            for (size_t j = 0; j < sizeof(known_keys) / sizeof(KeySymMap); j++) {
                if (known_keys[j].keysym == 0 && strcasecmp(keyname, known_keys[j].name) == 0) {
                    known_keys[j].keysym = keysym;
                }
            }
        }
    }

    xcb_keysym_t result = XCB_NO_SYMBOL;
    for (size_t i = 0; i < sizeof(known_keys) / sizeof(KeySymMap); i++) {
        if (strcasecmp(keyname, known_keys[i].name) == 0) {
            result = known_keys[i].keysym;
            break;
        }
    }

    free(reply);
    return result;
}

xcb_keycode_t get_special_keycode(xcb_connection_t *connection, const char *keyname) {
    xcb_keysym_t keysym = get_keysym_from_name(connection, keyname);
    if (keysym == XCB_NO_SYMBOL) {
        return 0;
    }

    xcb_get_keyboard_mapping_cookie_t cookie = xcb_get_keyboard_mapping(connection, 8, 255 - 8 + 1);
    xcb_get_keyboard_mapping_reply_t *reply = xcb_get_keyboard_mapping_reply(connection, cookie, NULL);
    
    if (!reply) {
        return 0;
    }

    xcb_keysym_t *keysyms = xcb_get_keyboard_mapping_keysyms(reply);
    int keysyms_per_keycode = reply->keysyms_per_keycode;

    xcb_keycode_t result_keycode = 0;
    for (xcb_keycode_t keycode = 8; keycode < 255; keycode++) {
        for (int i = 0; i < keysyms_per_keycode; i++) {
            if (keysyms[(keycode - 8) * keysyms_per_keycode + i] == keysym) {
                result_keycode = keycode;
                break;
            }
        }
        if (result_keycode) break;
    }

    free(reply);
    return result_keycode;
}

xcb_keycode_t get_keycode(xcb_connection_t *connection, char key) {
    xcb_get_keyboard_mapping_cookie_t cookie = xcb_get_keyboard_mapping(connection, 8, 255 - 8 + 1);
    xcb_get_keyboard_mapping_reply_t *reply = xcb_get_keyboard_mapping_reply(connection, cookie, NULL);
    
    if (!reply) {
        return 0;
    }
    
    xcb_keysym_t *keysyms = xcb_get_keyboard_mapping_keysyms(reply);
    int keysyms_per_keycode = reply->keysyms_per_keycode;
    xcb_keycode_t result_keycode = 0;
    
    for (xcb_keycode_t keycode = 8; keycode < 255; keycode++) {
        for (int i = 0; i < keysyms_per_keycode; i++) {
            xcb_keysym_t keysym = keysyms[(keycode - 8) * keysyms_per_keycode + i];
            xcb_keysym_t target = (xcb_keysym_t)key;
            xcb_keysym_t alternate = (key >= 'a' && key <= 'z') ? 
                                    (xcb_keysym_t)(key - 32) : 
                                    (xcb_keysym_t)key;
            
            if (keysym == target || keysym == alternate) {
                result_keycode = keycode;
                break;
            }
        }
        if (result_keycode) break;
    }
    
    free(reply);
    return result_keycode;
}

WindowManager *setup(){
    WindowManager *gwm = (WindowManager *)calloc(1, sizeof(WindowManager));

    if(!gwm){
        printf("[fatal]: failed to allocate memory for the window manager\n");
        cleanup(gwm);
        exit(EXIT_FAILURE);
    }

    // TODO: implement configuration file

    gwm->connection = xcb_connect(NULL, NULL);
    if(xcb_connection_has_error(gwm->connection)){
        printf("[error]: failed to connect to the X server\n");
        cleanup(gwm);
        exit(EXIT_FAILURE);
    }

    gwm->screen = xcb_setup_roots_iterator(xcb_get_setup(gwm->connection)).data;
    if(!gwm->screen){
        printf("[fatal]: failed to get the default screen\n");
        xcb_disconnect(gwm->connection);
        exit(EXIT_FAILURE);
    }

    const uint32_t root_background[] = { gwm->screen->black_pixel };
    xcb_void_cookie_t clear_cookie = xcb_change_window_attributes_checked(gwm->connection, gwm->screen->root, XCB_CW_BACK_PIXEL, root_background);

    xcb_generic_error_t *clear_error = xcb_request_check(gwm->connection, clear_cookie);
    if(clear_error){
        printf("[error]: failed to set root window background\n");
        free(clear_error);
    }

    xcb_clear_area(gwm->connection, 1, gwm->screen->root, 0, 0, gwm->screen->width_in_pixels, gwm->screen->height_in_pixels);

    uint32_t values[1] = {
        XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT 
        | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY 
        | XCB_EVENT_MASK_STRUCTURE_NOTIFY 
        | XCB_EVENT_MASK_FOCUS_CHANGE
	};

    gwm->cookie = xcb_change_window_attributes_checked(gwm->connection, gwm->screen->root, XCB_CW_EVENT_MASK, values);
    gwm->error = xcb_request_check(gwm->connection, gwm->cookie);

    if(gwm->error){
        printf("[fatal]: another window manager is already running\n");
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
    
    // xcb_ewmh_set_number_of_desktops(&gwm->ewmh, 0, gwm->config.workspace_count);
    // gwm->num_workspaces = gwm->config.workspace_count;

    // set _NET_CURRENT_DESKTOP
    xcb_ewmh_set_current_desktop(&gwm->ewmh, 0, 0);
    gwm->current_workspace_id = 0;

    // TODO: set _NET_CLIENT_LIST (?) -- needs more research
    
    // TODO: organize this code in another area - updating number of screens -- not implemented yet
    // Query the RandR extension
    const xcb_query_extension_reply_t *extension_reply = xcb_get_extension_data(gwm->connection, &xcb_randr_id);
    if(!extension_reply || !extension_reply->present){
        printf("[fatal]: RandR extension not available\n");
        xcb_disconnect(gwm->connection);
        exit(EXIT_FAILURE);
    }

    // Get the screen resources
    xcb_randr_get_screen_resources_cookie_t screen_resources_cookie = xcb_randr_get_screen_resources(gwm->connection, xcb_setup_roots_iterator(xcb_get_setup(gwm->connection)).data->root);
    xcb_randr_get_screen_resources_reply_t *screen_resources = xcb_randr_get_screen_resources_reply(gwm->connection, screen_resources_cookie, NULL);
    if(!screen_resources){
        printf("[fatal]: failed to get screen resources\n");
        xcb_disconnect(gwm->connection);
        exit(EXIT_FAILURE);
    }

    // Get the number of CRTCs (physical monitors)
    int num_crtcs = xcb_randr_get_screen_resources_crtcs_length(screen_resources);
    printf("[debug]: number of physical monitors connected: %d\n", num_crtcs);
    
    // Clean up and disconnect from X server
    free(screen_resources);

    // Ungrab all keys
    xcb_ungrab_key(gwm->connection, XCB_GRAB_ANY, gwm->screen->root, XCB_MOD_MASK_ANY);

    gwm->key_symbols = xcb_key_symbols_alloc(gwm->connection);
    if(!gwm->key_symbols){
        printf("[fatal]: failed to allocate key symbols\n");
        xcb_disconnect(gwm->connection);
        exit(EXIT_FAILURE);
    }

    // TODO: implement configurable and flexible keybindings
    // MOD1 + Q
    xcb_keycode_t keycode_q = get_keycode(gwm->connection, 'Q');
    if(keycode_q){
        xcb_grab_key(gwm->connection, 1, gwm->screen->root, XCB_MOD_MASK_1, keycode_q, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    }

    // MOD1 + Tab - cycle through workspaces
    xcb_keycode_t keycode_tab = get_special_keycode(gwm->connection, "Tab");
    if(keycode_tab){
        xcb_grab_key(gwm->connection, 1, gwm->screen->root, XCB_MOD_MASK_1, keycode_tab, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    }

    // MOD1 + L - reset workspace layouts
    xcb_keycode_t keycode_l = get_keycode(gwm->connection, 'L');
    if(keycode_l){
        xcb_grab_key(gwm->connection, 1, gwm->screen->root, XCB_MOD_MASK_1, keycode_l, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    }
    
    xcb_flush(gwm->connection);

    // TODO: create workspaces

    printf("[info]: gwm running\n");
    printf("[info]: gwm window id: %d\n", gwm->wm_window);
    printf("[debug]: number of workspaces: %d\n", gwm->num_workspaces);
    printf("[debug]: current workspace id: %d\n", gwm->current_workspace_id);
    printf("[debug]: screen width: %d\n", gwm->screen->width_in_pixels);
    printf("[debug]: screen height: %d\n", gwm->screen->height_in_pixels);
    printf("[debug]: root window id: %d\n", gwm->screen->root);

    // TODO: handle rules -- not implemented yet
    // TODO: handle previous opened windows before gwm launching -- partially implemented
    xcb_query_tree_cookie_t tree_cookie = xcb_query_tree(gwm->connection, gwm->screen->root);
    xcb_query_tree_reply_t *tree_reply = xcb_query_tree_reply(gwm->connection, tree_cookie, NULL);
    if(!tree_reply){
        fprintf(stderr, "error getting the tree reply\n");
        xcb_disconnect(gwm->connection);
        exit(EXIT_FAILURE);
    }

    xcb_window_t *children = xcb_query_tree_children(tree_reply);
    int num_children = xcb_query_tree_children_length(tree_reply);

    for (int i = 0; i < num_children; i++){
        xcb_window_t window = children[i];
        if(window == gwm->wm_window)
            continue;

        xcb_get_window_attributes_cookie_t window_attributes_cookie = xcb_get_window_attributes(gwm->connection, window);
        xcb_get_window_attributes_reply_t *window_attributes_reply = xcb_get_window_attributes_reply(gwm->connection, window_attributes_cookie, NULL);
        if(window_attributes_reply->override_redirect){
            free(window_attributes_reply);
            continue;
        }

        // TODO: handle window types (desktop, dock, toolbar, etc)
        bool ignore_window = false;
        xcb_ewmh_get_atoms_reply_t win_type;
        if(xcb_ewmh_get_wm_window_type_reply(&gwm->ewmh, xcb_ewmh_get_wm_window_type(&gwm->ewmh, window), &win_type, NULL)){
            for (unsigned int i = 0; i < win_type.atoms_len; i++){
                xcb_atom_t a = win_type.atoms[i];
                if(a == gwm->ewmh._NET_WM_WINDOW_TYPE_DESKTOP || a == gwm->ewmh._NET_WM_WINDOW_TYPE_DOCK || a == gwm->ewmh._NET_WM_WINDOW_TYPE_TOOLBAR || a == gwm->ewmh._NET_WM_WINDOW_TYPE_MENU || a == gwm->ewmh._NET_WM_WINDOW_TYPE_SPLASH || a == gwm->ewmh._NET_WM_WINDOW_TYPE_UTILITY){
                    ignore_window = true;
                    break;
                }
            }
            xcb_ewmh_get_atoms_reply_wipe(&win_type);
        } else {
            printf("[error]: failed to get _NET_WM_WINDOW_TYPE property of window: %d\n", window);
            ignore_window = true;
        }

        if(ignore_window){
            free(window_attributes_reply);
            continue;
        }

        // TODO: handle windows that launched before gwm?
    }
    
    free(tree_reply);
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

    free(gwm->workspaces);
    xcb_key_symbols_free(gwm->key_symbols);
    xcb_ewmh_connection_wipe(&gwm->ewmh);
    xcb_disconnect(gwm->connection);
    free(gwm);
    printf("[info]: gwm terminated\n");
}
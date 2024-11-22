#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <xcb/xcb.h>
#include <xcb/xcb_util.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_icccm.h>
#include "structs.h"

// TODO: check if this is the best flag to keep the event loop and handling signals
volatile sig_atomic_t run = 1;

void sigint_handler(){
    run = 0;
}

int handle_button_press(WindowManager *gwm, xcb_generic_event_t *event){
    xcb_button_press_event_t *button_press_event = (xcb_button_press_event_t *)event;
    printf("Button press event: %d\n", button_press_event->detail);
    return 0;
}

int handle_enter_notify(WindowManager *gwm, xcb_generic_event_t *event){
    xcb_enter_notify_event_t *enter_notify_event = (xcb_enter_notify_event_t *)event;
    printf("Enter notify event: %d\n", enter_notify_event->event);
    return 0;
}

int handle_configure_request(WindowManager *gwm, xcb_generic_event_t *event){
    printf("Configure request event\n");
    return 0;
}

int handle_destroy_notify(WindowManager *gwm, xcb_generic_event_t *event){
    printf("Destroy notify event\n");
    return 0;
}

int handle_key_press(WindowManager *gwm, xcb_generic_event_t *event){
    xcb_key_press_event_t *key_event = (xcb_key_press_event_t *)event;
    printf("Key press event: %d\n", key_event->detail);
    return 0;
}

int handle_map_request(WindowManager *gwm, xcb_generic_event_t *event){
    printf("Map request event\n");
    return 0;
}

int handle_unmap_notify(WindowManager *gwm, xcb_generic_event_t *event){
    printf("Unmap notify event\n");
    return 0;
}

int handle_client_message(WindowManager *gwm, xcb_generic_event_t *event){
    printf("Client message event\n");
    return 0;
}

int handle_motion_notify(WindowManager *gwm, xcb_generic_event_t *event){
    printf("Motion notify event\n");
    return 0;
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
                case XCB_CLIENT_MESSAGE:
                    handle_client_message(gwm, event);
                default:
                    break;
            }
        }

        free(event);
    }
    return 0;
}

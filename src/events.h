#ifndef EVENTS_H
#define EVENTS_H

#include "structs.h"

int handle_events(WindowManager *gwm);
int handle_configure_request(WindowManager *gwm, xcb_generic_event_t *event);

#endif

#ifndef WORKSPACE_H
#define WORKSPACE_H

#include "structs.h"

Workspace *create_workspace(WindowManager *gwm, LayoutMode layout_mode);
void realloc_workspace(WindowManager *gwm, Workspace *workspace);
int add_client_to_workspace(WindowManager *gwm, Workspace *workspace, Client *client);

#endif
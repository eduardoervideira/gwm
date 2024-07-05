#ifndef WORKSPACE_H
#define WORKSPACE_H

#include "structs.h"

Workspace *create_workspace(WindowManager *gwm, LayoutMode layout_mode);

int add_client_to_workspace(WindowManager *gwm, Workspace *workspace, Client *client);
void remove_client_from_workspace(Workspace *workspace, Client *client);
void swap_clients(Workspace *workspace, Client *client1, Client *client2);

void map_clients(WindowManager *gwm, Workspace *workspace);
void unmap_clients(WindowManager *gwm, Workspace *workspace);

int cycle_through_clients(WindowManager *gwm, Workspace *workspace);
int cycle_through_workspaces(WindowManager *gwm);

int change_workspace_layout(WindowManager *gwm, Workspace *workspace, LayoutMode layout_mode);

#endif
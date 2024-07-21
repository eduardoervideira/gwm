#ifndef LAYOUT_H
#define LAYOUT_H

#include "structs.h"

int establish_layout(WindowManager *gwm);
int establish_stacking_mode(WindowManager *gwm, Workspace *workspace);
int establish_monocle_mode(WindowManager *gwm, Workspace *workspace);
int establish_columns_mode(WindowManager *gwm, Workspace *workspace);
int establish_rows_mode(WindowManager *gwm, Workspace *workspace);
int establish_n_columns_mode(WindowManager *gwm, Workspace *workspace, uint8_t num_columns);
int establish_n_rows_mode(WindowManager *gwm, Workspace *workspace, uint8_t num_rows);
void fibonacci(WindowManager *gwm);

#endif
//******************************************************************************
// actions.h
//
// Prototype definitions for the actions functions.
//
// Date      Name       Description
// ========  =========  ========================================================
// 12/17/09  Graves     Initial version.
//******************************************************************************

#ifndef _ACTIONS_H_
#define _ACTIONS_H_

void GP_add_xp(void *arg, int num);
void GP_add_ps(void *arg, int num);
void GP_add_vertex_i(void *arg, int v, int label);
void GP_add_vertex_f(void *arg, int v, double label);
void GP_add_vertex_s(void *arg, int v, char *label);
void GP_add_edge_i(void *arg, int type, int src, int dst, int label);
void GP_add_edge_f(void *arg, int type, int src, int dst, double label);
void GP_add_edge_s(void *arg, int type, int src, int dst, char *label);

#endif

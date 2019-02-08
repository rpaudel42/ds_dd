//******************************************************************************
// actions.c
//
// GBAD functions for graph parsing for the GUI.
//
// Date      Name       Description
// ========  =========  ========================================================
// 12/17/09  Graves     Initial version.
// 06/15/14  Eberle     Added fclose to GP_read_graph.
// 01/02/15  Graves     Changed the return type of int to GP_read_graph.
//
//******************************************************************************

#include <stdio.h>
#include <stdlib.h>

#include "gbad.h"
#include "actions.h"

#include "y.tab.h"
#include "lex.yy.h"

int yyparse(void *arg);
void yyerror(void *arg, const char *str);

extern int yylineno;
char *GP_file_name;

#define ERR_STR_LEN 64

char GP_err_str[ERR_STR_LEN];

//******************************************************************************
// NAME: GP_read_graph
//
// INPUTS: (Graph_Info *info)
//         (char *inputFileName) - specifies the input file to read 
// RETURN: (int) - 0 if success, 1 if error
//
// PURPOSE: Reads in the graph input file (MDL/FSM format).  The field 
// info->xp_graph specifies the type of input file to be read in.
// 
// If info->xp_graph is TRUE, then inputFileName is treated as a graph and read
// into the info->graph field.  If info->graph already contains a graph (i.e.
// info->graph is non-NULL) then info->posGraphVertexListSize,
// info->posGraphEdgeListSize, and info->vertexOffset are used to append the
// input graph onto info->graph.  Upon return, info->posEgsVertexIndices will
// contain vertex indices of where positive egs begin.  Also, the values in 
// posGraphVertexListSize, posGraphEdgeListSize, and vertexOffset will have
// been updated.
//
// If info->xp_graph is FALSE, then inputFileName is treated as containing 
// PS examples, which are read into the info->preSubs field.  If info->preSubs
// is non-NULL, then numPreSubs is used to append the graphs found in the input
// file to the end of preSubs.  Upon return, the value in numPreSubs will have
// been updated.
//
// The filed info->labelList is used to store labels found in the input file
// and is assumed to be non-NULL.  The info->directed field is used to
// determine if 'e' edges are directed.
//******************************************************************************
int GP_read_graph(Graph_Info *info, char *inputFileName)
{
   int ret;
   FILE *tmp = yyin;
	
   FILE *input = fopen(inputFileName, "r");
   if (input == NULL)
   {
      fprintf(stderr, "Unable to open input file %s.\n", inputFileName);
      exit(1);
   }
	
   GP_file_name = inputFileName;
	
   yyin = input;
   yylineno = 1;
   ret = yyparse((void *)info);
   yyin = tmp;
   fclose(input);
   
   return ret;
}

//******************************************************************************
// NAME:    GP_add_xp
//
// INPUTS:  input - file pointer to read the input graph from
//          GP_info - structure used to store the graph read in
//
// RETURN:  none
//
// PURPOSE: this function sets up and calls yyparse to parse the input graph
//
//******************************************************************************
void GP_add_xp(void *arg, int num)
{
   Graph_Info *GP_info = (Graph_Info *)arg;
	
   if (!GP_info->xp_graph)
   {
      yyerror(NULL, "invalid graph type, found PS, expecting XP.");
      exit(1);	
   }
	
   if (num != GP_info->numPosEgs+1)
   {
      snprintf(GP_err_str, ERR_STR_LEN, "invalid XP number, found %d, expecting %lu",
               num, GP_info->numPosEgs+1);
      yyerror(NULL, GP_err_str);
      exit(1);
   }
	
   if (GP_info->graph == NULL)
   {
      GP_info->graph = AllocateGraph(0,0);
   }
	
   GP_info->numPosEgs++;
   GP_info->vertexOffset = GP_info->graph->numVertices;
   GP_info->posEgsVertexIndices = AddVertexIndex(
   GP_info->posEgsVertexIndices,
   GP_info->numPosEgs, GP_info->vertexOffset);
}

//******************************************************************************
// NAME:    GP_add_ps
//
// INPUTS:  arg - pointer to the argument passed to yyparse
//          num - the number of the PS instance
//
// RETURN:  none
//
// PURPOSE: function called by the parser when it finds a new PS instance
//
// NOTE:    PS graphs are not yet supported by gbad-fsm.  A PS instance will
//          cause a parse error.
//
//******************************************************************************
void GP_add_ps(void *arg, int num)
{
   Graph_Info *GP_info = (Graph_Info *)arg;
	
   if (GP_info->xp_graph)
   {
      yyerror(NULL, "invalid graph type, found PS, expecting XP.");
      exit(1);	
   }
	
   if (num != GP_info->numPreSubs+1)
   {
      snprintf(GP_err_str, ERR_STR_LEN, "invalid PS number, found %d, expecting %lu",
               num, GP_info->numPreSubs+1);
      yyerror(NULL, GP_err_str);
      exit(1);
   }
	
   GP_info->numPreSubs++;
   GP_info->preSubs = (Graph **) realloc(GP_info->preSubs, (sizeof(Graph *) * GP_info->numPreSubs));
   if (GP_info->preSubs == NULL)
   {
      OutOfMemoryError("ReadSubGraphsFromFile:subGraphs");
   }
	
   GP_info->preSubs[GP_info->numPreSubs - 1] = AllocateGraph(0, 0);
   GP_info->graph = GP_info->preSubs[GP_info->numPreSubs - 1];
   GP_info->posGraphVertexListSize = 0;
   GP_info->posGraphEdgeListSize = 0;
}

//******************************************************************************
// NAME:    GP_add_vertex_i
//
// INPUTS:  arg   - pointer to the argument passed to yyparse
//          v     - the number of the vertex
//          label - the label of the vertex
//
// RETURN:  none
//
// PURPOSE: function called by the parser when it finds a vertex with an integer
//          label
//
//******************************************************************************
void GP_add_vertex_i(void *arg, int v, int label)
{
   Graph_Info *GP_info = (Graph_Info *)arg;
	
   ULONG labelIndex;
   Label graph_label;
	
   if (v+GP_info->vertexOffset != (GP_info->graph->numVertices + 1))
   {
      snprintf(GP_err_str, ERR_STR_LEN, "invalid vertex number, found %d, expecting %lu",
               v, (GP_info->graph->numVertices + 1)-GP_info->vertexOffset);
      yyerror(NULL, GP_err_str);
      exit(1);
   }
	
   graph_label.labelType = NUMERIC_LABEL;
   graph_label.labelValue.numericLabel = (double)label;
	
   labelIndex = StoreLabel(&graph_label, GP_info->labelList);
   AddVertex(GP_info->graph, labelIndex, &(GP_info->posGraphVertexListSize), v);
}

//******************************************************************************
// NAME:    GP_add_vertex_f
//
// INPUTS:  arg   - pointer to the argument passed to yyparse
//          v     - the number of the vertex
//          label - the label of the vertex
//
// RETURN:  none
//
// PURPOSE: function called by the parser when it finds a vertex with a floating
//          point label
//
//******************************************************************************
void GP_add_vertex_f(void *arg, int v, double label)
{
   Graph_Info *GP_info = (Graph_Info *)arg;
	
   ULONG labelIndex;
   Label graph_label;
	
   if (v+GP_info->vertexOffset !=
   	(GP_info->graph->numVertices + 1)) 
   {
      snprintf(GP_err_str, ERR_STR_LEN, "invalid vertex number, found %d, expecting %lu",
               v, (GP_info->graph->numVertices + 1)-GP_info->vertexOffset);
      yyerror(NULL, GP_err_str);
      exit(1);
   }
	
   graph_label.labelType = NUMERIC_LABEL;
   graph_label.labelValue.numericLabel = label;
   
   labelIndex = StoreLabel(&graph_label, GP_info->labelList);
   AddVertex(GP_info->graph, labelIndex, &(GP_info->posGraphVertexListSize), v);
}

//******************************************************************************
// NAME:    GP_add_vertex_s
//
// INPUTS:  arg   - pointer to the argument passed to yyparse
//          v     - the number of the vertex
//          label - the label of the vertex
//
// RETURN:  none
//
// PURPOSE: function called by the parser when it finds a vertex with a string
//          label
//
//******************************************************************************
void GP_add_vertex_s(void *arg, int v, char *label)
{
   Graph_Info *GP_info = (Graph_Info *)arg;
	
   ULONG labelIndex;
   Label graph_label;
	
   if (v+GP_info->vertexOffset !=
   	(GP_info->graph->numVertices + 1)) 
   {
      snprintf(GP_err_str, ERR_STR_LEN, "invalid vertex number, found %d, expecting %lu",
      	       v, (GP_info->graph->numVertices + 1)-GP_info->vertexOffset);
      yyerror(NULL, GP_err_str);
      exit(1);
   }
   
   graph_label.labelType = STRING_LABEL;
   graph_label.labelValue.stringLabel = label;
   
   labelIndex = StoreLabel(&graph_label, GP_info->labelList);
   AddVertex(GP_info->graph, labelIndex, &(GP_info->posGraphVertexListSize), v);
   
   free(label);
}

//******************************************************************************
// NAME:    GP_add_edge_i
//
// INPUTS:  arg   - pointer to the argument passed to yyparse
//          type  - integer value representing the type of edge
//                  this will be one of the following defined in y.tab.h:
//                       D_EDGE E_EDGE U_EDGE
//          src   - the source vertex
//          dst   - the target vertex
//          label - the edge label
// RETURN:  none
//
// PURPOSE: function called by the parser when it finds an edge with an integer
//          label
//
//******************************************************************************
void GP_add_edge_i(void *arg, int type, int src, int dst, int label)
{
   ULONG labelIndex;
   Label graph_label;
	
   Graph_Info *GP_info = (Graph_Info *)arg;
	
   if (src+GP_info->vertexOffset > GP_info->graph->numVertices) 
   {
      snprintf(GP_err_str, ERR_STR_LEN, "undefined source vertex number, found %d",
      	       src);
      yyerror(NULL, GP_err_str);
      exit(1);
   }
	
   if (dst+GP_info->vertexOffset > GP_info->graph->numVertices) 
   {
      snprintf(GP_err_str, ERR_STR_LEN, "undefined target vertex number, found %d",
      	       src);
      yyerror(NULL, GP_err_str);
      exit(1);
   }
   
   graph_label.labelType = NUMERIC_LABEL;
   graph_label.labelValue.numericLabel = (double)label;
   
   labelIndex = StoreLabel(&graph_label, GP_info->labelList);
   
   if (type == E_EDGE)
   {
      //printf("e %d %d %d\n", src, dst, label);
   	
      AddEdge(GP_info->graph, src-1+GP_info->vertexOffset,
      	       dst-1+GP_info->vertexOffset, GP_info->directed,
      	       labelIndex, &(GP_info->posGraphEdgeListSize), FALSE);
   }
   else if (type == D_EDGE)
   {
      //printf("d %d %d %d\n", src, dst, label);
   	
      AddEdge(GP_info->graph, src-1+GP_info->vertexOffset,
      	       dst-1+GP_info->vertexOffset, TRUE, labelIndex,
             	&(GP_info->posGraphEdgeListSize), FALSE);
   
   }
   else if (type == U_EDGE)
   {
      //printf("u %d %d %d\n", src, dst, label);
   	
      AddEdge(GP_info->graph, src-1+GP_info->vertexOffset,
             	dst-1+GP_info->vertexOffset, FALSE, labelIndex,
             	&(GP_info->posGraphEdgeListSize), FALSE);
   }
}

//******************************************************************************
// NAME:    GP_add_edge_f
//
// INPUTS:  arg   - pointer to the argument passed to yyparse
//          type  - integer value representing the type of edge
//                  this will be one of the following defined in y.tab.h:
//                       D_EDGE E_EDGE U_EDGE
//          src   - the source vertex
//          dst   - the target vertex
//          label - the edge label
// RETURN:  none
//
// PURPOSE: function called by the parser when it finds an edge with an floating
//          point label
//
//******************************************************************************
void GP_add_edge_f(void *arg, int type, int src, int dst, double label)
{
   ULONG labelIndex;
   Label graph_label;
	
   Graph_Info *GP_info = (Graph_Info *)arg;
	
   if (src+GP_info->vertexOffset > GP_info->graph->numVertices) 
   {
      snprintf(GP_err_str, ERR_STR_LEN, "undefined target vertex number, found %d",
      	src);
      yyerror(NULL, GP_err_str);
      exit(1);
   }
   
   if (dst+GP_info->vertexOffset > GP_info->graph->numVertices) 
   {
      snprintf(GP_err_str, ERR_STR_LEN, "undefined target vertex number, found %d",
      	src);
      yyerror(NULL, GP_err_str);
      exit(1);
   }
   
   graph_label.labelType = NUMERIC_LABEL;
   graph_label.labelValue.numericLabel = label;
   
   labelIndex = StoreLabel(&graph_label, GP_info->labelList);
   
   if (type == E_EDGE)
   {
      //printf("e %d %d %f\n", src, dst, label);
   	
      AddEdge(GP_info->graph, src-1+GP_info->vertexOffset,
   		dst-1+GP_info->vertexOffset, GP_info->directed,
   		labelIndex, &(GP_info->posGraphEdgeListSize), FALSE);
   }
   else if (type == D_EDGE)
   {
      //printf("d %d %d %f\n", src, dst, label);
      
      AddEdge(GP_info->graph, src-1+GP_info->vertexOffset,
   		dst-1+GP_info->vertexOffset, TRUE, labelIndex,
   		&(GP_info->posGraphEdgeListSize), FALSE);
   
   }
   else if (type == U_EDGE)
   {
      //printf("u %d %d %f\n", src, dst, label);
   	
      AddEdge(GP_info->graph, src-1+GP_info->vertexOffset,
   		dst-1+GP_info->vertexOffset, FALSE, labelIndex,
   		&(GP_info->posGraphEdgeListSize), FALSE);
   }
}

//******************************************************************************
// NAME:    GP_add_edge_s
//
// INPUTS:  arg   - pointer to the argument passed to yyparse
//          type  - integer value representing the type of edge
//                  this will be one of the following defined in y.tab.h:
//                       D_EDGE E_EDGE U_EDGE
//          src   - the source vertex
//          dst   - the target vertex
//          label - the edge label
// RETURN:  none
//
// PURPOSE: function called by the parser when it finds an edge with a string
//          label
//
//******************************************************************************
void GP_add_edge_s(void *arg, int type, int src, int dst, char *label)
{
   ULONG labelIndex;
   Label graph_label;
	
   Graph_Info *GP_info = (Graph_Info *)arg;
	
   if (src+GP_info->vertexOffset > GP_info->graph->numVertices) 
   {
      snprintf(GP_err_str, ERR_STR_LEN, "undefined target vertex number, found %d",
   		src);
      yyerror(NULL, GP_err_str);
      exit(1);
   }
   
   if (dst+GP_info->vertexOffset > GP_info->graph->numVertices) 
   {
      snprintf(GP_err_str, ERR_STR_LEN, "undefined target vertex number, found %d",
   		src);
      yyerror(NULL, GP_err_str);
      exit(1);
   }
   
   graph_label.labelType = STRING_LABEL;
   graph_label.labelValue.stringLabel = label;
   
   labelIndex = StoreLabel(&graph_label, GP_info->labelList);
   
   if (type == E_EDGE)
   {
      //printf("e %d %d %s\n", src, dst, label);
   	
      AddEdge(GP_info->graph, src-1+GP_info->vertexOffset,
   		dst-1+GP_info->vertexOffset, GP_info->directed,
   		labelIndex, &(GP_info->posGraphEdgeListSize), FALSE);
   }
   else if (type == D_EDGE)
   {
      //printf("d %d %d %s\n", src, dst, label);
   	
      AddEdge(GP_info->graph, src-1+GP_info->vertexOffset,
   		dst-1+GP_info->vertexOffset, TRUE, labelIndex,
   		&(GP_info->posGraphEdgeListSize), FALSE);
   
   }
   else if (type == U_EDGE)
   {
      //printf("u %d %d %s\n", src, dst, label);
   	
      AddEdge(GP_info->graph, src-1+GP_info->vertexOffset,
   		dst-1+GP_info->vertexOffset, FALSE, labelIndex,
   		&(GP_info->posGraphEdgeListSize), FALSE);
   }
   
   free(label);
}

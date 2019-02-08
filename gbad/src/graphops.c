//******************************************************************************
// graphops.c
//
// Graph allocation, deallocation, input and output functions.
//
//
// Date      Name       Description
// ========  =========  ========================================================
// 08/12/09  Eberle     Initial version, taken from SUBDUE 5.2.1
// 12/17/09  Graves     Added GUI coloring logic
//
//******************************************************************************

#include "gbad.h"


//******************************************************************************
// NAME: ReadInputFile
//
// INPUTS: (Parameters *parameters)
//
// RETURN: (void)
//
// PURPOSE: Reads in the graph input file (SUBDUE format), which currently only
// consists of positive graphs (no negative graphs), which are collected into
// the positive graph fields of the parameters.  Each example in the input file 
// is prefaced by the appropriate token defined in gbad.h.  The first graph in 
// the file is assumed positive.  Each graph is assumed to begin at vertex #1 
// and therefore examples are not connected to one another.
//******************************************************************************

void ReadInputFile(Parameters *parameters)
{
   ULONG index;
   
   Graph_Info info;
   info.graph = parameters->posGraph;
   info.labelList = parameters->labelList;
   info.preSubs = NULL;
   info.numPreSubs = 0;
   info.numPosEgs = parameters->numPosEgs;
   info.posEgsVertexIndices = parameters->posEgsVertexIndices;
   info.directed = parameters->directed;
   
   info.posGraphVertexListSize = parameters->posGraphVertexListSize;
   info.posGraphEdgeListSize = parameters->posGraphEdgeListSize;
   info.vertexOffset = (parameters->posGraph == NULL) ? 0 : parameters->posGraph->numVertices;
   
   info.xp_graph = TRUE;
   
   GP_read_graph(&info, parameters->inputFileName);
  
   parameters->posGraph = info.graph;
   parameters->labelList = info.labelList;
   parameters->numPosEgs = info.numPosEgs;
   parameters->posEgsVertexIndices = info.posEgsVertexIndices;
   
   parameters->posGraphVertexListSize = info.posGraphVertexListSize;
   parameters->posGraphEdgeListSize = info.posGraphEdgeListSize;
   
	
   // GUI coloring
   if (parameters->posGraph == NULL)
      parameters->originalPosGraph = NULL;
   else
      parameters->originalPosGraph = CopyGraph(parameters->posGraph);
   parameters->originalLabelList = AllocateLabelList();
   for (index=0; index != parameters->labelList->numLabels; index++)
      StoreLabel(&(parameters->labelList->labels[index]), parameters->originalLabelList);
}


//******************************************************************************
// NAME: AddVertexIndex
//
// INPUTS: (ULONG *vertexIndices) - array of indices to augment
//         (ULONG n) - size of new array
//         (ULONG index) - index to add to nth element of array
//
// RETURN: (ULONG *) - new vertex index array
//
// PURPOSE: Reallocate the given vertex index array and store the new
// index in the nth element of the array.  This is used to build the
// array of indices into the positive examples graphs.
//******************************************************************************

ULONG *AddVertexIndex(ULONG *vertexIndices, ULONG n, ULONG index)
{
   vertexIndices = (ULONG *) realloc(vertexIndices, sizeof(ULONG) * n);
   if (vertexIndices == NULL)
      OutOfMemoryError("AddVertexIndex:vertexIndices");
   vertexIndices[n - 1] = index;
   return vertexIndices;
}


//******************************************************************************
// NAME: ReadPredefinedSubsFile
//
// INPUTS: (Parameters *parameters)
//
// RETURN: (void)
//
// PURPOSE: Reads in one or more graphs from the given file and stores
// these on the predefined substructure list in parameters.  Each
// graph is prefaced by the predefined substructure token defined in
// gbad.h.
//
// Right now, these substructures will be used to compress the graph,
// if present, and therefore any labels not present in the input graph
// will be discarded during compression.  If the predefined
// substructures are ever simply put on the discovery queue, then care
// should be taken to not include labels that do not appear in the
// input graph, as this would bias the MDL computation. (*****)
//******************************************************************************

void ReadPredefinedSubsFile(Parameters *parameters)
{
   Graph_Info info;
   info.graph = NULL;
   info.labelList = parameters->labelList;
   info.preSubs = parameters->preSubs;
   info.numPreSubs = parameters->numPreSubs;
   info.numPosEgs = 0;
   info.posEgsVertexIndices = 0;
   info.directed = parameters->directed;
   
   info.posGraphVertexListSize = 0;
   info.posGraphEdgeListSize = 0;
   info.vertexOffset = 0;
   
   info.xp_graph = FALSE;
   
   GP_read_graph(&info, parameters->psInputFileName);
  
   parameters->labelList = info.labelList;
   parameters->preSubs = info.preSubs;
   parameters->numPreSubs = info.numPreSubs;
}

//******************************************************************************
// NAME: AddVertex
//
// INPUTS: (Graph *graph) - graph to add vertex
//         (ULONG labelIndex) - index into label list of vertex's label
//         (ULONG *vertexListSize) - pointer to size of graph's allocated
//                                   vertex array
//         (ULONG sourceVertex) - vertex index to be saved for possible
//                                information after future compression
//
// RETURN: (void)
//
// PURPOSE: Add vertex information to graph. AddVertex also changes the
// size of the currently-allocated vertex array, which increases by
// LIST_SIZE_INC (instead of just 1) when exceeded.
//******************************************************************************

void AddVertex(Graph *graph, ULONG labelIndex, ULONG *vertexListSize, 
               ULONG sourceVertex)
{
   Vertex *newVertexList;
   ULONG numVertices;

   numVertices = graph->numVertices;
   // make sure there is enough room for another vertex
   if (*vertexListSize == graph->numVertices) 
   {
      *vertexListSize += LIST_SIZE_INC;
      newVertexList = (Vertex *) realloc(graph->vertices, 
                                         (sizeof(Vertex) * (*vertexListSize)));
      if (newVertexList == NULL)
         OutOfMemoryError("vertex list");
      graph->vertices = newVertexList;
   }

   // store information in vertex
   graph->vertices[numVertices].label = labelIndex;
   graph->vertices[numVertices].numEdges = 0;
   graph->vertices[numVertices].edges = NULL;
   graph->vertices[numVertices].map = VERTEX_UNMAPPED;
   graph->vertices[numVertices].used = FALSE;
   graph->vertices[numVertices].sourceVertex = sourceVertex;
   graph->vertices[numVertices].sourceExample = 0;   // will set later...

   // GUI coloring
   graph->vertices[numVertices].originalVertexIndex = numVertices;
   graph->vertices[numVertices].color = VERTEX_DEFAULT;
   graph->vertices[numVertices].anomalousValue = 2.0;

   graph->numVertices++;
}


//******************************************************************************
// NAME: AddEdge
//
// INPUTS: (Graph *graph) - graph to add edge to
//         (ULONG sourceVertexIndex) - index of edge's source vertex
//         (ULONG targetVertexIndex) - index of edge's target vertex
//         (BOOLEAN directed) - TRUE is edge is directed
//         (ULONG labelIndex) - index of edge's label in label list
//         (ULONG *edgeListSize) - pointer to size of graph's allocated
//                                 edge array
//         (ULONG spansIncrement)
//
// RETURN: (void)
//
// PURPOSE: Add edge information to graph. AddEdge also changes the
// size of the currently-allocated edge array, which increases by
// LIST_SIZE_INC (instead of just 1) when exceeded.
//******************************************************************************

void AddEdge(Graph *graph, ULONG sourceVertexIndex, ULONG targetVertexIndex,
             BOOLEAN directed, ULONG labelIndex, ULONG *edgeListSize,
             BOOLEAN spansIncrement)
{
   Edge *newEdgeList;

   // make sure there is enough room for another edge in the graph
   if (*edgeListSize == graph->numEdges) 
   {
      *edgeListSize += LIST_SIZE_INC;
      newEdgeList = (Edge *) realloc(graph->edges,
                                     (sizeof(Edge) * (*edgeListSize)));
      if (newEdgeList == NULL)
         OutOfMemoryError("AddEdge:newEdgeList");
      graph->edges = newEdgeList;
   }

   // add edge to graph
   graph->edges[graph->numEdges].vertex1 = sourceVertexIndex;
   graph->edges[graph->numEdges].vertex2 = targetVertexIndex;
   graph->edges[graph->numEdges].label = labelIndex;
   graph->edges[graph->numEdges].directed = directed;
   graph->edges[graph->numEdges].used = FALSE;
   graph->edges[graph->numEdges].spansIncrement = spansIncrement;
   graph->edges[graph->numEdges].validPath = TRUE;
   //
   // GBAD-P: Initialize anomalous flag and source vertices
   //
   graph->edges[graph->numEdges].anomalous = FALSE;
   graph->edges[graph->numEdges].sourceVertex1 = graph->vertices[sourceVertexIndex].sourceVertex;
   graph->edges[graph->numEdges].sourceVertex2 = graph->vertices[targetVertexIndex].sourceVertex;
   graph->edges[graph->numEdges].sourceExample = 0;   // will set later...
   //

   // add index to edge in edge index array of both vertices
   AddEdgeToVertices(graph, graph->numEdges);

   // GUI coloring
   graph->edges[graph->numEdges].originalEdgeIndex = graph->numEdges;
   graph->edges[graph->numEdges].color = EDGE_DEFAULT;
   graph->edges[graph->numEdges].anomalousValue = 2.0;

   graph->numEdges++;
}


//******************************************************************************
// NAME: StoreEdge
//
// INPUTS: (Edge *overlapEdges) - edge array where edge is stored
//         (ULONG edgeIndex) - index into edge array where edge is stored
//         (ULONG v1) - vertex1 of edge
//         (ULONG v2) - vertex2 of edge
//         (ULONG label) - edge label index
//         (BOOLEAN directed) - edge directedness
//         (BOOLEAN spansIncrement) - edge crossing increment boundary
//
// RETURN: (void)
//
// PURPOSE: Procedure to store an edge in given edge array.
//******************************************************************************

void StoreEdge(Edge *overlapEdges, ULONG edgeIndex,
               ULONG v1, ULONG v2, ULONG label, BOOLEAN directed, 
               BOOLEAN spansIncrement)
{
   overlapEdges[edgeIndex].vertex1 = v1;
   overlapEdges[edgeIndex].vertex2 = v2;
   overlapEdges[edgeIndex].label = label;
   overlapEdges[edgeIndex].directed = directed;
   overlapEdges[edgeIndex].used = FALSE;
   overlapEdges[edgeIndex].spansIncrement = spansIncrement;
}


//******************************************************************************
// NAME: AddEdgeToVertices
//
// INPUTS: (Graph *graph) - graph containing edge and vertices
//         (ULONG edgeIndex) - edge's index into graph edge array
//
// RETURN: (void)
//
// PURPOSE: Add edge index to the edge array of each of the two
// vertices involved in the edge.  If a self-edge, then only add once.
//******************************************************************************

void AddEdgeToVertices(Graph *graph, ULONG edgeIndex)
{
   ULONG v1, v2;
   Vertex *vertex;
   ULONG *edgeIndices;

   v1 = graph->edges[edgeIndex].vertex1;
   v2 = graph->edges[edgeIndex].vertex2;
   vertex = & graph->vertices[v1];
   edgeIndices = (ULONG *) realloc(vertex->edges,
                                   sizeof(ULONG) * (vertex->numEdges + 1));
   if (edgeIndices == NULL)
      OutOfMemoryError("AddEdgeToVertices:edgeIndices1");
   edgeIndices[vertex->numEdges] = edgeIndex;
   vertex->edges = edgeIndices;
   vertex->numEdges++;

   if (v1 != v2) 
   { // don't add a self edge twice
      vertex = & graph->vertices[v2];
      edgeIndices = (ULONG *) realloc(vertex->edges,
                                      sizeof(ULONG) * (vertex->numEdges + 1));
      if (edgeIndices == NULL)
         OutOfMemoryError("AddEdgeToVertices:edgeIndices2");
      edgeIndices[vertex->numEdges] = edgeIndex;
      vertex->edges = edgeIndices;
      vertex->numEdges++;
   }
}


//******************************************************************************
// NAME:    AllocateGraph
//
// INPUTS:  (ULONG v) - number of vertices in graph
//          (ULONG e) - number of edges in graph
//
// RETURN:  (Graph *) - pointer to newly-allocated graph
//
// PURPOSE: Allocate memory for new graph containing v vertices and e
// edges.
//******************************************************************************

Graph *AllocateGraph(ULONG v, ULONG e)
{
   Graph *graph;

   graph = (Graph *) malloc(sizeof(Graph));
   if (graph == NULL)
      OutOfMemoryError("AllocateGraph:graph");

   graph->numVertices = v;
   graph->numEdges = e;
   graph->vertices = NULL;
   graph->edges = NULL;
   if (v > 0) 
   {
      graph->vertices = (Vertex *) malloc(sizeof(Vertex) * v);
      if (graph->vertices == NULL)
         OutOfMemoryError("AllocateGraph:graph->vertices");
   }
   if (e > 0) 
   {
      graph->edges = (Edge *) malloc(sizeof(Edge) * e);
      if (graph->edges == NULL)
         OutOfMemoryError("AllocateGraph:graph->edges");
   }

   return graph;
}

//******************************************************************************
// NAME:    CopyGraph
//
// INPUTS:  (Graph *g) - graph to be copied
//
// RETURN:  (Graph *) - pointer to copy of graph
//
// PURPOSE: Create and return a copy of the given graph.
//******************************************************************************

Graph *CopyGraph(Graph *g)
{
   Graph *gCopy;
   ULONG nv;
   ULONG ne;
   ULONG v;
   ULONG e;
   ULONG numEdges;

   nv = g->numVertices;
   ne = g->numEdges;

   // allocate graph
   gCopy = AllocateGraph(nv, ne);

   // copy vertices; allocate and copy vertex edge arrays
   for (v = 0; v < nv; v++) 
   {
      gCopy->vertices[v].label = g->vertices[v].label;
      gCopy->vertices[v].map = g->vertices[v].map;
      gCopy->vertices[v].used = g->vertices[v].used;
      numEdges = g->vertices[v].numEdges;
      gCopy->vertices[v].numEdges = numEdges;
      gCopy->vertices[v].edges = NULL;
      gCopy->vertices[v].sourceVertex = g->vertices[v].sourceVertex;
      gCopy->vertices[v].sourceExample = g->vertices[v].sourceExample;
      if (numEdges > 0) 
      {
          gCopy->vertices[v].edges = (ULONG *) malloc(numEdges * sizeof(ULONG));
          if (gCopy->vertices[v].edges == NULL)
             OutOfMemoryError("CopyGraph:edges");
          for (e = 0; e < numEdges; e++)
             gCopy->vertices[v].edges[e] = g->vertices[v].edges[e];
      }
      // GUI coloring
      gCopy->vertices[v].originalVertexIndex = g->vertices[v].originalVertexIndex;
      gCopy->vertices[v].color = g->vertices[v].color;
      gCopy->vertices[v].anomalousValue = g->vertices[v].anomalousValue;
   }

   // copy edges
   for (e = 0; e < ne; e++) 
   {
      gCopy->edges[e].vertex1 = g->edges[e].vertex1;
      gCopy->edges[e].vertex2 = g->edges[e].vertex2;
      gCopy->edges[e].label = g->edges[e].label;
      gCopy->edges[e].directed = g->edges[e].directed;
      gCopy->edges[e].used = g->edges[e].used;
      gCopy->edges[e].sourceVertex1 = g->edges[e].sourceVertex1;
      gCopy->edges[e].sourceVertex2 = g->edges[e].sourceVertex2;
      gCopy->edges[e].sourceExample = g->edges[e].sourceExample;
      // GUI coloring
      gCopy->edges[e].originalEdgeIndex = g->edges[e].originalEdgeIndex;
      gCopy->edges[e].color = g->edges[e].color;
      gCopy->edges[e].anomalousValue = g->edges[e].anomalousValue;
   }

   return gCopy;
}


//******************************************************************************
// NAME:    FreeGraph
//
// INPUTS:  (Graph *graph) - graph to be freed
//
// RETURN:  void
//
// PURPOSE: Free memory used by given graph, including the vertices array
// and the edges array for each vertex.
//******************************************************************************

void FreeGraph(Graph *graph)
{
   ULONG v;

   if (graph != NULL) 
   {
      for (v = 0; v < graph->numVertices; v++)
         free(graph->vertices[v].edges);
      free(graph->edges);
      free(graph->vertices);
      free(graph);
   }
}


//******************************************************************************
// NAME:    PrintGraph
//
// INPUTS:  (Graph *graph) - graph to be printed
//          (LabelList *labelList) - indexed list of vertex and edge labels
//
// RETURN:  void
//
// PURPOSE: Print the vertices and edges of the graph to stdout.
//******************************************************************************

void PrintGraph(Graph *graph, LabelList *labelList)
{
   ULONG v;
   ULONG e;

   if (graph != NULL) 
   {
      printf("  Graph(%luv,%lue):\n", graph->numVertices, graph->numEdges);
      // print vertices
      for (v = 0; v < graph->numVertices; v++) 
      {
         printf("    ");
         PrintVertex(graph, v, labelList);
      }
      // print edges
      for (e = 0; e < graph->numEdges; e++) 
      {
         printf("    ");
         PrintEdge(graph, e, labelList);
      }
   }
}


//******************************************************************************
// NAME: PrintVertex
//
// INPUTS: (Graph *graph) - graph containing vertex
//         (ULONG vertexIndex) - index of vertex to print
//         (LabelList *labelList) - labels in graph
//
// RETURN: (void)
//
// PURPOSE: Print a vertex.
//******************************************************************************

void PrintVertex(Graph *graph, ULONG vertexIndex, LabelList *labelList)
{
   printf("v %lu ", vertexIndex + 1);
   PrintLabel(graph->vertices[vertexIndex].label, labelList);
   printf("\n");
}


//******************************************************************************
// NAME: PrintEdge
//
// INPUTS: (Graph *graph) - graph containing edge
//         (ULONG edgeIndex) - index of edge to print
//         (LabelList *labelList) - labels in graph
//
// RETURN: (void)
//
// PURPOSE: Print an edge.
//******************************************************************************

void PrintEdge(Graph *graph, ULONG edgeIndex, LabelList *labelList)
{
   Edge *edge = & graph->edges[edgeIndex];

   if (edge->directed)
      printf("d");
   else 
      printf("u");
   printf(" %lu %lu ", edge->vertex1 + 1, edge->vertex2 + 1);
   PrintLabel(edge->label, labelList);
   printf("\n");
}


//******************************************************************************
// NAME:    WriteGraphToFile
//
// INPUTS:  (FILE *outFile) - file stream to write graph
//          (Graph *graph) - graph to be written
//          (LabelList *labelList) - indexed list of vertex and edge labels
//          (ULONG vOffset) - vertex offset for compressed increments
//          (ULONG start) - beginning of vertex range to print
//          (ULONG finish) - end of vertex range to print
//          (BOOLEAN printPS) - flag indicating output is for predefined sub
//
// RETURN:  void
//
// PURPOSE: Write the vertices and edges of the graph to the given
//          file, prefaced by the SUB_TOKEN defined in gbad.h
//          (when WriteSubToken is TRUE).
//******************************************************************************

/*void PrintSubList(SubList *subList, Parameters *parameters)
{
   ULONG counter = 1;
   SubListNode *subListNode = NULL;

   if (subList != NULL) 
   {
      subListNode = subList->head;
      while (subListNode != NULL) 
      {
         printf("(%lu) ", counter);
         counter++;
         PrintSub(subListNode->sub, parameters);
         printf("\n");
         subListNode = subListNode->next;
      }
   }
}

void PrintSub(Substructure *sub, Parameters *parameters)
{
   // parameters used
   LabelList *labelList = parameters->labelList;
   ULONG outputLevel = parameters->outputLevel;

   if (sub != NULL) 
   {
      printf("Substructure: value = %.*g", NUMERIC_OUTPUT_PRECISION,
             sub->value);
      // print instance/example statistics if output level high enough
      if (outputLevel > 2) 
      {
         printf("\n                  pos instances = %lu",sub->numInstances);
         printf(", examples = %lu\n",sub->numExamples);
      } 
      else 
      {
         printf(", instances = %lu\n", sub->numInstances);
      }
      // print subgraph
      if (sub->definition != NULL) 
      {
         PrintGraph(sub->definition, labelList);
      }
      // print instances if output level high enough
      if (outputLevel > 2) 
      {
         printf("\n  Instances:\n");
         PrintPosInstanceList(sub, parameters);
      }
   }
}
*/

void WriteSubGraphToFile(FILE *outFile, SubList *subList, Parameters *parameters, BOOLEAN printPS)
{
   ULONG counter = 1;
   SubListNode *subListNode = NULL;

   if (subList != NULL) 
   {
      subListNode = subList->head;
      while (subListNode != NULL) 
      {
         //printf("(%lu) ", counter);
         //PrintSub(subListNode->sub, parameters);
         //printf("\n");
         Graph *graph = subListNode->sub->definition;
         LabelList *labelList = parameters->labelList;
         ULONG v;
         ULONG e;
         ULONG vOffset = 0;
         ULONG start = 0;
         ULONG finish = subListNode->sub->definition->numVertices;
         Edge *edge = NULL;

         if (graph != NULL) 
         {
            if (printPS)
               fprintf(outFile, "%s %lu\n", SUB_TOKEN, subListNode->sub->numInstances);
            // write vertices
            for (v = start; v < finish; v++)
            {
               fprintf(outFile, "v %lu ", (v + 1 + vOffset - start));
               WriteLabelToFile(outFile, graph->vertices[v].label, labelList, FALSE);
               fprintf(outFile, "\n");
            }
            // write edges
            for (e = 0; e < graph->numEdges; e++)
            {
               edge = &graph->edges[e];
               if ((edge->vertex1 >= start) && (edge->vertex1 < finish))
               {
                  if (edge->directed)
                     fprintf(outFile, "d");
                  else
                     fprintf(outFile, "u");
                  fprintf(outFile, " %lu %lu ",
                     (edge->vertex1 + 1 + vOffset - start),
                     (edge->vertex2 + 1 + vOffset - start));
                  WriteLabelToFile(outFile, edge->label, labelList, FALSE);
                  fprintf(outFile, "\n");
               }
            }
            if (printPS)
               fprintf(outFile, "\n");
            counter++;
         }
         subListNode = subListNode->next;
      }
   }
}


void WriteGraphToFile(FILE *outFile, Graph *graph, LabelList *labelList,
                      ULONG vOffset, ULONG start, ULONG finish, BOOLEAN printPS)
{
   ULONG v;
   ULONG e;
   Edge *edge = NULL;

   if (graph != NULL) 
   {
      if (printPS)
         fprintf(outFile, "%s\n", SUB_TOKEN);
      // write vertices
      for (v = start; v < finish; v++)
      {
         fprintf(outFile, "v %lu ", (v + 1 + vOffset - start));
         WriteLabelToFile(outFile, graph->vertices[v].label, labelList, FALSE);
         fprintf(outFile, "\n");
      }
      // write edges
      for (e = 0; e < graph->numEdges; e++)
      {
         edge = &graph->edges[e];
         if ((edge->vertex1 >= start) && (edge->vertex1 < finish))
         {
            if (edge->directed)
               fprintf(outFile, "d");
            else
               fprintf(outFile, "u");
            fprintf(outFile, " %lu %lu ",
               (edge->vertex1 + 1 + vOffset - start),
               (edge->vertex2 + 1 + vOffset - start));
            WriteLabelToFile(outFile, edge->label, labelList, FALSE);
            fprintf(outFile, "\n");
         }
      }
      if (printPS)
         fprintf(outFile, "\n");
   }
}

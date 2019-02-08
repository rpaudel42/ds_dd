//******************************************************************************
// gbad.h
//
// Data type and prototype definitions for the GBAD system.
//
// Date      Name       Description
// ========  =========  ========================================================
// 08/12/09  Eberle     Initial version, taken from SUBDUE 5.2.1
// 11/30/09  Eberle     Changed MATCH_SEARCH_THRESHOLD_EXPONENT to 3.0 and
//                      added parameters to FindAnomalousAncestors and
//                      ExtendAnomalousInstancesByEdge; added frequency and
//                      matched flag to instance structure
// 12/17/09  Graves     Added GUI coloring
// 11/13/12  Eberle     Added -optimize option; added function 
//                      ExtendPotentialInstancesByEdgeForMPS
// 06/15/14  Eberle     Modified ExtendPotentialInstancesByEdge.
// 01/02/15  Graves     Changed the return type of GP_read_graph to int.
//
//******************************************************************************

#ifndef GBAD_H
#define GBAD_H

#include <limits.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GBAD_VERSION "3.3"

// Substructure evaluation methods
#define EVAL_MDL      1
#define EVAL_SIZE     2
#define EVAL_SETCOVER 3

// Graph match search space limited to V^MATCH_SEARCH_THRESHOLD_EXPONENT
// If set to zero, then no limit
#define MATCH_SEARCH_THRESHOLD_EXPONENT 3.0

// Starting strings for input files
#define SUB_TOKEN        "S"  // new substructure
#define PREDEF_SUB_TOKEN "PS" // new predefined substructure
#define POS_EG_TOKEN     "XP" // new positive example

// Vertex and edge labels used for graph compression
#define SUB_LABEL_STRING     "SUB"
#define OVERLAP_LABEL_STRING "OVERLAP"
#define PREDEFINED_PREFIX    "PS"

#define POS 1

// Costs of various graph match transformations
#define INSERT_VERTEX_COST             1.0 // insert vertex
#define DELETE_VERTEX_COST             1.0 // delete vertex
#define SUBSTITUTE_VERTEX_LABEL_COST   1.0 // substitute vertex label
#define INSERT_EDGE_COST               1.0 // insert edge
#define INSERT_EDGE_WITH_VERTEX_COST   1.0 // insert edge with vertex
#define DELETE_EDGE_COST               1.0 // delete edge
#define DELETE_EDGE_WITH_VERTEX_COST   1.0 // delete edge with vertex
#define SUBSTITUTE_EDGE_LABEL_COST     1.0 // substitute edge label
#define SUBSTITUTE_EDGE_DIRECTION_COST 1.0 // change directedness of edge
#define REVERSE_EDGE_DIRECTION_COST    1.0 // change direction of directed edge

// Constants for graph matcher.  Special vertex mappings use the upper few
// unsigned long integers.  This assumes graphs will never have this many
// vertices, which is a pretty safe assumption.  The maximum double is used
// for initial costs.
#define MAX_UNSIGNED_LONG ULONG_MAX  // ULONG_MAX defined in limits.h
#define VERTEX_UNMAPPED   MAX_UNSIGNED_LONG
#define VERTEX_DELETED    MAX_UNSIGNED_LONG - 1
#define MAX_DOUBLE        DBL_MAX    // DBL_MAX from float.h

// Label types
#define STRING_LABEL  0
#define NUMERIC_LABEL 1

// General defines
#define LIST_SIZE_INC  100  // initial size and increment for realloc-ed lists
#define TOKEN_LEN     256  // maximum length of token from input graph file
#define FILE_NAME_LEN 512  // maximum length of file names
#define COMMENT       '%'  // comment character for input graph file
#define NUMERIC_OUTPUT_PRECISION 6
#define LOG_2 0.6931471805599452862 // log_e(2) pre-computed

#define SPACE ' '
#define TAB   '\t'
#define NEWLINE '\n'
#define DOUBLEQUOTE '\"'
#define CARRIAGERETURN '\r'

#define FALSE 0
#define TRUE  1

//******************************************************************************
// Type Definitions
//******************************************************************************

typedef unsigned char UCHAR;
typedef unsigned char BOOLEAN;
typedef unsigned long ULONG;

// GUI coloring
typedef enum COLOR
{
        BLACK,
        WHITE,
        BROWN,
        PURPLE,
        BLUE,
        GREEN,
        YELLOW,
        ORANGE,
        RED,
        NO_COLOR,
} COLOR;

#define VERTEX_DEFAULT WHITE
#define EDGE_DEFAULT WHITE
#define POSITIVE_NORM_VERTEX BLUE
#define POSITIVE_NORM_EDGE BLUE
#define POSITIVE_PARTIAL_ANOM_VERTEX ORANGE
#define POSITIVE_PARTIAL_ANOM_EDGE ORANGE
#define POSITIVE_ANOM_VERTEX RED
#define POSITIVE_ANOM_EDGE RED

// Label
typedef struct 
{
   UCHAR labelType;       // one of STRING_LABEL or NUMERIC_LABEL
   union 
   {
      char *stringLabel;
      double numericLabel;
   } labelValue;
   BOOLEAN used;          // flag used to mark labels at various times
} Label;

// Label list
typedef struct 
{
   ULONG size;      // Number of label slots currently allocated in array
   ULONG numLabels; // Number of actual labels stored in list
   Label *labels;   // Array of labels
} LabelList;

// Edge
typedef struct 
{
   ULONG   vertex1;  // source vertex index into vertices array
   ULONG   vertex2;  // target vertex index into vertices array
   ULONG   label;    // index into label list of edge's label
   BOOLEAN directed; // TRUE if edge is directed
   BOOLEAN used;     // flag for marking edge used at various times
                     //   used flag assumed FALSE, so always reset when done
   BOOLEAN spansIncrement;   // TRUE if edge crosses a previous increment
   BOOLEAN validPath;
   BOOLEAN anomalous; // flag indicating whether or not this vertex is an anomaly
                      // will be marked when compression takes place
   ULONG   sourceVertex1; // original source vertex ID
   ULONG   sourceVertex2; // original target vertex ID
   ULONG   sourceExample; // original example number

   ULONG originalEdgeIndex;  // index needed for coloring
   COLOR color;              // edge coloring
   double anomalousValue;    // anomalous value for appropriate coloring
} Edge;

// Vertex
typedef struct 
{
   ULONG label;    // index into label list of vertex's label
   ULONG numEdges; // number of edges defined using this vertex
   ULONG *edges;   // indices into edge array of edges using this vertex
   ULONG map;      // used to store mapping of this vertex to corresponding
                   //   vertex in another graph
   BOOLEAN used;   // flag for marking vertex used at various times
                   //   used flag assumed FALSE, so always reset when done
   BOOLEAN anomalous; // flag indicating whether or not this vertex is an anomaly
                      // will be marked when compression takes place
   ULONG   sourceVertex;  // original source vertex ID
   ULONG   sourceExample; // original example number

   ULONG originalVertexIndex;  // index needed for coloring
   COLOR color;                // vertex coloring
   double anomalousValue;      // anomalous value for appropriate coloring
} Vertex;

// Graph
typedef struct 
{
   ULONG  numVertices; // number of vertices in graph
   ULONG  numEdges;    // number of edges in graph
   Vertex *vertices;   // array of graph vertices
   Edge   *edges;      // array of graph edges
} Graph;

// VertexMap: vertex to vertex mapping for graph match search
typedef struct 
{
   ULONG v1;
   ULONG v2;
} VertexMap;

// Instance
typedef struct _instance
{
   ULONG numVertices;   // number of vertices in instance
   ULONG numEdges;      // number of edges in instance
   ULONG *vertices;     // ordered indices of instance's vertices in graph
   ULONG *edges;        // ordered indices of instance's edges in graph
   double minMatchCost; // lowest cost so far of matching this instance to
                        // a substructure
   ULONG newVertex;     // index into vertices array of newly added vertex
                        //    (0 means no new vertex added)
   ULONG newEdge;       // index into edges array of newly added edge
   ULONG refCount;      // counter of references to this instance; if zero,
                        //    then instance can be deallocated
   VertexMap *mapping;  // instance mapped to substructure definition
   ULONG mappingIndex1; // index of source vertex of latest mapping
   ULONG mappingIndex2; // index of target vertex of latest mapping
   BOOLEAN used;        // flag indicating instance already associated
                        // with a substructure (for the current iteration)
   struct _instance *parentInstance;  // pointer to parent instance
   double infoAnomalousValue;  // information theoretic anomalousness value
   double probAnomalousValue;  // probabilistic anomalousness value
   double mpsAnomalousValue;   // maximum partial substructure anomalousness value

   ULONG numAnomalousVertices; // number of anomalous vertices in instance
   ULONG numAnomalousEdges;    // number of anomalous edges in instance
   ULONG *anomalousVertices;   // indices of instance's vertices that are anomalous
   ULONG *anomalousEdges;      // indices of instance's edgs that are anomalous
   ULONG frequency;     // frequency of this type of instance
   BOOLEAN matched;     // flag to indicate if instance has already matched
} Instance;

// InstanceListNode: node in singly-linked list of instances
typedef struct _instance_list_node 
{
   Instance *instance;
   struct _instance_list_node *next;
} InstanceListNode;

// InstanceList: singly-linked list of instances
typedef struct 
{
   InstanceListNode *head;
} InstanceList;

// Substructure

typedef struct _substructure
{
   Graph  *definition;         // graph definition of substructure
   ULONG  numInstances;        // number of positive instances
   ULONG  numExamples;         // number of unique positive examples
   InstanceList *instances;    // instances in positive graph
   double value;               // value of substructure
   double posIncrementValue;   // DL/#Egs value of sub for positive increment
   ULONG  numParentInstances;  // number of positive parent instances
   InstanceList *parentInstances;  // instances in positive parent substructure
} Substructure;

// SubListNode: node in singly-linked list of substructures
typedef struct _sub_list_node 
{
   Substructure *sub;
   struct _sub_list_node *next;
} SubListNode;

// SubList: singly-linked list of substructures
typedef struct 
{
   SubListNode *head;
} SubList;

// MatchHeapNode: node in heap for graph match search queue
typedef struct 
{
   ULONG  depth; // depth of node in search space (number of vertices mapped)
   double cost;  // cost of mapping
   VertexMap *mapping;
} MatchHeapNode;

// MatchHeap: heap of match nodes
typedef struct 
{
   ULONG size;      // number of nodes allocated in memory
   ULONG numNodes;  // number of nodes in heap
   MatchHeapNode *nodes;
} MatchHeap;

// ReferenceEdge
typedef struct
{
   ULONG   vertex1;  // source vertex index into vertices array
   ULONG   vertex2;  // target vertex index into vertices array
   BOOLEAN spansIncrement; //true if edge spans to or from a previous increment
   ULONG   label;    // index into label list of edge's label
   BOOLEAN directed; // TRUE if edge is directed
   BOOLEAN used;     // flag for marking edge used at various times
                     //   used flag assumed FALSE, so always reset when done
   BOOLEAN failed;
   ULONG map;
} ReferenceEdge;

// ReferenceVertex
typedef struct
{
   ULONG label;    // index into label list of vertex's label
   ULONG numEdges; // number of edges defined using this vertex
   ULONG *edges;   // indices into edge array of edges using this vertex
   ULONG map;      // used to store mapping of this vertex to corresponding
                   // vertex in another graph
   BOOLEAN used;   // flag for marking vertex used at various times
                   // used flag assumed FALSE, so always reset when done
   BOOLEAN vertexValid;
} ReferenceVertex;

// ReferenceGraph
typedef struct
{
   ULONG  numVertices; // number of vertices in graph
   ULONG  numEdges;    // number of edges in graph
   ReferenceVertex *vertices;   // array of graph vertices
   ReferenceEdge   *edges;      // array of graph edges
} ReferenceGraph;

// ReferenceInstanceList
typedef struct _ref_inst_list_node
{
   InstanceList *instanceList;
   ReferenceGraph *refGraph;
   ULONG vertexListSize;
   ULONG edgeListSize;
   BOOLEAN firstPass;
   BOOLEAN doExtend;
   struct _ref_inst_list_node *next;
} RefInstanceListNode;

typedef struct
{
   RefInstanceListNode *head;
} RefInstanceList;

typedef struct _avltablenode
{
   struct avl_table *vertexTree;
   Substructure *sub;
   struct _avltablenode *next;
} AvlTableNode;

typedef struct
{
   AvlTableNode *head;
} AvlTreeList;

typedef struct
{
   AvlTreeList *avlTreeList;
} InstanceVertexList;

// Singly-connected linked list of subarrays for each increment
typedef struct _increment
{
   SubList *subList;           // List of subs for an increment
   ULONG incrementNum;         // Increment in which these substructures were 
                               // discovered
   ULONG numPosVertices;       // Number of pos vertices in this increment
   ULONG numPosEdges;          // Number of pos edges in this increment
   ULONG startPosVertexIndex;  // Index of the first vertex in this increment
   ULONG startPosEdgeIndex;    // Index of the first edge in this increment
   double numPosEgs;           // Number of pos examples in this increment
} Increment;

typedef struct _increment_list_node
{
   Increment *increment;
   struct _increment_list_node *next;
} IncrementListNode;

typedef struct
{
   IncrementListNode *head;
} IncrementList;

// Parameters: parameters used throughout GBAD system
typedef struct 
{
   char inputFileName[FILE_NAME_LEN];   // main input file
   char psInputFileName[FILE_NAME_LEN]; // predefined substructures input file
   char outFileName[FILE_NAME_LEN];     // file for machine-readable output
   Graph *posGraph;      // Graph of positive examples
   double posGraphDL;    // Description length of positive input graph
   ULONG numPosEgs;      // Number of positive examples
   ULONG *posEgsVertexIndices; // vertex indices of where positive egs begin
   LabelList *labelList; // List of unique labels in input graph(s)
   Graph **preSubs;      // Array of predefined substructure graphs
   ULONG numPreSubs;     // Number of predefined substructures read in
   BOOLEAN predefinedSubs; // TRUE is predefined substructures given
   BOOLEAN outputToFile; // TRUE if file given for machine-readable output
   BOOLEAN directed;     // If TRUE, 'e' edges treated as directed
   ULONG beamWidth;      // Limit on size of substructure queue (> 0)
   ULONG limit;          // Limit on number of substructures expanded (> 0)
   ULONG maxVertices;    // Maximum vertices in discovered substructures
   ULONG minVertices;    // Minimum vertices in discovered substructures
   ULONG numBestSubs;    // Limit on number of best substructures
                         //   returned (> 0)
   BOOLEAN valueBased;   // If TRUE, then queues are trimmed to contain
                         //   all substructures with the top beamWidth
                         //   values; otherwise, queues are trimmed to
                         //   contain only the top beamWidth substructures.
   BOOLEAN prune;        // If TRUE, then expanded substructures with lower
                         //   value than their parents are discarded.
   ULONG outputLevel;    // More screen (stdout) output as value increases
   BOOLEAN allowInstanceOverlap; // Default is FALSE; if TRUE, then instances
                                 // may overlap, but compression costlier
   ULONG evalMethod;     // One of EVAL_MDL (default), EVAL_SIZE or
                         //   EVAL_SETCOVER
   double threshold;     // Percentage of size by which an instance can differ
                         // from the substructure definition according to
                         // graph match transformation costs
   ULONG iterations;     // Number of GBAD iterations; if more than 1, then
                         // graph compressed with best sub between iterations
   double *log2Factorial;   // Cache array A[i] = lg(i!); grows as needed
   ULONG log2FactorialSize; // Size of log2Factorial array
   BOOLEAN compress;     // If TRUE, write compressed graph to file
   IncrementList *incrementList;   // Set of increments
   InstanceVertexList *vertexList; // List of avl trees containing
                                   // instance vertices
   ULONG posGraphVertexListSize;
   ULONG posGraphEdgeListSize;
   ULONG posGraphSize;
   BOOLEAN mdl;          // Anomaly Detection method: Information Theoretic
   BOOLEAN prob;         // Anomaly Detection method: Probabilistic
   BOOLEAN mps;          // Anomaly Detection method: Max Partial Substructure
   double mdlThreshold;  // Information Theoretic (MDL) threshold
   double mpsThreshold;  // Maximum Partial Substructure (MPS) threshold
   double maxAnomalousScore;  // Maximum allowed anomalous score (for output)
   double minAnomalousScore;  // Maximum allowed anomalous score (for output)
   ULONG currentIteration;  // current iteration
   BOOLEAN noAnomalyDetection; // flag is TRUE if none of the anomaly detection
                               // methods are chosen
   ULONG norm;           // chosen normative pattern (default is best sub)
   double similarity;    // "I only want substructures that are similar to the
                         //  best substructure up to X%"
   ULONG numPreviousInstances; // number of best substructure instances from 
                               // previous iteration (needed for current
                               // iteration)

   // GUI coloring
   char dotFileName[FILE_NAME_LEN];     // file name to write dot file into
   Graph *originalPosGraph;      // copy of posGraph
   LabelList *originalLabelList;  // copy of labelList
   BOOLEAN dotToFile;    // TRUE if file given for dot file output
   BOOLEAN optimize;     // user option to skip certain processing and assume 
                         // solution is near
} Parameters;


//******************************************************************************
// Function Prototypes
//******************************************************************************

// compress.c

Graph *CompressGraph(Graph *, InstanceList *, Parameters *);
//
// GBAD
//
void AddOverlapEdges(Graph *, Graph *, InstanceList *, ULONG, ULONG, ULONG,
                     Parameters *);
Edge *AddOverlapEdge(Edge *, ULONG *, ULONG, ULONG, ULONG, Parameters *);
Edge *AddDuplicateEdges(Edge *, ULONG *, Edge *, Graph *, ULONG, ULONG,
                     Parameters *);
ULONG NumOverlapEdges(Graph *, InstanceList *, Parameters *);
//
// GBAD
//
void CompressFinalGraphs(Substructure *, Parameters *, ULONG, BOOLEAN);
void CompressLabelListWithGraph(LabelList *, Graph *, Parameters *);
ULONG SizeOfCompressedGraph(Graph *, InstanceList *, Parameters *, ULONG);
void RemovePosEgsCovered(Substructure *, Parameters *);
void MarkExample(ULONG, ULONG, Graph *, BOOLEAN);
void CopyUnmarkedGraph(Graph *, Graph *, ULONG, Parameters *);
void CompressWithPredefinedSubs(Parameters *);
void WriteCompressedGraphToFile(Substructure *sub, Parameters *parameters,
                                ULONG iteration);
void WriteUpdatedGraphToFile(Substructure *sub, Parameters *parameters);
void WriteUpdatedIncToFile(Substructure *sub, Parameters *parameters);
void MarkPosEgsCovered(Substructure *, Parameters *);

// discover.c
void color_subs(Parameters *, SubList *);
SubList *DiscoverSubs(Parameters *, ULONG);     // GBAD-P  change in parameters
SubList *GetInitialSubs(Parameters *);
BOOLEAN SinglePreviousSub(Substructure *, Parameters *);

// dot.c
char *get_color(COLOR);
void WriteGraphToDotFile(char *, Parameters *);
void WriteGraphWithInstancesToDotFile(char *, Graph *, InstanceList *,
                                      Parameters *);
void WriteSubsToDotFile(char *, Graph **, ULONG, Parameters *);
void WriteVertexToDotFile(FILE *, ULONG, ULONG, Graph *, LabelList *, char *);
void WriteEdgeToDotFile(FILE *, ULONG, ULONG, Graph *, LabelList *, char *);

// evaluate.c

void EvaluateSub(Substructure *, Parameters *);
ULONG GraphSize(Graph *);
double MDL(Graph *, ULONG, Parameters *);
ULONG NumUniqueEdges(Graph *, ULONG);
ULONG MaxEdgesToSingleVertex(Graph *, ULONG);
double ExternalEdgeBits(Graph *, Graph *, ULONG);
double Log2Factorial(ULONG, Parameters *);
double Log2(ULONG);
ULONG PosExamplesCovered(Substructure *, Parameters *);
ULONG ExamplesCovered(InstanceList *, Graph *, ULONG, ULONG *, ULONG);

// extend.c

SubList *ExtendSub(Substructure *, Parameters *);
// GBAD-P  changed the following parameters
InstanceList *ExtendInstances(InstanceList *, Graph *, BOOLEAN, Parameters *);
Instance *CreateExtendedInstance(Instance *, ULONG, ULONG, Graph *, BOOLEAN);

Substructure *CreateSubFromInstance(Instance *, Graph *);
void AddPosInstancesToSub(Substructure *, Instance *, InstanceList *, 
                          Parameters *, ULONG);

// gbad.c

void PrintProbabilisticAnomalies(InstanceList *, Parameters *);
void RemoveSimilarSubstructures(SubList *, Parameters *);
void FlagAnomalousVerticesAndEdges(InstanceList *,Graph *,Substructure *,
                                   Parameters *);
void StoreAnomalousEdge(Edge *, ULONG, ULONG, ULONG, ULONG, BOOLEAN, BOOLEAN, 
                        BOOLEAN, ULONG, ULONG, ULONG);
void SetExampleNumber(SubList *, Parameters *);
void PrintAnomalousVertex(Graph *, ULONG, LabelList *, Instance *, 
                          Parameters *);
void PrintAnomalousEdge(Graph *, ULONG, LabelList *, Instance *, Parameters *);
void PrintAnomalousInstance(Instance *, Graph *, Parameters *);

InstanceList *FindAnomalousInstances(Substructure *, Graph *, Parameters *);
InstanceList *FindPotentialAnomalousAncestors(Substructure *, Graph *, Parameters *);
InstanceList *ExtendPotentialInstancesByEdge(InstanceList *, Graph *, Edge *,
                                             Graph *, Substructure *, Parameters *);
InstanceList *ExtendPotentialInstancesByEdgeForMPS(InstanceList *, Graph *, 
                                                   Edge *, Graph *, 
                                                   Substructure *, 
                                                   Parameters *);
void ScoreAndPrintAnomalousInstances(InstanceList *, Substructure *, 
                                     Parameters *);
void ScoreAndPrintAnomalousAncestors(InstanceList *, Substructure *, 
                                     Parameters *);
void GBAD_MDL(SubList *, Parameters *);
void GBAD_MPS(SubList *, Parameters *);
Substructure *GBAD_P(SubList *, ULONG, Parameters *);

// graphmatch.c

BOOLEAN GraphMatch(Graph *, Graph *, LabelList *, double, double *,
                   VertexMap *);
double InexactGraphMatch(Graph *, Graph *, LabelList *, double, VertexMap *);
void OrderVerticesByDegree(Graph *, ULONG *);
ULONG MaximumNodes(ULONG);
double DeletedEdgesCost(Graph *, Graph *, ULONG, ULONG, ULONG *, LabelList *);
double InsertedEdgesCost(Graph *, ULONG, ULONG *);
double InsertedVerticesCost(Graph *, ULONG *);
MatchHeap *AllocateMatchHeap(ULONG);
VertexMap *AllocateNewMapping(ULONG, VertexMap *, ULONG, ULONG);
void InsertMatchHeapNode(MatchHeapNode *, MatchHeap *);
void ExtractMatchHeapNode(MatchHeap *, MatchHeapNode *);
void HeapifyMatchHeap(MatchHeap *);
BOOLEAN MatchHeapEmpty(MatchHeap *);
void MergeMatchHeaps(MatchHeap *, MatchHeap *);
void CompressMatchHeap(MatchHeap *, ULONG);
void PrintMatchHeapNode(MatchHeapNode *);
void PrintMatchHeap(MatchHeap *);
void ClearMatchHeap(MatchHeap *);
void FreeMatchHeap(MatchHeap *);

// graphops.c

void ReadInputFile(Parameters *);
ULONG *AddVertexIndex(ULONG *, ULONG, ULONG);
void ReadPredefinedSubsFile(Parameters *);
void AddVertex(Graph *, ULONG, ULONG *, ULONG);
void AddEdge(Graph *, ULONG, ULONG, BOOLEAN, ULONG, ULONG *, BOOLEAN);
void StoreEdge(Edge *, ULONG, ULONG, ULONG, ULONG, BOOLEAN, BOOLEAN);
void AddEdgeToVertices(Graph *, ULONG);
Graph *AllocateGraph(ULONG, ULONG);
Graph *CopyGraph(Graph *);
void FreeGraph(Graph *);
void PrintGraph(Graph *, LabelList *);
void PrintVertex(Graph *, ULONG, LabelList *);
void PrintEdge(Graph *, ULONG, LabelList *);
void WriteGraphToFile(FILE *, Graph *, LabelList *, ULONG, ULONG, ULONG, BOOLEAN);
void WriteSubGraphToFile(FILE *, SubList *, Parameters *, BOOLEAN);


// labels.c

LabelList *AllocateLabelList(void);
ULONG StoreLabel(Label *, LabelList *);
ULONG GetLabelIndex(Label *, LabelList *);
ULONG SubLabelNumber(ULONG, LabelList *);
double LabelMatchFactor(ULONG, ULONG, LabelList *);
void PrintLabel(ULONG, LabelList *);
void PrintLabelList(LabelList *);
void FreeLabelList(LabelList *);
void WriteLabelToFile(FILE *, ULONG, LabelList *, BOOLEAN);

// sgiso.c

InstanceList *FindInstances(Graph *, Graph *, Parameters *);
InstanceList *FindSingleVertexInstances(Graph *, Vertex *, Parameters *);
InstanceList *ExtendInstancesByEdge(InstanceList *, Graph *, Edge *,
                                    Graph *, Parameters *);
BOOLEAN EdgesMatch(Graph *, Edge *, Graph *, Edge *, Parameters *);
InstanceList *FilterInstances(Graph *, InstanceList *, Graph *,
                              Parameters *);

// subops.c

Substructure *AllocateSub(void);
void FreeSub(Substructure *);
void PrintSub(Substructure *, Parameters *);
SubListNode *AllocateSubListNode(Substructure *);
void FreeSubListNode(SubListNode *);
SubList *AllocateSubList(void);
void SubListInsert(Substructure *, SubList *, ULONG, BOOLEAN, LabelList *);
BOOLEAN MemberOfSubList(Substructure *, SubList *, LabelList *);
void FreeSubList(SubList *);
void PrintSubList(SubList *, Parameters *);
void PrintNewBestSub(Substructure *, SubList *, Parameters *);
ULONG CountSubs(SubList *);
Instance *AllocateInstance(ULONG, ULONG);
void FreeInstance(Instance *);
void PrintInstance(Instance *, Graph *, LabelList *);
void PrintInstanceList(InstanceList *, Graph *, LabelList *);
void PrintPosInstanceList(Substructure *, Parameters *);
//
void MarkInstanceVertices(Instance *, Graph *, BOOLEAN);
void MarkInstanceEdges(Instance *, Graph *, BOOLEAN);
InstanceListNode *AllocateInstanceListNode(Instance *);
void FreeInstanceListNode(InstanceListNode *);
InstanceList *AllocateInstanceList(void);
void FreeInstanceList(InstanceList *);
ULONG InstanceExampleNumber(Instance *, ULONG *, ULONG);
ULONG CountInstances(InstanceList *);
void InstanceListInsert(Instance *, InstanceList *, BOOLEAN);
BOOLEAN MemberOfInstanceList(Instance *, InstanceList *);
BOOLEAN InstanceMatch(Instance *, Instance *);
BOOLEAN InstanceOverlap(Instance *, Instance *);
BOOLEAN InstanceListOverlap(Instance *, InstanceList *);
BOOLEAN InstancesOverlap(InstanceList *);
Graph *InstanceToGraph(Instance *, Graph *);
BOOLEAN InstanceContainsVertex(Instance *, ULONG);
void AddInstanceToInstance(Instance *, Instance *);
void AddEdgeToInstance(ULONG, Edge *, Instance *);
void UpdateMapping(Instance *, Instance *);

// utility.c

void OutOfMemoryError(char *);
void PrintBoolean(BOOLEAN);
Substructure * CopySub(Substructure *);


//******************************************************************************
// actions.c
typedef struct graph_info_t
{
   Graph *graph;                 // pointer to store graphs read from files
   LabelList *labelList;         // list of unique labels in input graph(s)
   
   Graph **preSubs;              // array of predefined substructure graphs
   ULONG numPreSubs;             // number of predefined substructures read in
   
   ULONG numPosEgs;              // number of positive examples
   ULONG *posEgsVertexIndices;   // vertex indices of where positive egs begin
                                 // used when reading XP graphs
   
   BOOLEAN directed;             // TRUE if edge is directed
   
   ULONG posGraphVertexListSize; //
   ULONG posGraphEdgeListSize;   //
   ULONG vertexOffset;           // offset to add to vertex numbers
   
   BOOLEAN xp_graph;             // TRUE if reading XP graphs, otherwise
                                 // FALSE if reading PS graphs
} Graph_Info;

int GP_read_graph(Graph_Info *, char *);

#endif

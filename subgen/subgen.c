/***********************************************************************
 * subgen.c                                                            *
 *                                                                     *
 * Usage: subgen file seed                                             *
 *                                                                     *
 * Substructure graph generator for use with the SUBDUE substructure   *
 * discovery system.  Reads in a file containing various parameters    *
 * and the substructure definition and produces two files: file.graph  *
 * and file.insts.  file.graph contains the graph in SUBDUE format.    *
 * file.insts contains the instances of the substructure in the graph  *
 * along with any deviations from the original substructure.  The seed *
 * argument is the seed to the random number generator.                *
 *                                                                     *
 ***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILENAMELEN 80
#define TOKENLEN    80
#define FALSE        0
#define TRUE         1

#define VERTEXLABEL  1  /* instance deviation types */
#define EDGELABEL    2
#define VERTEXDEL    3
#define EDGEDEL      4
#define VERTEXDEL2   5  /* secondary deletions */
#define EDGEDEL2     6


/************************* Type Definitions *************************/

typedef unsigned boolean;

typedef unsigned long ul;

struct VertexAssoc {   /* used for substructure to instance mappings */
  ul v1, v2;
  struct VertexAssoc *next;
};

struct Label {         /* stores a vertex or edge label and its probability */
  char name[TOKENLEN];
  float probability;
  struct Label *next;
};

struct Vertex {
  ul number;
  char label[TOKENLEN];
  boolean deleted;                /* true if vertex temporarily deleted   */
  ul connected;                   /* used to determine graph connectivity */
  struct InstanceList *instances; /* backpointer to instances containing  */
};                                /* this vertex.                         */

struct Edge {
  char label[TOKENLEN];
  ul vertex1, vertex2;            /* directed edge from vertex1 to vertex2 */
  boolean deleted;                /* true if edge temporarily deleted      */
  struct InstanceList *instances; /* backpointer to instances containing   */
};                                /* this edge.                            */

struct VertexList {
  struct Vertex *vertex;
  struct VertexList *next;
};

struct EdgeList {
  struct Edge *edge;
  struct EdgeList *next;
};

struct Graph {
  struct VertexList *vertices;
  struct EdgeList *edges;
};

struct Deviation {
  unsigned type;         /* type of deviation: VERTEXLABEL, EDGELABEL, */
  union {                /*   VERTEXDEL, EDGEDEL, VERTEXDEL2, EDGEDEL2 */
    struct Vertex *newv;
    struct Edge *newe;
  } new;
  union {
    struct Vertex *oldv;
    struct Edge *olde;
  } old;
};

struct DeviationList {
  struct Deviation *deviation;
  struct DeviationList *next;
};

struct Instance {
  ul number;                        /* instance reference number         */
  struct Graph *graph;
  boolean deleted;                  /* TRUE is instance to be removed    */
  struct DeviationList *deviations; /* deviations from main substructure */
};

struct InstanceList {
  struct Instance *instance;
  struct InstanceList *next;
};


/************************* Global Variables *************************/

ul Vertices;          /* number of vertices in final graph */
ul Edges;             /* number of edges in final graph */

ul *Map;              /* mapping from old to new vertex numbers */

ul Connectivity;      /* number of external connections for an instance  */
float Coverage;       /* percentage of final graph covered by instances  */
float Overlap;        /* percentage of structure overlap for an instance */

float DeviationAmt;   /* desired average amount of deviation per instance */

float WSubVertexLabel; /* weight of vertex label deviation */
float WSubEdgeLabel;   /* weight of edge label deviation */
float WDelVertex;      /* weight of deleted vertex deviation */
float WDelEdge;        /* weight of deleted edge deviation */
float WDelVertex2;     /* weight of internal vertex deletion deviation */
float WDelEdge2;       /* weight of internal edge deletion deviation */
float PSubVertexLabel; /* prob of vertex label deviation */
float PSubEdgeLabel;   /* prob of edge label deviation */
float PDelVertex;      /* prob of deleted vertex deviation */
float PDelEdge;        /* prob of deleted edge deviation */

struct Label *VertexLabels;      /* list of vertex labels and distribution */
struct Label *EdgeLabels;        /* list of edge labels and distribution */
struct Graph *Substructure;      /* substructure definition */
struct Graph *G;                 /* final graph to be built */
struct InstanceList *Instances;  /* list of instances of substructure */


/************************* Utilities *************************/

/*** Structure Allocation ***/

struct Vertex *allocate_vertex(number,label,connected,instances)
     ul number;
     char *label;
     boolean connected;
     struct InstanceList *instances;
{
  struct Vertex *v;
  if ((v = (struct Vertex *) malloc(sizeof(struct Vertex))) == NULL) {
    fprintf(stderr,"Unable to allocate vertex.\n");
    exit(1);
  }
  v->number = number;
  strcpy(v->label,label);
  v->deleted = FALSE;
  v->connected = connected;
  v->instances = instances;
  return(v);
}

struct Edge *allocate_edge(label,vertex1,vertex2,instances)
     char *label;
     ul vertex1, vertex2;
     struct InstanceList *instances;
{
  struct Edge *e;
  if ((e = (struct Edge *) malloc(sizeof(struct Edge))) == NULL) {
    fprintf(stderr,"Unable to allocate edge.\n");
    exit(1);
  }
  strcpy(e->label,label);
  e->vertex1 = vertex1;
  e->vertex2 = vertex2;
  e->deleted = FALSE;
  e->instances = instances;
  return(e);
}

struct DeviationList *allocate_deviation(dtype)
     unsigned dtype;
/* Returns a deviation list with an allocated deviation according
   to the given deviation type. */
{
  struct Deviation *d;
  struct DeviationList *dlptr;
  if ((d = (struct Deviation *) malloc(sizeof(struct Deviation)))
      == NULL) {
    fprintf(stderr,"Unable to allocate deviation.\n");
    exit(1);
  }
  d->type = dtype;
  if ((dtype == VERTEXLABEL) || (dtype == VERTEXDEL) ||
      (dtype == VERTEXDEL2)) {
    d->new.newv = NULL;
    d->old.oldv = NULL;
  }
  else {
    d->new.newe = NULL;
    d->old.olde = NULL;
  }
  if ((dlptr = (struct DeviationList *) malloc(sizeof(struct DeviationList)))
      == NULL) {
    fprintf(stderr,"Unable to allocate deviation list.\n");
    exit(1);
  }
  dlptr->deviation = d;
  dlptr->next = NULL;
  return(dlptr);
}

/*** Random Numbers ***/

long random_num(n)
     long n;
/* Returns a random integer between 0 and n - 1. */
{
  long rn;
  float maxrand;
  maxrand = 2147483648.0;  /* 2^31 */
  rn = (((float) rand()) / maxrand) * ((float) (n));
  return(rn);
}

/*** List Lengths ***/

ul n_vertices(g)
     struct Graph *g;
/* Return number of vertices in graph g. */
{
  ul size;
  struct VertexList *vlptr;
  size = 0;
  vlptr = g->vertices;
  while (vlptr != NULL) {
    size++;
    vlptr = vlptr->next;
  }
  return(size);
}

ul n_edges(g)
     struct Graph *g;
/* Return number of edges in graph g. */
{
  ul size;
  struct EdgeList *elptr;
  size = 0;
  elptr = g->edges;
  while (elptr != NULL) {
    size++;
    elptr = elptr->next;
  }
  return(size);
}

ul n_instances(ilist)
     struct InstanceList *ilist;
{
  ul size = 0;
  while (ilist != NULL) {
    size++;
    ilist = ilist->next;
  }
  return(size);
}

/*** List nth's ***/

struct Instance *nth_inst(n)
     ul n;
/* Returns nth instance, where n=0 corresponds to first instance. */
{
  struct InstanceList *ilptr;
  ilptr = Instances;
  while (n > 0) {
    ilptr = ilptr->next;
    n--;
  }
  return(ilptr->instance);
}

struct Vertex *nth_vertex(n,g)
     ul n;
     struct Graph *g;
/* Returns nth vertex in graph g, where n=0 corresponds to first vertex. */
{
  struct VertexList *vlptr;
  vlptr = g->vertices;
  while (n > 0) {
    vlptr = vlptr->next;
    n--;
  }
  return(vlptr->vertex);
}

struct Edge *nth_edge(n,g)
     ul n;
     struct Graph *g;
/* Returns nth edge in graph g, where n=0 corresponds to first edge. */
{
  struct EdgeList *elptr;
  elptr = g->edges;
  while (n > 0) {
    elptr = elptr->next;
    n--;
  }
  return(elptr->edge);
}

/*** Equality ***/

boolean vertex_equal(v1,v2)
     struct Vertex *v1, *v2;
/* Return true if vertices have same number. */
{
  if (v1 == v2)
    return(TRUE);
  else if ((v1->number == v2->number) &&
	   (strcmp(v1->label,v2->label) == 0))
    return(TRUE);
  else return(FALSE);
}

boolean edge_equal(e1,e2)
     struct Edge *e1, *e2;
/* Return true if edges have same vertices. */
{
  if (e1 == e2)
    return(TRUE);
  else if ((e1->vertex1 == e2->vertex1) &&
	   (e1->vertex2 == e2->vertex2) &&
	   (strcmp(e1->label,e2->label) == 0))
    return(TRUE);
  else return(FALSE);
}

/*** Finding Elements ***/

struct Vertex *find_vertex(v,g)
     ul v;
     struct Graph *g;
/* Return vertex structure corresponding to vertex number v in graph g. */
{
  struct VertexList *vlptr;
  vlptr = g->vertices;
  while ((vlptr != NULL) && (vlptr->vertex->number != v))
    vlptr = vlptr->next;
  return(vlptr->vertex);
}

/*** Membership ***/

boolean instance_member(inst,ilist)
     struct Instance *inst;
     struct InstanceList *ilist;
{
  while (ilist != NULL)
    if (ilist->instance == inst)
      return(TRUE);
    else ilist = ilist->next;
  return(FALSE);
}

boolean edge_member(e,g)
     struct Edge *e;
     struct Graph *g;
{
  struct EdgeList *elptr;
  elptr = g->edges;
  while (elptr != NULL)
    if (edge_equal(e,elptr->edge))
      return(TRUE);
  else elptr = elptr->next;
  return(FALSE);
}

boolean vertex_member(v,g)
     struct Vertex *v;
     struct Graph *g;
{
  struct VertexList *vlptr;
  vlptr = g->vertices;
  while (vlptr != NULL)
    if (vertex_equal(v,vlptr->vertex))
      return(TRUE);
  else vlptr = vlptr->next;
  return(FALSE);
}

/*** List Removal ***/

remove_vertex(v,g)
     struct Vertex *v;
     struct Graph *g;
/* Remove vertex from graph g's list of vertices. */
{
  struct VertexList *vlptr1, *vlptr2;
  if (vertex_equal(v,g->vertices->vertex))
      g->vertices = g->vertices->next;
  else {
    vlptr1 = g->vertices;
    vlptr2 = g->vertices->next;
    while (! vertex_equal(v,vlptr2->vertex)) {
      vlptr1 = vlptr2;
      vlptr2 = vlptr2->next;
    }
    vlptr1->next = vlptr2->next;
  }
}

remove_edge(e,g)
     struct Edge *e;
     struct Graph *g;
/* Remove edge from graph g's list of edges. */
{
  struct EdgeList *elptr1, *elptr2;
  if (edge_equal(e,g->edges->edge))
    g->edges = g->edges->next;
  else {
    elptr1 = g->edges;
    elptr2 = g->edges->next;
    while (! edge_equal(e,elptr2->edge)) {
      elptr1 = elptr2;
      elptr2 = elptr2->next;
    }
    elptr1->next = elptr2->next;
  }
}

vertex_remove_instance(v,inst)
     struct Vertex *v;
     struct Instance *inst;
/* Remove instance inst from vertex v's instance list. */
{
  struct InstanceList *ilptr1, *ilptr2;
  if (inst == v->instances->instance)
    v->instances = v->instances->next;
  else {
    ilptr1 = v->instances;
    ilptr2 = v->instances->next;
    while (inst != ilptr2->instance) {
      ilptr1 = ilptr2;
      ilptr2 = ilptr2->next;
    }
    ilptr1->next = ilptr2->next;
  }
}

edge_remove_instance(e,inst)
     struct Edge *e;
     struct Instance *inst;
/* Remove instance inst from edge e's instance list. */
{
  struct InstanceList *ilptr1, *ilptr2;
  if (inst == e->instances->instance)
    e->instances = e->instances->next;
  else {
    ilptr1 = e->instances;
    ilptr2 = e->instances->next;
    while (inst != ilptr2->instance) {
      ilptr1 = ilptr2;
      ilptr2 = ilptr2->next;
    }
    ilptr1->next = ilptr2->next;
  }
}


/************************* Read in parameters *************************/

ignore_line(fp)
     FILE *fp;
{
  int c;
  while ((c = getc(fp)) != EOF && c != '\n') ;
}

read_token(fp,s)
     FILE *fp;
     char *s;
/* Reads a token from stream fp into string s. */
{
  int c, index, intoken;
  index = 0;
  c = getc(fp); /* skip over whitespace and comments */
  while (c == ' ' || c == '\n' || c == '\t' || c == '%') {
    if (c == '%') ignore_line(fp);
    c = getc(fp);
  }
  while (c != EOF && c != ' ' && c != '\n' && c != '\t' && c != '%') {
    s[index++] = c;
    c = getc(fp);
  }
  if (c == '%') ignore_line(fp);
  s[index] = '\0';
}

float get_real(fp)
     FILE *fp;
{
  char s[TOKENLEN];
  float r;
  read_token(fp,s);
  if (strlen(s) == 0) {
    fprintf(stderr,"Unexpected end of file.\n");
    exit(1);
  }
  if (sscanf(s,"%f",&r) != 1) {
    fprintf(stderr,"Read error: %s should be a real number.\n",s);
    exit(1);
  }
  else return(r);
}

ul get_integer(fp)
     FILE *fp;
{
  char s[TOKENLEN];
  ul i;
  read_token(fp,s);
  if (strlen(s) == 0) {
    fprintf(stderr,"Unexpected end of file.\n");
    exit(1);
  }
  if (sscanf(s,"%lu",&i) != 1) {
    fprintf(stderr,"Read error: %s should be an unsigned integer.\n",s);
    exit(1);
  }
  else return(i);
}

float get_probability(fp)
     FILE *fp;
{
  float p;
  p = get_real(fp);
  if (p < 0.0 || p > 1.0) {
    fprintf(stderr,"Invalid probability %f.\n",p);
    exit(1);
  }
  else return(p);
}

struct Label *get_label(fp)
     FILE *fp;
/* Return pointer to a Label structure read from file. */
{
  char s[TOKENLEN];
  struct Label *lptr;
  lptr = NULL;
  read_token(fp,s);
  if (strcmp(s,"}") == 0)
    return(NULL);
  else {
    if ((lptr = (struct Label *) malloc(sizeof(struct Label))) == NULL) {
      fprintf(stderr,"Unable to allocate label.\n");
      exit(1);
    }
    lptr->next = NULL;
    strcpy(lptr->name,s);
    lptr->probability = get_probability(fp);
  }
  return(lptr);
}

struct Label *get_label_list(fp)
     FILE *fp;
/* Return pointer to a list of Label structures read from file. */
{
  char s[TOKENLEN];
  struct Label *lptr, *llptr;
  float sum;
  read_token(fp,s);
  if (strcmp(s,"{") != 0) {
    fprintf(stderr,"Expected { while reading labels.\n");
    exit(1);
  }
  sum = 0.0;
  llptr = NULL;
  lptr = get_label(fp);
  while (lptr != NULL) {
    lptr->next = llptr;
    llptr = lptr;
    sum = sum + lptr->probability;
    lptr = get_label(fp);
  }
  if (llptr == NULL) {
    fprintf(stderr,"Empty label list.\n");
    exit(1);
  }
/*  if (sum != 1.0) {
    fprintf(stderr,"Label probabilities must sum to one.\n");
    exit(1);
  } */
  return(llptr);
}

struct Graph *get_substructure(fp)
     FILE *fp;
{
  char s[TOKENLEN];
  struct Graph *sub;
  struct Vertex *v;
  struct Edge *e;
  struct VertexList *vlptr;
  struct EdgeList *elptr;
  if ((sub = (struct Graph *) malloc(sizeof(struct Graph))) == NULL) {
	fprintf(stderr,"Unable to allocate substructure.\n");
	exit(1);
      }
  sub->vertices = NULL;
  sub->edges = NULL;
  read_token(fp,s);
  if (strcmp(s,"{") != 0) {
    fprintf(stderr,"Expected { while reading substructure.\n");
    exit(1);
  }
  read_token(fp,s);
  while (strcmp(s,"}") != 0) {
    if (strcasecmp(s,"v") == 0) {      /* read a vertex */
      v = allocate_vertex(0,"",FALSE,NULL);
      v->number = get_integer(fp);
      read_token(fp,v->label);
      if ((vlptr = (struct VertexList *) malloc(sizeof(struct VertexList)))
	  == NULL) {
	fprintf(stderr,"Unable to allocate vertex list.\n");
	exit(1);
      }
      vlptr->vertex = v;
      vlptr->next = sub->vertices;
      sub->vertices = vlptr;
    }
    else if (strcasecmp(s,"e") == 0) { /* read an edge */
      e = allocate_edge("",0,0,NULL);
      read_token(fp,e->label);
      e->vertex1 = get_integer(fp);
      e->vertex2 = get_integer(fp);
      if ((elptr = (struct EdgeList *) malloc(sizeof(struct EdgeList)))
	  == NULL) {
	fprintf(stderr,"Unable to allocate edge list.\n");
	exit(1);
      }
      elptr->edge = e;
      elptr->next = sub->edges;
      sub->edges = elptr;
    }
    else {
      fprintf(stderr,"Unknown token read within substructure.\n");
      exit(1);
    }
    read_token(fp,s);
  }
  return(sub);
}
      
read_parameters(fp)
     FILE *fp;
{
  char s[TOKENLEN];
  float sum;
  Vertices = 0;              /* default parameter values */
  Edges = 0;
  Connectivity = 1;
  Coverage = 1.0;
  Overlap = 0.0;
  DeviationAmt = 0.0;
  WSubVertexLabel = 1.0;
  WSubEdgeLabel = 1.0;
  WDelVertex = 1.0;
  WDelEdge = 1.0;
  WDelVertex2 = 0.5;
  WDelEdge2 = 0.5;
  PSubVertexLabel = 0.25;
  PSubEdgeLabel = 0.25;
  PDelVertex = 0.10;
  PDelEdge = 0.4;
  VertexLabels = NULL;
  EdgeLabels = NULL;
  Substructure = NULL;
  read_token(fp,s);
  while (strlen(s) > 0) {
    if (strcasecmp(s,"Vertices") == 0)
      Vertices = get_integer(fp);
    else if (strcasecmp(s,"Edges") == 0)
      Edges = get_integer(fp);
    else if (strcasecmp(s,"Connectivity") == 0)
      Connectivity = get_integer(fp);
    else if (strcasecmp(s,"Coverage") == 0)
      Coverage = get_probability(fp);
    else if (strcasecmp(s,"Overlap") == 0)
      Overlap = get_probability(fp);
    else if (strcasecmp(s,"Deviation") == 0)
      DeviationAmt = get_real(fp);
    else if (strcasecmp(s,"SubVertexLabel") == 0) {
      WSubVertexLabel = get_real(fp);
      PSubVertexLabel = get_probability(fp);
    }
    else if (strcasecmp(s,"SubEdgeLabel") == 0) {
      WSubEdgeLabel = get_real(fp);
      PSubEdgeLabel = get_probability(fp);
    }
    else if (strcasecmp(s,"DelVertex") == 0) {
      WDelVertex = get_real(fp);
      PDelVertex = get_probability(fp);
    }
    else if (strcasecmp(s,"DelEdge") == 0) {
      WDelEdge = get_real(fp);
      PDelEdge = get_probability(fp);
    }
    else if (strcasecmp(s,"DelVertex2") == 0)
      WDelVertex2 = get_real(fp);
    else if (strcasecmp(s,"DelEdge2") == 0)
      WDelEdge2 = get_real(fp);
    else if (strcasecmp(s,"VertexLabels") == 0)
      VertexLabels = get_label_list(fp);
    else if (strcasecmp(s,"EdgeLabels") == 0)
      EdgeLabels = get_label_list(fp);
    else if (strcasecmp(s,"Substructure") == 0)
      Substructure = get_substructure(fp);
    else {
      fprintf(stderr,"Unknown token: %s",s);
      exit(1);
    }
    read_token(fp,s);
  }
  if (VertexLabels == NULL) {
    fprintf(stderr,"VertexLabels undefined.\n");
    exit(1);
  }
  if (EdgeLabels == NULL) {
    fprintf(stderr,"EdgeLabels undefined.\n");
    exit(1);
  }
  sum = PSubVertexLabel + PSubEdgeLabel + PDelVertex + PDelEdge;
  if (sum != 1.0) {
    fprintf(stderr,"Deviation probabilities do not sum to one.\n");
    exit(1);
  }
  if (Substructure == NULL) {
    fprintf(stderr,"Substructure undefined.\n");
    exit(1);
  }
  if (Vertices == 0) {
    fprintf(stderr,"Number of graph vertices undefined.\n");
    exit(1);
  }
  if (Edges == 0) {
    fprintf(stderr,"Number of graph edges undefined.\n");
    exit(1);
  }
}


/************************* Write parameters to file *************************/

write_labels(fp,labels)
     FILE *fp;
     struct Label *labels;
{
  while (labels != NULL) {
    fprintf(fp,"%s %f\n",labels->name,labels->probability);
    labels = labels->next;
  }
}

int lookup(num, nv)
   ul num, nv;
{
   ul i;

   for (i=0; i<nv; i++)
      if (Map[i] == num)
         return(i+1);
   return((ul) 0);
}


map_vertices(g, nv)
     struct Graph *g;
     ul nv;
{
  int i=0;
  struct VertexList *vl;
  struct EdgeList *el;

  if (g != NULL) {
     vl = g->vertices;
     while (vl != NULL) {
        Map[i] = vl->vertex->number;
        vl->vertex->number = i+1;
        vl = vl->next;
        i++;
     }
     el = g->edges;
     while (el != NULL) {
        el->edge->vertex1 = lookup(el->edge->vertex1, nv);
        el->edge->vertex2 = lookup(el->edge->vertex2, nv);
        el = el->next;
     }
  }
}


write_graph(fp,g)
     FILE *fp;
     struct Graph *g;
{
  struct VertexList *vl;
  struct EdgeList *el;
  if (g != NULL) {
    vl = g->vertices;
    while (vl != NULL) {
      fprintf(fp,"v %u %s\n",vl->vertex->number,vl->vertex->label);
      vl = vl->next;
    }
    el = g->edges;
    while (el != NULL) {
      fprintf(fp,"e %s %u %u\n",el->edge->label,el->edge->vertex1,
	      el->edge->vertex2);
      el = el->next;
    }
  }
}

write_parameters(fp)
     FILE *fp;
{
  fprintf(fp,"Vertices %u\n",Vertices);
  fprintf(fp,"Edges %u\n",Edges);
  fprintf(fp,"Connectivity %u\n",Connectivity);
  fprintf(fp,"Coverage %f\n",Coverage);
  fprintf(fp,"Overlap %f\n\n",Overlap);
  fprintf(fp,"Deviation %f\n",DeviationAmt);
  fprintf(fp,"SubVertexLabel %f %f\n",WSubVertexLabel,PSubVertexLabel);
  fprintf(fp,"SubEdgeLabel %f %f\n",WSubEdgeLabel,PSubEdgeLabel);
  fprintf(fp,"DelVertex %f %f\n",WDelVertex,PDelVertex);
  fprintf(fp,"DelEdge %f %f\n\n",WDelEdge,PDelEdge);
  fprintf(fp,"VertexLabels {\n");
  write_labels(fp,VertexLabels);
  fprintf(fp,"}\n\nEdgeLabels {\n");
  write_labels(fp,EdgeLabels);
  fprintf(fp,"}\n\nSubstructure {\n");
  write_graph(fp,Substructure);
  fprintf(fp,"}\n");
}


/************************* Write instances to file *************************/

write_deviation(fp,dptr)
     FILE *fp;
     struct Deviation *dptr;
{
  fprintf(fp,"%%   ");
  switch (dptr->type) {
  case VERTEXLABEL :
    fprintf(fp,"vertex label:  v %u %s-> v %u %s\n",
	    dptr->old.oldv->number,dptr->old.oldv->label,
	    dptr->new.newv->number,dptr->new.newv->label);
    break;
  case EDGELABEL :
    fprintf(fp,"edge label:    e %s %u %u -> e %s %u %u\n",
	    dptr->old.olde->label,dptr->old.olde->vertex1,
	    dptr->old.olde->vertex2,dptr->new.newe->label,
	    dptr->new.newe->vertex1,dptr->new.newe->vertex2);
    break;
  case VERTEXDEL :
    fprintf(fp,"delete vertex: v %u %s\n",
	    dptr->old.oldv->number,dptr->old.oldv->label);
    break;
  case EDGEDEL :
    fprintf(fp,"delete edge:   e %s %u %u\n",
	    dptr->old.olde->label,dptr->old.olde->vertex1,
	    dptr->old.olde->vertex2);
    break;
  case VERTEXDEL2 :
    fprintf(fp,"delete vertex: v %u %s  (secondary)\n",
	    dptr->old.oldv->number,dptr->old.oldv->label);
    break;
  case EDGEDEL2 :
    fprintf(fp,"delete edge:   e %s %u %u  (secondary)\n",
	    dptr->old.olde->label,dptr->old.olde->vertex1,
	    dptr->old.olde->vertex2);
    break;
  default :
    fprintf(stderr,"Unknown deviation type: %u", dptr->type);
    exit(1);
  }
}

write_instance(fp,iptr)
     FILE *fp;
     struct Instance *iptr;
{
  struct DeviationList *dlptr;
  fprintf(fp,"Instance %u {\n",iptr->number);
  dlptr = iptr->deviations;
  if (dlptr != NULL)
    fprintf(fp,"%% Deviations from substructure:\n%%\n");
  while (dlptr != NULL) {
    write_deviation(fp,dlptr->deviation);
    dlptr = dlptr->next;
  }
  write_graph(fp,iptr->graph);
  fprintf(fp,"}\n");
}

write_instances(fp)
     FILE *fp;
{
  struct InstanceList *ilptr;
  ilptr = Instances;
  while (ilptr != NULL) {
    write_instance(fp,ilptr->instance);
    if ((ilptr = ilptr->next) != NULL)
      fprintf(fp,"\n");
  }
}


/************************* Build graph *************************/
  
ul graph_size(g)
     struct Graph *g;
/* The size of a graph is the number of vertices and edges. */
{
  return(n_vertices(g) + n_edges(g));
}

propagate(v,g)
     struct Vertex *v;
     struct Graph *g;
/* Recursively propagates the connectivity of v through g. */
{
  struct EdgeList *elptr;
  struct Vertex *v2;
  elptr = g->edges;
  while (elptr != NULL) {
    if (! elptr->edge->deleted) {
      if (elptr->edge->vertex1 == v->number) {
	v2 = find_vertex(elptr->edge->vertex2,g);
	if ((! v2->deleted) && (v2->connected != v->connected)) {
	  v2->connected = v->connected;
	  propagate(v2,g);
	}
      }
      if (elptr->edge->vertex2 == v->number) {
	v2 = find_vertex(elptr->edge->vertex1,g);
	if ((! v2->deleted) && (v2->connected != v->connected)) {
	  v2->connected = v->connected;
	  propagate(v2,g);
	}
      }
    }
    elptr = elptr->next;
  }
}

boolean connected(g)
     struct Graph *g;
/* Returns true if graph is connected.  A null graph is considered
   disconnected. */
{
  struct VertexList *vlptr;
  vlptr = g->vertices;
  while (vlptr != NULL) {
    vlptr->vertex->connected = 0;
    vlptr = vlptr->next;
  }
  vlptr = g->vertices;
  while ((vlptr != NULL) && vlptr->vertex->deleted)
    vlptr = vlptr->next;
  if (vlptr != NULL) {
    vlptr->vertex->connected = 1;
    propagate(vlptr->vertex,g);
    vlptr = g->vertices;
    while (vlptr != NULL)
      if ((! vlptr->vertex->deleted) && (vlptr->vertex->connected != 1))
	return(FALSE);
      else vlptr = vlptr->next;
    return(TRUE);
  }
  return(FALSE);
}

char *random_label(oldlabel,labels)
     char *oldlabel;
     struct Label *labels;
{
  ul rn;
  float cumprob;
  char *rl;
  struct Label *lptr;

  rl = oldlabel;
  while (strcmp(oldlabel,rl) == 0) {
    lptr = labels;
    rn = random_num(10000);
    cumprob = 0.0;
    while (lptr != NULL) {
      cumprob = cumprob + lptr->probability;
      if (((float) rn) < (cumprob * 10000.0)) {
	rl = lptr->name;
	break;
      } else lptr = lptr->next;
    }
  }
  return(rl);
}

/*** Build Instances ***/

struct VertexAssoc *build_vertex_assoc(n)
     ul n;
/* Returns Substructure to n... vertex mapping. */
{
  struct VertexList *subv;
  struct VertexAssoc *vassoc, *vaptr;
  subv = Substructure->vertices;
  vassoc = NULL;
  while (subv != NULL) {
    if ((vaptr = (struct VertexAssoc *) malloc(sizeof(struct VertexAssoc)))
        == NULL) {
      fprintf(stderr,"Unable to allocate VertexAssoc.\n");
      exit(1);
    }
    vaptr->v1 = subv->vertex->number;
    vaptr->v2 = n++;
    vaptr->next = vassoc;
    vassoc = vaptr;
    subv = subv->next;
  }
  return(vassoc);
}

ul get_vertex_assoc(v,vaptr)
     ul v;
     struct VertexAssoc *vaptr;
{
  while (vaptr != NULL)
    if (vaptr->v1 == v)
      return(vaptr->v2);
    else vaptr = vaptr->next;
  return(0);
}

dispose_vertex_assoc(vaptr)
     struct VertexAssoc *vaptr;
{
  struct VertexAssoc *vaptr2;
  while (vaptr != NULL) {
    vaptr2 = vaptr->next;
    free(vaptr);
    vaptr = vaptr2;
  }
}

struct Vertex *copy_vertex(v,vassoc)
     struct Vertex *v;
     struct VertexAssoc *vassoc;
{
  struct Vertex *vcopy;
  vcopy = allocate_vertex(get_vertex_assoc(v->number,vassoc),
			  v->label,v->connected,NULL);
  return(vcopy);
}

struct Edge *copy_edge(e,vassoc)
     struct Edge *e;
     struct VertexAssoc *vassoc;
{
  struct Edge *ecopy;
  ecopy = allocate_edge(e->label,get_vertex_assoc(e->vertex1,vassoc),
			get_vertex_assoc(e->vertex2,vassoc),NULL);
  return(ecopy);
}

struct Graph *copy_graph(g,vassoc)
     struct Graph *g;
     struct VertexAssoc *vassoc;
/* Return a copy of graph g with vertex numbers changed according to
   the vertex association mapping in vassoc. */
{
  struct Graph *gcopy;
  struct VertexList *vl, *vlnew;
  struct EdgeList *el, *elnew;
  if ((gcopy = (struct Graph *) malloc(sizeof(struct Graph))) == NULL) {
    fprintf(stderr,"Unable to allocate graph for copy.\n");
    exit(1);
  }
  gcopy->vertices = NULL;
  gcopy->edges = NULL;
  vl = g->vertices;
  while (vl != NULL) {
    if ((vlnew = (struct VertexList *) malloc(sizeof(struct VertexList)))
	== NULL) {
      fprintf(stderr,"Unable to allocate vertex list for copy.\n");
      exit(1);
    }
    vlnew->vertex = copy_vertex(vl->vertex,vassoc);
    vlnew->next = gcopy->vertices;
    gcopy->vertices = vlnew;
    vl = vl->next;
  }
  el = g->edges;
  while (el != NULL) {
    if ((elnew = (struct EdgeList *) malloc(sizeof(struct EdgeList)))
	== NULL) {
      fprintf(stderr,"Unable to allocate edge list for copy.\n");
      exit(1);
    }
    elnew->edge = copy_edge(el->edge,vassoc);
    elnew->next = gcopy->edges;
    gcopy->edges = elnew;
    el = el->next;
  }
  return(gcopy);
}
  
ul compute_instances(subsize)
     ul subsize;
/* The number of instances is computed by seeing how many substructures
   the size of subsize can fit in the Coverage portion of the final
   graph.  The substructure size is reduced according to the amount
   of Overlap desired. */
{
  ul i;
  i = ((Vertices + Edges) * Coverage) / (subsize * (1.0 - Overlap));
  return(i + 1);
}

struct Instance *build_instance(vn)
     ul vn;
/* Return pointer to instance of Substructure whose vertex numbers
   begin at vn. */
{
  struct VertexAssoc *vassoc;
  struct Instance *iptr;
  struct InstanceList *ilptr;
  struct VertexList *vlptr;
  struct EdgeList *elptr;
  vassoc = build_vertex_assoc(vn);
  if ((iptr = (struct Instance *) malloc(sizeof(struct Instance))) == NULL) {
    fprintf(stderr,"Unable to allocate instance.\n");
    exit(1);
  }
  iptr->number = 0;
  iptr->deleted = FALSE;
  iptr->deviations = NULL;
  iptr->graph = copy_graph(Substructure,vassoc);
  /* point vertices and edges back to instance */
  vlptr = iptr->graph->vertices;
  while (vlptr != NULL) {
    if ((ilptr = (struct InstanceList *) malloc(sizeof(struct InstanceList)))
	== NULL) {
      fprintf(stderr,"Unable to allocate instance list.\n");
      exit(1);
    }
    ilptr->instance = iptr;
    ilptr->next = vlptr->vertex->instances;
    vlptr->vertex->instances = ilptr;
    vlptr = vlptr->next;
  }
  elptr = iptr->graph->edges;
  while (elptr != NULL) {
    if ((ilptr = (struct InstanceList *) malloc(sizeof(struct InstanceList)))
	== NULL) {
      fprintf(stderr,"Unable to allocate instance list.\n");
      exit(1);
    }
    ilptr->instance = iptr;
    ilptr->next = elptr->edge->instances;
    elptr->edge->instances = ilptr;
    elptr = elptr->next;
  }
  dispose_vertex_assoc(vassoc);
  return(iptr);
}

struct InstanceList *build_instances(n)
     ul n;
/* Return pointer to list of n unique instances of Substructure. */
{
  struct InstanceList *ilptr, *ilist;
  ul count, nv, vn;
  nv = n_vertices(Substructure);
  vn = 1;
  count = 0;
  ilist = NULL;
  while (count < n) {
    if ((ilptr = (struct InstanceList *) malloc(sizeof(struct InstanceList)))
	== NULL) {
      fprintf(stderr,"Unable to allocate instance list.\n");
      exit(1);
    }
    ilptr->instance = build_instance(vn);
    ilptr->instance->number = count + 1;
    ilptr->next = ilist;
    ilist = ilptr;
    vn = vn + nv;
    count++;
  }
  return(ilist);
}

/*** Merge Instances ***/

substitute_vertex(v1,v2)
     struct Vertex *v1, *v2;
/* Replace v1 by v2 in all of v1's instances, which are put on v2's
   instance list, if not already there. */
{
  struct VertexList *vlptr;
  struct EdgeList *elptr;
  struct InstanceList *ilptr;
  while (v1->instances != NULL) {
    vlptr = v1->instances->instance->graph->vertices;
    while (vlptr->vertex != v1)
      vlptr = vlptr->next;
    vlptr->vertex = v2;
    elptr = v1->instances->instance->graph->edges;
    while (elptr != NULL) {
      if (elptr->edge->vertex1 == v1->number)
	elptr->edge->vertex1 = v2->number;
      if (elptr->edge->vertex2 == v1->number)
	elptr->edge->vertex2 = v2->number;
      elptr = elptr->next;
    }
    ilptr = v1->instances;
    v1->instances = v1->instances->next;
    if (! instance_member(ilptr->instance,v2->instances)) {
      ilptr->next = v2->instances;
      v2->instances = ilptr;
    }
  }
  free(v1);
}

substitute_edge(e1,e2)
     struct Edge *e1, *e2;
/* Replace e1 by e2 in all of e1's instances, which are put on e2's
   instance list, if not already there. */
{
  struct EdgeList *elptr;
  struct InstanceList *ilptr;
  while (e1->instances != NULL) {
    elptr = e1->instances->instance->graph->edges;
    while (elptr->edge != e1)
      elptr = elptr->next;
    elptr->edge = e2;
    ilptr = e1->instances;
    e1->instances = e1->instances->next;
    if (! instance_member(ilptr->instance,e2->instances)) {
      ilptr->next = e2->instances;
      e2->instances = ilptr;
    }
  }
  free(e1);
}

ul change_vertex(v1,v2)
     struct Vertex *v1, *v2;
/* Change vertex v1 to vertex v2 and propagate change through edges
   of instances pointed to by the vertices.  If changing this vertex
   causes previously un-merged edges to become equal, then merge the
   edges as well, updating the returned amount of overlap appropriately. */
{
  ul amount = 1;
  struct InstanceList *ilptr1, *ilptr2;
  struct EdgeList *elptr1, *elptr2;
  substitute_vertex(v1,v2);
  /* check for edges that have become identical and merge them */
  ilptr1 = v2->instances;
  while (ilptr1 != NULL) {
    ilptr2 = ilptr1->next;
    while (ilptr2 != NULL) {
      elptr1 = ilptr1->instance->graph->edges;
      elptr2 = ilptr2->instance->graph->edges;
      while (elptr1 != NULL) {
	if ((elptr1->edge->vertex1 == v2->number) ||
	    (elptr1->edge->vertex2 == v2->number))
	  if ((elptr1->edge != elptr2->edge) &&
	      edge_equal(elptr1->edge,elptr2->edge)) {
#ifdef DEBUG
	    printf("  Merging edges I%u(e %s %u %u) -> I%u(e %s %u %u).\n",
		   ilptr1->instance->number,elptr1->edge->label,
		   elptr1->edge->vertex1,elptr1->edge->vertex2,
		   ilptr2->instance->number,elptr2->edge->label,
		   elptr2->edge->vertex1,elptr2->edge->vertex2);
#endif
	    substitute_edge(elptr1->edge,elptr2->edge);
	    amount++;
	  }
	elptr1 = elptr1->next;
	elptr2 = elptr2->next;
      }
      ilptr2 = ilptr2->next;
    }
    ilptr1 = ilptr1->next;
  }
  return(amount);
}

ul change_edge(e1,e2,i1,i2)
     struct Edge *e1, *e2;
     struct Instance *i1, *i2;
/* Change edge e1 in instance i1 to edge e2 in instance i2 and propagate
   vertex changes. */
{
  ul amount = 0;
  struct Vertex *v1, *v2;
  if (e1->vertex1 != e2->vertex1) {
    v1 = find_vertex(e1->vertex1,i1->graph);
    v2 = find_vertex(e2->vertex1,i2->graph);
#ifdef DEBUG
    printf("  Changing (v %u %s) -> (v %u %s).\n",
	   v1->number,v1->label,v2->number,v2->label);
#endif
    amount = amount + change_vertex(v1,v2);
  }
  if (e1->vertex2 != e2->vertex2) {
    v1 = find_vertex(e1->vertex2,i1->graph);
    v2 = find_vertex(e2->vertex2,i2->graph);
#ifdef DEBUG
    printf("  Changing (v %u %s) -> (v %u %s).\n",
	   v1->number,v1->label,v2->number,v2->label);
#endif
    amount = amount + change_vertex(v1,v2);
  }
  return(amount);
}

ul merge(i1,i2)
     struct Instance *i1, *i2;
/* Merge either a vertex or edge between the two instances.  Return
   the amount of resulting overlapping structure. */
{
  ul amount, n, r;
  struct Vertex *v1, *v2;
  struct Edge *e1, *e2;
  amount = 0;
#ifdef DEBUG
  printf("\nAttempting to merge following two instances:\n");
  write_instance(stdout,i1);
  write_instance(stdout,i2);
#endif
  if (random_num(100) < 50) {
    /* merge vertex */
    n = n_vertices(i1->graph);
    r = random_num(n);
    v1 = nth_vertex(r,i1->graph);
    v2 = nth_vertex(r,i2->graph);
    if (! vertex_equal(v1,v2)) {
#ifdef DEBUG
      printf("Merging vertices (v %u %s) -> (v %u %s).\n",
	     v1->number,v1->label,v2->number,v2->label);
#endif
      amount = change_vertex(v1,v2);
#ifdef DEBUG
      printf("Done merging vertices, overlap amount = %u.\n",amount);
#endif
    }
#ifdef DEBUG
    else printf("Failed to merge (v %u %s) -> (v %u %s).\n",
		v1->number,v1->label,v2->number,v2->label);
#endif
  }
  else {
    /* merge edge */
    n = n_edges(i1->graph);
    r = random_num(n);
    e1 = nth_edge(r,i1->graph);
    e2 = nth_edge(r,i2->graph);
    if (! edge_equal(e1,e2)) {
#ifdef DEBUG
      printf("Merging edges (e %s %u %u) -> (e %s %u %u).\n",
	     e1->label,e1->vertex1,e1->vertex2,
	     e2->label,e2->vertex1,e2->vertex2);
#endif
      amount = change_edge(e1,e2,i1,i2);
#ifdef DEBUG
      printf("Done merging edges, overlap amount = %u.\n",amount);
#endif
    }
#ifdef DEBUG
    else printf("Failed to merge (e %s %u %u) -> (e %s %u %u).\n",
		e1->label,e1->vertex1,e1->vertex2,
		e2->label,e2->vertex1,e2->vertex2);
#endif
  }
  return(amount);
}

merge_instances(olap,ninsts)
     ul olap, ninsts;
/* Randomly merge instances until given overlap achieved. */
{
  ul r1, r2, amt_ovr;
  int overlap;
  struct Instance *i1, *i2;
  overlap = olap;
  while (overlap > 0) {
    r1 = random_num(ninsts);
    r2 = random_num(ninsts);
    while (r1 == r2)
      r2 = random_num(ninsts);
    i1 = nth_inst(r1);
    i2 = nth_inst(r2);
    amt_ovr = merge(i1,i2);
    overlap = overlap - amt_ovr;
    }
}

/*** Fuzz Instances ***/

boolean deviated_vertex(v,inst)
     struct Vertex *v;
     struct Instance *inst;
/* Returns TRUE if vertex v's label has been changed. */
{
  struct DeviationList *dlptr;
  dlptr = inst->deviations;
  while (dlptr != NULL) {
    if (dlptr->deviation->type == VERTEXLABEL)
      if (vertex_equal(v,dlptr->deviation->new.newv))
	return(TRUE);
    dlptr = dlptr->next;
  }
  return(FALSE);
}

boolean deviated_edge(e,inst)
     struct Edge *e;
     struct Instance *inst;
/* Returns TRUE if edge e's label has been changed. */
{
  struct DeviationList *dlptr;
  dlptr = inst->deviations;
  while (dlptr != NULL) {
    if (dlptr->deviation->type == EDGELABEL)
      if (edge_equal(e,dlptr->deviation->new.newe))
	return(TRUE);
    dlptr = dlptr->next;
  }
  return(FALSE);
}

boolean vertex_in_edges(vn,g)
     ul vn;
     struct Graph *g;
/* Returns TRUE if vertex number vn occurs in any non-deleted edge of the
   given graph g. */
{
  struct EdgeList *elptr;
  elptr = g->edges;
  while (elptr != NULL) {
    if (! elptr->edge->deleted)
      if ((elptr->edge->vertex1 == vn) || (elptr->edge->vertex2 == vn))
	return(TRUE);
    elptr = elptr->next;
  }
  return(FALSE);
}

edge_remove_deleted_instances(e)
     struct Edge *e;
/* Removes instances marked for deletion from given edge e's instance list. */
{
  struct InstanceList *ilptr1, *ilptr2;
  while ((e->instances != NULL) && (e->instances->instance->deleted))
    e->instances = e->instances->next;
  ilptr1 = e->instances;
  if (ilptr1 != NULL) {
    ilptr2 = ilptr1->next;
    while (ilptr2 != NULL) {
      if (ilptr2->instance->deleted) {
	ilptr2->instance->deleted = FALSE;
	ilptr1->next = ilptr2->next;
      }
      else ilptr1 = ilptr2;
      ilptr2 = ilptr2->next;
    }
  }
}

vertex_remove_deleted_instances(v)
     struct Edge *v;
/* Removes instances marked for deletion from given vertex v's instance
   list.  Deletedness of instance reset to FALSE for next deviation. */
{
  struct InstanceList *ilptr1, *ilptr2;
  while ((v->instances != NULL) && (v->instances->instance->deleted)) {
    v->instances->instance->deleted = FALSE;
    v->instances = v->instances->next;
  }
  ilptr1 = v->instances;
  if (ilptr1 != NULL) {
    ilptr2 = ilptr1->next;
    while (ilptr2 != NULL) {
      if (ilptr2->instance->deleted) {
	ilptr2->instance->deleted = FALSE;
	ilptr1->next = ilptr2->next;
      }
      else ilptr1 = ilptr2;
      ilptr2 = ilptr2->next;
    }
  }
}

float change_vertex_label(inst)
     struct Instance *inst;
{
  ul rn;
  float weight;
  struct Vertex *v, *oldv;
  char *rl;
  struct Deviation *d;
  struct DeviationList *dlptr;
  struct InstanceList *ilptr;

  weight = 0.0;
  rn = random_num(n_vertices(inst->graph));
  v = nth_vertex(rn,inst->graph);
#ifdef DEBUG
  printf("  Attempting change on vertex (v %u %s).\n",v->number,v->label);
#endif
  if (! deviated_vertex(v,inst)) {
    rl = random_label(v->label,VertexLabels);
    oldv = allocate_vertex(v->number,v->label,v->connected,v->instances);
    strcpy(v->label,rl);
    if ((d = (struct Deviation *) malloc(sizeof(struct Deviation))) == NULL) {
      fprintf(stderr,"Unable to allocate deviation.\n");
      exit(1);
    }
    d->type = VERTEXLABEL;
    d->new.newv = v;
    d->old.oldv = oldv;
    ilptr = v->instances;
    while (ilptr != NULL) {
      if ((dlptr = (struct DeviationList *) malloc(sizeof(struct DeviationList))) == NULL) {
	fprintf(stderr,"Unable to allocate deviation list.\n");
	exit(1);
      }
      dlptr->deviation = d;
      dlptr->next = ilptr->instance->deviations;
      ilptr->instance->deviations = dlptr;
      weight = weight + WSubVertexLabel;
      ilptr = ilptr->next;
    }
#ifdef DEBUG
    printf("  Changed vertex label (v %u %s) -> (v %u %s).\n",
	   oldv->number,oldv->label,v->number,v->label);
#endif
  }
#ifdef DEBUG
  else printf(" Failed due to previously deviated vertex (v %u %s).\n",
	      v->number,v->label);
#endif
  return(weight);
}

float change_edge_label(inst)
     struct Instance *inst;
{
  ul rn;
  struct Edge *e, *olde;
  char *rl;
  struct Deviation *d;
  struct InstanceList *ilptr;
  struct DeviationList *dlptr;
  float weight;

  weight = 0.0;
  rn = random_num(n_edges(inst->graph));
  e = nth_edge(rn,inst->graph);
#ifdef DEBUG
  printf("  Attempting change on edge (e %s %u %u).\n",
	 e->label,e->vertex1,e->vertex2);
#endif
  if (! deviated_edge(e,inst)) {
    rl = random_label(e->label,EdgeLabels);
    olde = allocate_edge(rl,e->vertex1,e->vertex2,e->instances);
    if (! edge_member(olde,inst->graph)) {
      strcpy(olde->label,e->label);
      strcpy(e->label,rl);
      if ((d = (struct Deviation *) malloc(sizeof(struct Deviation)))
	  == NULL) {
	fprintf(stderr,"Unable to allocate deviation.\n");
	exit(1);
      }
      d->type = EDGELABEL;
      d->new.newe = e;
      d->old.olde = olde;
      ilptr = e->instances;
      while (ilptr != NULL) {
	if ((dlptr = (struct DeviationList *) malloc(sizeof(struct DeviationList))) == NULL) {
	  fprintf(stderr,"Unable to allocate deviation list.\n");
	  exit(1);
	}
	dlptr->deviation = d;
	dlptr->next = ilptr->instance->deviations;
	ilptr->instance->deviations = dlptr;
	weight = weight + WSubEdgeLabel;
	ilptr = ilptr->next;
      }
#ifdef DEBUG
      printf("  Changed edge label (e %s %u %u) -> (e %s %u %u).\n",
	     olde->label,olde->vertex1,olde->vertex2,
	     e->label,e->vertex1,e->vertex2);
#endif
    }
    else {
#ifdef DEBUG
      printf("  Failed due to duplicate existing edge (e %s %u %u).\n",
	     olde->label,olde->vertex1,olde->vertex2);
#endif
      olde->instances = NULL;
      free(olde);
    }
  }
#ifdef DEBUG
  else printf("  Failed due to previously deviated edge (e %s %u %u).\n",
	      e->label,e->vertex1,e->vertex2);
#endif
  return(weight);
}

float delete_vertex(inst)
     struct Instance *inst;
/* Returns the total deviation weight from deleting a random vertex
   from the given instance.  Deleting a vertex may also involve deleting
   incident edges and their lone vertices.  The actual deletion takes
   place only if the above modifications to the instance graph do not
   disconnect the graph.  Vertices contained in multiple instances are
   deleted (if possible) from every such instance. */
{
  ul rn, vn;
  float weight, weight2;
  struct Vertex *v, *v2;
  struct InstanceList *ilptr;
  struct VertexList *vlptr;
  struct EdgeList *elptr;
  struct DeviationList *dlptr;

  weight = 0.0;
  /* select random vertex */
  rn = random_num(n_vertices(inst->graph));
  v = nth_vertex(rn,inst->graph);
#ifdef DEBUG
  printf("  Attempting deletion on vertex (v %u %s).\n",
	 v->number,v->label);
#endif
  /* if vertex label previously changed, then abort deletion */
  if (! deviated_vertex(v,inst)) {
    /* try to delete vertex in each instance containing vertex */
    ilptr = v->instances;
    while (ilptr != NULL) {
#ifdef DEBUG
      printf("  Processing instance %u:\n",ilptr->instance->number);
#endif
      weight2 = WDelVertex;
      v->deleted = TRUE; /* temporary deletion for now */
      /* delete edges using vertex */
      elptr = ilptr->instance->graph->edges;
      while (elptr != NULL) {
	if (! elptr->edge->deleted)
	  if ((elptr->edge->vertex1 == v->number) ||
	      (elptr->edge->vertex2 == v->number)) {
	    elptr->edge->deleted = TRUE;
	    weight2 = weight2 + WDelEdge2;
	    if (elptr->edge->vertex1 == v->number)
	      vn = elptr->edge->vertex2;
	    else vn = elptr->edge->vertex1;
	    /* delete lone vertices on secondary edge deletions */
	    if (! vertex_in_edges(vn,ilptr->instance->graph)) {
	      v2 = find_vertex(vn,ilptr->instance->graph);
	      if (! v2->deleted) {
		v2->deleted = TRUE;
		weight2 = weight2 + WDelVertex2;
	      }
	    }
	  }
	elptr = elptr->next;
      }
      /* check resulting instance graph for disconnection */
      if (connected(ilptr->instance->graph)) {
	/* really remove secondary vertices and instance from vertices' list */
	vlptr = ilptr->instance->graph->vertices;
	while (vlptr != NULL) {
	  if ((vlptr->vertex->deleted) &&
	      (! vertex_equal(v,vlptr->vertex))) {
	    remove_vertex(vlptr->vertex,ilptr->instance->graph);
	    vertex_remove_instance(vlptr->vertex,ilptr->instance);
	    dlptr = allocate_deviation(VERTEXDEL2);
	    dlptr->deviation->old.oldv = vlptr->vertex;
	    dlptr->next = ilptr->instance->deviations;
	    ilptr->instance->deviations = dlptr;
	    vlptr->vertex->deleted = FALSE;
#ifdef DEBUG
	    printf("    Secondary deletion of vertex (v %u %s).\n",
		   vlptr->vertex->number,vlptr->vertex->label);
#endif
	  }
	  vlptr = vlptr->next;
	}
	/* really remove secondary edges and instance from edges' list */
	elptr = ilptr->instance->graph->edges;
	while (elptr != NULL) {
	  if (elptr->edge->deleted) {
	    remove_edge(elptr->edge,ilptr->instance->graph);
	    edge_remove_instance(elptr->edge,ilptr->instance);
	    dlptr = allocate_deviation(EDGEDEL2);
	    dlptr->deviation->old.olde = elptr->edge;
	    dlptr->next = ilptr->instance->deviations;
	    ilptr->instance->deviations = dlptr;
	    elptr->edge->deleted = FALSE;
#ifdef DEBUG
	    printf("    Secondary deletion of edge (e %s %u %u).\n",
		   elptr->edge->label,elptr->edge->vertex1,
		   elptr->edge->vertex2);
#endif
	  }
	  elptr = elptr->next;
	}
	/* remove selected vertex from graph */
	remove_vertex(v,ilptr->instance->graph);
	ilptr->instance->deleted = TRUE;
	dlptr = allocate_deviation(VERTEXDEL);
	dlptr->deviation->old.oldv = v;
	dlptr->next = ilptr->instance->deviations;
	ilptr->instance->deviations = dlptr;
	v->deleted = FALSE;
#ifdef DEBUG
	printf("    Deleted vertex (v %u %s).\n",v->number,v->label);
#endif
	weight = weight + weight2;
      } else {
#ifdef DEBUG
	printf("    Failed due to disconnection of instance graph.\n");
#endif
	/* reset vertices and edges deleted flag for next instance */
	vlptr = ilptr->instance->graph->vertices;
	while (vlptr != NULL) {
	  vlptr->vertex->deleted = FALSE;
	  vlptr = vlptr->next;
	}
	elptr = ilptr->instance->graph->edges;
	while (elptr != NULL) {
	  elptr->edge->deleted = FALSE;
	  elptr = elptr->next;
	}
      }
      ilptr = ilptr->next;
    }
    /* instance removal postponed since we're using this instance list
       in the above while loop */
    vertex_remove_deleted_instances(v);
  }
#ifdef DEBUG
  else printf("  Failed due to previously deviated vertex (v %u %s).\n",
	      v->number,v->label);
#endif
  return(weight);
}

float delete_edge(inst)
     struct Instance *inst;
/* Returns the total deviation weight from deleting a random edge from
   the given instance.  Deleting an edge may also involve deleting
   vertices whose only edge is the one being deleted.  The actual
   deletion takes place only if the above modifications to the instance
   graph do not disconnect the graph.  Edges contained in multiple
   instances are deleted (if possible) from every such instance. */
{
  ul rn;
  float weight, weight2;
  struct Edge *e;
  struct Vertex *v1, *v2;
  struct InstanceList *ilptr;
  struct DeviationList *dlptr;

  weight = 0.0;
  /* select random edge */
  rn = random_num(n_edges(inst->graph));
  e = nth_edge(rn,inst->graph);
#ifdef DEBUG
  printf("  Attempting deletion on edge (e %s %u %u).\n",
	 e->label,e->vertex1,e->vertex2);
#endif
  /* if edge label previously changed, then abort deletion */
  if (! deviated_edge(e,inst)) {
    /* try to delete edge in each instance containing edge */
    ilptr = e->instances;
    while (ilptr != NULL) {
#ifdef DEBUG
      printf("  Processing instance %u:\n",ilptr->instance->number);
#endif
      weight2 = WDelEdge;
      e->deleted = TRUE; /* temporary deletion for now */
      /* check edge vertices for possible deletion */
      v1 = NULL;
      if (! vertex_in_edges(e->vertex1,ilptr->instance->graph)) {
	v1 = find_vertex(e->vertex1,ilptr->instance->graph);
	v1->deleted = TRUE;
	weight2 = weight2 + WDelVertex2;
      }
      v2 = NULL;
      if (! vertex_in_edges(e->vertex2,ilptr->instance->graph)) {
	v2 = find_vertex(e->vertex2,ilptr->instance->graph);
	v2->deleted = TRUE;
	weight2 = weight2 + WDelVertex2;
      }
      /* check resulting instance graph for disconnection */
      if (connected(ilptr->instance->graph)) {
	/* really remove vertices and instance from vertices' list */
	if (v1 != NULL) {
	  remove_vertex(v1,ilptr->instance->graph);
	  vertex_remove_instance(v1,ilptr->instance);
	  dlptr = allocate_deviation(VERTEXDEL2);
	  dlptr->deviation->old.oldv = v1;
	  dlptr->next = ilptr->instance->deviations;
	  ilptr->instance->deviations = dlptr;
#ifdef DEBUG
	  printf("    Secondary deletion of vertex (v %u %s).\n",
		 v1->number,v1->label);
#endif
	}
	if (v2 != NULL) {
	  remove_vertex(v2,ilptr->instance->graph);
	  vertex_remove_instance(v2,ilptr->instance);
	  dlptr = allocate_deviation(VERTEXDEL2);
	  dlptr->deviation->old.oldv = v2;
	  dlptr->next = ilptr->instance->deviations;
	  ilptr->instance->deviations = dlptr;
#ifdef DEBUG
	  printf("    Secondary deletion of vertex (v %u %s).\n",
		 v2->number,v2->label);
#endif
	}
	/* remove edge from graph */
	remove_edge(e,ilptr->instance->graph);
	ilptr->instance->deleted = TRUE;
	dlptr = allocate_deviation(EDGEDEL);
	dlptr->deviation->old.olde = e;
	dlptr->next = ilptr->instance->deviations;
	ilptr->instance->deviations = dlptr;
#ifdef DEBUG
	printf("    Deleted edge (e %s %u %u).\n",
	       e->label,e->vertex1,e->vertex2);
#endif
	weight = weight + weight2;
      }
#ifdef DEBUG
      else printf("    Failed due to disconnection of instance graph.\n");
#endif
      if (v1 != NULL) v1->deleted = FALSE;
      if (v2 != NULL) v2->deleted = FALSE;
      e->deleted = FALSE;
      ilptr = ilptr->next;
    }
    /* instance removal postponed since we're using this instance list
       in the above while loop */
    edge_remove_deleted_instances(e);
  }
#ifdef DEBUG
  else printf("  Failed due to previously deviated edge (e %s %u %u).\n",
	      e->label,e->vertex1,e->vertex2);
#endif
  return(weight);
}

fuzz_instances(ninsts)
     ul ninsts;
/* Randomly deviate random instances until DeviationAmt achieved. */
{
  ul rn, dev;
  float rd, weight, total_weight;
  struct Instance *inst;

  total_weight = 0.0;
  while ((total_weight / ((float) ninsts)) < DeviationAmt) {
    weight = 0.0;
    rn = random_num(ninsts);
    inst = nth_inst(rn);
    rd = (float) random_num(10000);
    if (rd < (PSubVertexLabel * 10000.0)) dev = VERTEXLABEL;
    if (rd < ((PSubVertexLabel + PSubEdgeLabel) * 10000.0)) dev = EDGELABEL;
    if (rd < ((PSubVertexLabel + PSubEdgeLabel + PDelVertex) * 10000.0))
      dev = VERTEXDEL;
    else dev = EDGEDEL;
    switch (dev) {
    case VERTEXLABEL:
#ifdef DEBUG
      printf("\nAttempting to change vertex label for instance:\n");
      write_instance(stdout,inst);
#endif
      weight = change_vertex_label(inst);
      break;
    case EDGELABEL:
#ifdef DEBUG
      printf("\nAttempting to change edge label for instance:\n");
      write_instance(stdout,inst);
#endif
      weight = change_edge_label(inst);
      break;
    case VERTEXDEL:
#ifdef DEBUG
      printf("\nAttempting to delete vertex for instance:\n");
      write_instance(stdout,inst);
#endif
      weight = delete_vertex(inst);
      break;
    case EDGEDEL:
#ifdef DEBUG
      printf("\nAttempting to delete edge for instance:\n");
      write_instance(stdout,inst);
#endif
      weight = delete_edge(inst);
      break;
    default:
      fprintf(stderr,"BUG: Invalid deviation selected.\n");
      exit(1);
    }
#ifdef DEBUG
    printf("  Total deviation weight (if any) = %f.\n",weight);
#endif
    total_weight = total_weight + weight;
  }
}

/*** Construct Graph from Instances  ***/

struct Graph *graph_from_instances()
/* Return graph made up of all unique vertices and edges from the instances. */
{
  struct Graph *g;
  struct InstanceList *ilptr;
  struct VertexList *vlptr, *vl;
  struct EdgeList *elptr, *el;

  if ((g = (struct Graph *) malloc(sizeof(struct Graph))) == NULL) {
    fprintf(stderr,"Unable to allocate final graph.\n");
    exit(1);
  }
  g->vertices = NULL;
  g->edges = NULL;
  ilptr = Instances;
  while (ilptr != NULL) {
    vlptr = ilptr->instance->graph->vertices;
    while (vlptr != NULL) {
      if (! vertex_member(vlptr->vertex,g)) {
	if ((vl = (struct VertexList *) malloc(sizeof(struct VertexList)))
	    == NULL) {
	  fprintf(stderr,"Unable to allocate final vertex list.\n");
	  exit(1);
	}
	vl->vertex = vlptr->vertex;
	vl->next = g->vertices;
	g->vertices = vl;
      }
      vlptr = vlptr->next;
    }
    elptr = ilptr->instance->graph->edges;
    while (elptr != NULL) {
      if (! edge_member(elptr->edge,g)) {
	if ((el = (struct EdgeList *) malloc(sizeof(struct EdgeList)))
	    == NULL) {
	  fprintf(stderr,"Unable to allocate final edge list.\n");
	  exit(1);
	}
	el->edge = elptr->edge;
	el->next = g->edges;
	g->edges = el;
      }
      elptr = elptr->next;
    }
    ilptr = ilptr->next;
  }
  return(g);
}

/*** Add Random Vertices to Graph ***/

add_random_vertices(g,size)
     struct Graph *g;
     ul size;
/* Add random vertices to graph g until size number of vertices in graph. */
{
  ul maxvn;
  long vneed;
  struct VertexList *vlptr;
  struct Vertex *v;
  char *rl;

  maxvn = 0;
  vlptr = g->vertices;
  while (vlptr != NULL) {
    if (vlptr->vertex->number > maxvn) maxvn = vlptr->vertex->number;
    vlptr = vlptr->next;
  }
  vneed = size - n_vertices(g);
  while (vneed > 0) {
    rl = random_label("",VertexLabels);
    maxvn++;
    v = allocate_vertex(maxvn,rl,0,NULL);
    if ((vlptr = (struct VertexList *) malloc(sizeof(struct VertexList)))
	== NULL) {
      fprintf(stderr,"Unable to allocate final vertex list.\n");
      exit(1);
    }
    vlptr->vertex = v;
    vlptr->next = g->vertices;
    g->vertices = vlptr;
    vneed--;
#ifdef DEBUG
    printf("Adding random vertex (v %u %s) to achieve graph size.\n",
	   v->number,v->label);
#endif
  }
}

/*** Connect Final Graph ***/

struct Vertex *random_vertex(g,connect)
     struct Graph *g;
     ul connect;
/* Find a random vertex in graph g whose connected value is connect. */
{
  ul nv, rn;
  struct Vertex *v;
  struct VertexList *vlptr;

  nv = n_vertices(g);
  rn = random_num(nv);
  v = nth_vertex(rn,g);
  vlptr = g->vertices; /* position pointer on random vertex */
  while (rn > 0) {
    vlptr = vlptr->next;
    rn--;
  }
  /* get first vertex with connected = connect */
  while (vlptr->vertex->connected != connect) {
    vlptr = vlptr->next;
    if (vlptr == NULL) vlptr = g->vertices;
  }
  return(vlptr->vertex);
}

connect_graph(g)
     struct Graph *g;
/* Connect graph with random unique edges. */
{
  struct Vertex *v1, *v2;
  char *el;
  struct Edge *e;
  struct EdgeList *elptr;

  while (! connected(g)) {
    v1 = random_vertex(g,1);
    v2 = random_vertex(g,0);
    el = random_label("",EdgeLabels);
    e = allocate_edge(el,v1->number,v2->number,NULL);
    if ((elptr = (struct EdgeList *) malloc(sizeof(struct EdgeList)))
	== NULL) {
      fprintf(stderr,"Unable to allocate final edge list\n");
      exit(1);
    }
    elptr->edge = e;
    elptr->next = g->edges;
    g->edges = elptr;
#ifdef DEBUG
    printf("Adding edge (e %s %u %u) to connect graph.\n",
	   e->label,e->vertex1,e->vertex2);
#endif
  }
}

/*** Achieve Connectivity ***/

ul external_connections(inst,g)
     struct Instance *inst;
     struct Graph *g;
/* Return number of external connections of instance inst in graph g. */
{
  ul nec;
  struct EdgeList *elptr;
  struct Vertex *v1, *v2;
  boolean v1mem, v2mem;

  nec = 0;
  elptr = g->edges;
  while (elptr != NULL) {
    v1 = find_vertex(elptr->edge->vertex1,g);
    v2 = find_vertex(elptr->edge->vertex2,g);
    v1mem = vertex_member(v1,inst->graph);
    v2mem = vertex_member(v2,inst->graph);
    if ((v1mem && (! v2mem)) || ((! v1mem) && v2mem))
      nec++;
    elptr = elptr->next;
  }
  return(nec);
}

achieve_connectivity(g,ilist)
     struct Graph *g;
     struct InstanceList *ilist;
/* Add edges to instances in graph g until Connectivity achieved. */
{
  ul nec, nv, rn;
  long ecneed;
  struct Vertex *v1, *v2;
  char *el;
  struct Edge *e;
  struct EdgeList *elptr;

  while (ilist != NULL) {
    nec = external_connections(ilist->instance,g);
    ecneed = Connectivity - nec;
#ifdef DEBUG
    printf("External connectivity for instance %u is %u.\n",
	   ilist->instance->number,nec);
#endif
    while (ecneed > 0) {
      nv = n_vertices(ilist->instance->graph);
      rn = random_num(nv);
      v1 = nth_vertex(rn,ilist->instance->graph);
      nv = n_vertices(g);
      rn = random_num(nv);
      v2 = nth_vertex(rn,g);
      if (! vertex_member(v2,ilist->instance->graph)) {
	el = random_label("",EdgeLabels);
	e = allocate_edge(el,v1->number,v2->number,NULL);
	if (edge_member(e,g))
	  free(e);
	else {
	  if ((elptr = (struct EdgeList *) malloc(sizeof(struct EdgeList)))
	      == NULL) {
	    fprintf(stderr,"Unable to allocate connectivity edge list.\n");
	    exit(1);
	  }
	  elptr->edge = e;
	  elptr->next = g->edges;
	  g->edges = elptr;
	  ecneed--;
#ifdef DEBUG
	  printf("  Added external connection (e %s %u %u).\n",
		 e->label,e->vertex1,e->vertex2);
#endif
	}
      }
    }
    ilist = ilist->next;
  }
}


/*** Add Random Edges to Graph ***/

add_random_edges(g,size)
     struct Graph *g;
     ul size;
/* Add random edges to graph g until size number of edges in graph. */
{
  ul nv, rn;
  long eneed;
  struct EdgeList *elptr;
  struct Edge *e;
  struct Vertex *v1, *v2;
  char *el;

  eneed = size - n_edges(g);
  nv = n_vertices(g);
  while (eneed > 0) {
    rn = random_num(nv);
    v1 = nth_vertex(rn,g);
    rn = random_num(nv);
    v2 = nth_vertex(rn,g);
    if (! vertex_equal(v1,v2)) {
      el = random_label("",EdgeLabels);
      e = allocate_edge(el,v1->number,v2->number,NULL);
      if (edge_member(e,g))
	free(e);
      else {
	if ((elptr = (struct EdgeList *) malloc(sizeof(struct EdgeList)))
	    == NULL) {
	  fprintf(stderr,"Unable to allocate connectivity edge list.\n");
	  exit(1);
	}
	elptr->edge = e;
	elptr->next = g->edges;
	g->edges = elptr;
	eneed--;
#ifdef DEBUG
	printf("Adding random edge (e %s %u %u) to achieve graph size.\n",
	       e->label,e->vertex1,e->vertex2);
#endif
      }
    }
  }
}

/*** Main Build-Graph Procedure ***/

build_graph()
{
  ul subsize, ninsts, olap;
  G = NULL;
  subsize = graph_size(Substructure);
  ninsts = compute_instances(subsize);
  olap = subsize * ninsts * Overlap;
#ifdef DEBUG
  printf("subsize = %u, ninsts = %u, olap = %u\n",subsize,ninsts,olap);
#endif
  Instances = build_instances(ninsts);
  merge_instances(olap,ninsts);
  fuzz_instances(ninsts);
#ifdef DEBUG
  printf("\n");
#endif
  G = graph_from_instances();
  add_random_vertices(G,Vertices);
#ifdef DEBUG
  printf("\n");
#endif
  connect_graph(G);
#ifdef DEBUG
  printf("\n");
#endif
  achieve_connectivity(G,Instances);
#ifdef DEBUG
  printf("\n");
#endif
  add_random_edges(G,Edges);
}


/************************* Write final graph to file *************************/

ul instance_structure()
{
  ul n;
  struct VertexList *vlptr;
  struct EdgeList *elptr;

  n = 0;
  vlptr = G->vertices;
  while (vlptr != NULL) {
    if (vlptr->vertex->instances != NULL) n++;
    vlptr = vlptr->next;
  }
  elptr = G->edges;
  while (elptr != NULL) {
    if (elptr->edge->instances != NULL) n++;
    elptr = elptr->next;
  }
  return(n);
}

ul overlapping_instance_structure()
/* Returns amount of instance structure that overlaps. */
{
  struct VertexList *vlptr;
  struct EdgeList *elptr;
  ul n, olap;

  olap = 0;
  vlptr = G->vertices;
  while (vlptr != NULL) {
    n = n_instances(vlptr->vertex->instances);
    if (n > 1) olap = olap + (n - 1);
    vlptr = vlptr->next;
  }
  elptr = G->edges;
  while (elptr != NULL) {
    n = n_instances(elptr->edge->instances);
    if (n > 1) olap = olap + (n - 1);
    elptr = elptr->next;
  }
  return(olap);
}

float deviation_weight(inst)
     struct Instance *inst;
/* Return total weight of instance's deviations. */
{
  struct DeviationList *dlptr;
  float weight;

  dlptr = inst->deviations;
  weight = 0.0;
  while (dlptr != NULL) {
    switch (dlptr->deviation->type) {
    case VERTEXLABEL:
      weight = weight + WSubVertexLabel;
      break;
    case EDGELABEL:
      weight = weight + WSubEdgeLabel;
      break;
    case VERTEXDEL:
      weight = weight + WDelVertex;
      break;
    case EDGEDEL:
      weight = weight + WDelEdge;
      break;
    case VERTEXDEL2:
      weight = weight + WDelVertex2;
      break;
    case EDGEDEL2:
      weight = weight + WDelEdge;
      break;
    default:
      fprintf(stderr,"Unknown deviation type in final graph.\n");
      exit(1);
    }
    dlptr = dlptr->next;
  }
  return(weight);
}

write_final_graph(fp,title,number)
     FILE *fp;
     char *title;
     int number;
{
  ul nv, ne, is, subsize, ninsts, nec;
  int threshold, limit;
  float ois, olap, dw;
  struct InstanceList *ilptr;

  nv = n_vertices(G);
  ne = n_edges(G);
  Map = (ul *) malloc(nv * sizeof(ul));
  is = instance_structure();
  subsize = graph_size(Substructure);
  ninsts = n_instances(Instances);
  ois = overlapping_instance_structure();
  olap = ((float) ois) / ((float) (subsize * ninsts));

  if (DeviationAmt == 0.0)     /* for SUBDUE input */
    threshold = subsize;
  else threshold = ((float) subsize) / DeviationAmt;
  limit = 1.5 * ((float) n_edges(Substructure));
  /*fprintf(fp,"t %d\n",threshold);
  fprintf(fp,"l %d\n",limit);

  fprintf(fp,"%% Comments:\n");
  fprintf(fp,"%%  graph size = %u (%u vertices, %u edges)\n",
	  nv + ne, nv, ne);
  fprintf(fp,"%%  coverage = %f\n",((float) is) / ((float) (nv + ne)));
  fprintf(fp,"%%  overlap = %f\n",olap);*/
  ilptr = Instances;
  nec = 0;
  dw = 0.0;
  while (ilptr != NULL) {
    nec = nec + external_connections(ilptr->instance,G);
    dw = dw + deviation_weight(ilptr->instance);
    ilptr = ilptr->next;
  }
  /*fprintf(fp,"%%  connectivity = %f (avg external connections per instance)\n",
	  ((float) nec) / ((float) ninsts));
  fprintf(fp,"%%  deviation = %f (avg deviation per instance)\n",
	  (dw / ((float) ninsts)));
  fprintf(fp,"p %s %u\n",title,number);*/
  map_vertices(G, nv);
  write_graph(fp,G);
}


/************************* Main program *************************/

main(argc,argv)
     int argc;
     char *argv[];
{
  FILE *fp;
  char *infile;
  char outfile[FILENAMELEN];
  int i;
  int seed;
  infile = argv[1];
  sscanf(argv[2],"%d",&seed);   /* init random number generator */
#ifdef DEBUG
  printf("Commence Graph Generation...\n\nSeed = %d\n",seed);
#endif
  srand(seed);
  if ((fp = fopen(infile,"r")) == NULL) {
    fprintf(stderr,"Unable to open %s.\n",infile);
    exit(1);
  }
  read_parameters(fp);
  fclose(fp);
#ifdef DEBUG
  sprintf(outfile,"%s.parms",infile);
  if ((fp = fopen(outfile,"w")) == NULL) {
    fprintf(stderr,"Unable to open %s.\n",outfile);
    exit(1);
  }
  write_parameters(fp);
  fclose(fp);
#endif
  build_graph();
  sprintf(outfile,"%s.graph",infile);
  if ((fp = fopen(outfile,"w")) == NULL) {
    fprintf(stderr,"Unable to open %s.\n",outfile);
    exit(1);
  }
  write_final_graph(fp,infile,1);
  fclose(fp);
  sprintf(outfile,"%s.insts",infile);
  if ((fp = fopen(outfile,"w")) == NULL) {
    fprintf(stderr,"Unable to open %s.\n",outfile);
    exit(1);
  }
  write_instances(fp);
  fclose(fp);
#ifdef DEBUG
  printf("\nGraph Generation Complete.\n");
#endif
  exit(0);
}


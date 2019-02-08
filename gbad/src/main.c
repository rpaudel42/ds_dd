//******************************************************************************
// main.c
//
// GBAD, version 3.3
//
// Date      Name       Description
// ========  =========  ==========================================================
// 08/12/09  Eberle     Initial version, taken from SUBDUE 5.2.1
// 11/30/09  Eberle     Removed call to RemoveSimilarSubstructures since some 
//                      situations resulted in potential anomalies being 
//                      discarded by this routine; Fixed bounds-checks on some 
//                      user provided values.
// 12/10/09  Eberle     Removed parameters->similarity.
// 12/17/09  Graves     Added GUI coloring support
// 11/13/12  Eberle     Added -noOpt option to turn off any special optimization
// 01/31/13  Hensley    Removed Linux-Specific aspects, for compilation in Windows
// 02/14/13  Eberle     Clarified "Optimized" flag.
// 02/16/13  Hensley    Removed obsolete code.
//
//********************************************************************************

#include "gbad.h"
#include <time.h>
#include <unistd.h>

// Function prototypes

int main(int, char **);
Parameters *GetParameters(int, char **);
void PrintParameters(Parameters *);
void FreeParameters(Parameters *);

//******************************************************************************
// NAME:    main
//
// INPUTS:  (int argc) - number of arguments to program
//          (char **argv) - array of strings of arguments to program
//
// RETURN:  (int) - 0 if all is well
//
// PURPOSE: Main GBAD function that processes command-line arguments
//          and initiates discovery.
//******************************************************************************

int main(int argc, char **argv)
{
   clock_t startTime, endTime;
   static long clktck = 0;
   time_t iterationStartTime;
   time_t iterationEndTime;
   SubList *subList;
   Substructure *normSub = NULL;
   Parameters *parameters;
   FILE *outputFile;
   ULONG iteration;
   BOOLEAN done;

   clktck = CLOCKS_PER_SEC;
   startTime = clock();
   printf("GBAD %s\n\n", GBAD_VERSION);
   parameters = GetParameters(argc, argv);

   // compress positive graphs with predefined subs, if given
   if (parameters->numPreSubs > 0)
      CompressWithPredefinedSubs(parameters);

   PrintParameters(parameters);

   if (parameters->iterations > 1)
      printf("----- Iteration 1 -----\n\n");

   iteration = 1;
   parameters->currentIteration = iteration;
   done = FALSE;

   while ((iteration <= parameters->iterations) && (!done))
   {
      iterationStartTime = time(NULL);
      if (iteration > 1)
         printf("----- Iteration %lu -----\n\n", iteration);

      printf("%lu positive graphs: %lu vertices, %lu edges",
             parameters->numPosEgs, parameters->posGraph->numVertices,
             parameters->posGraph->numEdges);

      if (parameters->evalMethod == EVAL_MDL)
         printf(", %.0f bits\n", parameters->posGraphDL);
      else
         printf("\n");
      printf("%lu unique labels\n", parameters->labelList->numLabels);
      printf("\n");

      if ((parameters->prob) && (iteration > 1))
      {
         //
         // If GBAD-P option chosen, after the first iteration, we no longer
         // care about minsize of maxsize after the first iteration (if the
         // user specified these parameters), as we are just dealing with
         // single extensions from the normative - so set it to where we
         // just look at substructures that are composed of the normative
         // pattern (SUB_) and the single vertex extension.
         //
         parameters->minVertices = 1;
         parameters->maxVertices = 2;
      }
      //
      // If the user has specified a normative pattern, on the first iteration
      // need to save the top-N substructures, where N is what the user
      // specified with the -norm parameter.
      //
      ULONG saveNumBestSubs = parameters->numBestSubs;
      if ((iteration == 1) && (!parameters->noAnomalyDetection) &&
          (parameters->norm > parameters->numBestSubs))
         parameters->numBestSubs = parameters->norm;
      //
      // -prune is useful to get to the initial normative pattern, but 
      // possibly detremental to discovering anomalies... so, turn off 
      // pruning (in case it was turned on), so that it is not used in 
      // future iterations.
      //
      if ((parameters->prob) && (iteration > 1))
      {
         parameters->prune = FALSE;
      }
 
      subList = DiscoverSubs(parameters, iteration);

      //
      // Now that we have the best substructure(s), return the user
      // specified number of best substructures to its original value.
      //
      if (iteration == 1)
         parameters->numBestSubs = saveNumBestSubs;

      if (subList->head == NULL) 
      {
         done = TRUE;
         printf("No substructures found.\n\n");
      }
      else 
      {
         //
         // GBAD-MDL
         //
         if (parameters->mdl)
            GBAD_MDL(subList,parameters);

         //
         // GBAD-MPS
         //
         if (parameters->mps)
         {
            GBAD_MPS(subList,parameters);
         }

         //
         // GBAD-P
         //
         if (parameters->prob)
         {
            normSub = GBAD_P(subList,iteration,parameters);
         }

         // write output to stdout
         if (parameters->outputLevel > 1) 
         {
            printf("\nBest %lu substructures:\n\n", CountSubs (subList));
            PrintSubList(subList, parameters);
         } 
         else 
         {
            printf("\nBest substructure: ");
            if ((CountSubs(subList) > 0) && (subList->head->sub != NULL))
               PrintSub(subList->head->sub, parameters);
            else
               printf("None.");
            printf("\n\n");
         }

         // write machine-readable output to file, if given
         if (parameters->outputToFile) 
         {
            /*outputFile = fopen(parameters->outFileName, "a");
            if (outputFile == NULL) 
            {
               printf("WARNING: unable to write to output file %s,",
                      parameters->outFileName);
               printf("disabling\n");
               parameters->outputToFile = FALSE;
            }

            WriteGraphToFile(outputFile, subList->head->sub->definition,
                             parameters->labelList, 0, 0,
                             subList->head->sub->definition->numVertices,
                             TRUE);
            fclose(outputFile);*/
            outputFile = fopen(parameters->outFileName, "a");
            if (outputFile == NULL) 
            {
               printf("WARNING: unable to write to output file %s,",
                      parameters->outFileName);
               printf("disabling\n");
               parameters->outputToFile = FALSE;
            }

            WriteSubGraphToFile(outputFile, subList, parameters,TRUE);
            fclose(outputFile);

         }

         if (iteration < parameters->iterations) 
         {                                    // Another iteration?
            if (parameters->evalMethod == EVAL_SETCOVER) 
            {
               printf("Removing positive examples covered by");
               printf(" best substructure.\n\n");
               RemovePosEgsCovered(subList->head->sub, parameters);
            } 
            else 
            {
               //
               // For the GBAD-P algorithm, multiple iterations will need
               // to be performed, and if it is the first iteration
	       // AND the user has specified a different normative
	       // pattern (other than the best one), we need to 
	       // use the substructure that was set above.
	       //
	       if ((iteration == 1) && (parameters->prob))
	       {
	          printf("Compressing graph by best substructure (%lu):\n",
	                 parameters->norm);
                  PrintSub(normSub,parameters);
	          printf("\n");
                  CompressFinalGraphs(normSub, parameters, 
	                              iteration, FALSE);
               } else
                  CompressFinalGraphs(subList->head->sub, parameters, 
	                              iteration, FALSE);
	    }

            // check for stopping condition
            // if set-covering, then no more positive examples
            // if MDL or size, then positive graph contains no edges
            if (parameters->evalMethod == EVAL_SETCOVER) 
            {
               if (parameters->numPosEgs == 0) 
               {
                  done = TRUE;
                  printf("Ending iterations - ");
                  printf("all positive examples covered.\n\n");
               }
            } 
            else 
            {
               if (parameters->posGraph->numEdges == 0) 
               {
                  done = TRUE;
                  printf("Ending iterations - graph fully compressed.\n\n");
               }
            }
         }
         if ((iteration == parameters->iterations) && (parameters->compress))
         {
            if (parameters->evalMethod == EVAL_SETCOVER)
               WriteUpdatedGraphToFile(subList->head->sub, parameters);
            else 
               WriteCompressedGraphToFile(subList->head->sub, parameters,
                                          iteration);
         }
      }

      //
      // Need to store information regarding initial best substructure, for use
      // in future GBAD-P calculations
      //
      if ((parameters->prob) && (iteration == 1) && (subList->head != NULL))
      {
         parameters->numPreviousInstances = subList->head->sub->numInstances;
      }
      if ((parameters->prob) && (iteration > 1) && (subList->head != NULL))
         parameters->numPreviousInstances = subList->head->sub->numInstances;

      FreeSubList(subList);
      if (parameters->iterations > 1) 
      {
         iterationEndTime = time(NULL);
         printf("Elapsed time for iteration %lu = %lu seconds.\n\n",
         iteration, (iterationEndTime - iterationStartTime));
      }
      iteration++;
      parameters->currentIteration = iteration;
   }
 
   // GUI coloring
   if (parameters->dotToFile)
   {
      ULONG index;
      double minAnomalousValue = 1.0;

      // find the min anom value
      for (index=0; index < parameters->originalPosGraph->numVertices; index++)
      {
         if (parameters->originalPosGraph->vertices[index].anomalousValue < minAnomalousValue)
            minAnomalousValue = parameters->originalPosGraph->vertices[index].anomalousValue;
      }
      for (index=0; index < parameters->originalPosGraph->numEdges; index++)
      {
         if (parameters->originalPosGraph->edges[index].anomalousValue < minAnomalousValue)
            minAnomalousValue = parameters->originalPosGraph->edges[index].anomalousValue;
      }

      // update color based on min anom value
      for (index=0; index < parameters->originalPosGraph->numVertices; index++)
      {
         if (parameters->originalPosGraph->vertices[index].anomalousValue == minAnomalousValue)
            parameters->originalPosGraph->vertices[index].color = POSITIVE_ANOM_VERTEX;
      }

      for (index=0; index < parameters->originalPosGraph->numEdges; index++)
      {
         if (parameters->originalPosGraph->edges[index].anomalousValue == minAnomalousValue)
            parameters->originalPosGraph->edges[index].color = POSITIVE_ANOM_EDGE;
      }

      WriteGraphToDotFile(parameters->dotFileName, parameters);
   }

   FreeParameters(parameters);
   endTime = clock();
   printf("\nGBAD done (elapsed CPU time = %7.2f seconds).\n",
          (endTime - startTime) / (double) clktck);
   return 0;
}


//******************************************************************************
// NAME: GetParameters
//
// INPUTS: (int argc) - number of command-line arguments
//         (char *argv[]) - array of command-line argument strings
//
// RETURN: (Parameters *)
//
// PURPOSE: Initialize parameters structure and process command-line
//          options.
//******************************************************************************

Parameters *GetParameters(int argc, char *argv[])
{
   Parameters *parameters;
   int i;
   double doubleArg;
   ULONG ulongArg;
   FILE *outputFile;
   ULONG argumentExists;

   parameters = (Parameters *) malloc(sizeof(Parameters));
   if (parameters == NULL)
      OutOfMemoryError("parameters");

   // GUI coloring
   strcpy(parameters->dotFileName, "none");
   parameters->dotToFile = FALSE;

   // initialize default parameter settings
   parameters->directed = TRUE;
   parameters->limit = 0;
   parameters->numBestSubs = 3;
   parameters->beamWidth = 4;
   parameters->valueBased = FALSE;
   parameters->prune = FALSE;
   strcpy(parameters->outFileName, "none");
   parameters->outputToFile = FALSE;
   parameters->outputLevel = 2;
   parameters->allowInstanceOverlap = FALSE;
   parameters->threshold = 0.0;
   parameters->evalMethod = EVAL_MDL;
   parameters->iterations = 1;
   strcpy(parameters->psInputFileName, "none");
   parameters->predefinedSubs = FALSE;
   parameters->minVertices = 1;
   parameters->maxVertices = 0; // i.e., infinity
   parameters->compress = FALSE;

   parameters->mdl = FALSE;
   parameters->mdlThreshold = 0.0;
   parameters->mpsThreshold = 0.0;
   parameters->prob = FALSE;
   parameters->mps = FALSE;
   parameters->maxAnomalousScore = MAX_DOUBLE;
   parameters->minAnomalousScore = 0.0;
   parameters->noAnomalyDetection = TRUE;
   parameters->norm = 1;
   parameters->optimize = TRUE;

   if (argc < 2)
   {
      fprintf(stderr, "input graph file name must be supplied\n");
      exit(1);
   }

   // process command-line options
   i = 1;
   while (i < (argc - 1))
   {
      if (strcmp(argv[i], "-beam") == 0) 
      {
         i++;
         sscanf(argv[i], "%lu", &ulongArg);
         if (ulongArg == 0) 
         {
            fprintf(stderr, "%s: beam must be greater than zero\n", argv[0]);
            exit(1);
         }
         parameters->beamWidth = ulongArg;
      }
      else if (strcmp(argv[i], "-compress") == 0)
      {
         parameters->compress = TRUE;
      }
      else if (strcmp(argv[i], "-eval") == 0) 
      {
         i++;
         sscanf(argv[i], "%lu", &ulongArg);
         if ((ulongArg < 1) || (ulongArg > 3)) 
         {
            fprintf(stderr, "%s: eval must be 1-3\n", argv[0]);
            exit(1);
         }
         parameters->evalMethod = ulongArg;
      } 
      else if (strcmp(argv[i], "-iterations") == 0) 
      {
         i++;
         sscanf(argv[i], "%lu", &ulongArg);
         parameters->iterations = ulongArg;
      } 
      else if (strcmp(argv[i], "-limit") == 0) 
      {
         i++;
         sscanf(argv[i], "%lu", &ulongArg);
         if (ulongArg == 0) 
         {
            fprintf(stderr, "%s: limit must be greater than zero\n", argv[0]);
            exit(1);
         }
         parameters->limit = ulongArg;
      }
      else if (strcmp(argv[i], "-maxsize") == 0) 
      {
         i++;
         sscanf(argv[i], "%lu", &ulongArg);
         if (ulongArg == 0) 
         {
            fprintf(stderr, "%s: maxsize must be greater than zero\n", argv[0]);
            exit(1);
         }
         parameters->maxVertices = ulongArg;
      }
      else if (strcmp(argv[i], "-minsize") == 0) 
      {
         i++;
         sscanf(argv[i], "%lu", &ulongArg);
         if (ulongArg == 0) 
         {
            fprintf(stderr, "%s: minsize must be greater than zero\n", argv[0]);
            exit(1);
         }
         parameters->minVertices = ulongArg;
      }
      else if (strcmp(argv[i], "-nsubs") == 0) 
      {
         i++;
         sscanf(argv[i], "%lu", &ulongArg);
         if (ulongArg == 0) 
         {
            fprintf(stderr, "%s: nsubs must be greater than zero\n", argv[0]);
            exit(1);
         }
         parameters->numBestSubs = ulongArg;
      }
      else if (strcmp(argv[i], "-out") == 0) 
      {
         i++;
         strcpy(parameters->outFileName, argv[i]);
         parameters->outputToFile = TRUE;
      }
      else if (strcmp(argv[i], "-output") == 0) 
      {
         i++;
         sscanf(argv[i], "%lu", &ulongArg);
         if ((ulongArg < 1) || (ulongArg > 5)) 
         {
            fprintf(stderr, "%s: output must be 1-5\n", argv[0]);
            exit(1);
         }
         parameters->outputLevel = ulongArg;
      }
      else if (strcmp(argv[i], "-overlap") == 0) 
      {
         parameters->allowInstanceOverlap = TRUE;
      }
      else if (strcmp(argv[i], "-prune") == 0) 
      {
         parameters->prune = TRUE;
      }
      else if (strcmp(argv[i], "-ps") == 0) 
      {
         i++;
         strcpy(parameters->psInputFileName, argv[i]);
         parameters->predefinedSubs = TRUE;
      }
      else if (strcmp(argv[i], "-threshold") == 0) 
      {
         i++;
         sscanf(argv[i], "%lf", &doubleArg);
         if ((doubleArg < (double) 0.0) || (doubleArg > (double) 1.0))
         {
            fprintf(stderr, "%s: threshold must be 0.0-1.0\n", argv[0]);
            exit(1);
         }
         parameters->threshold = doubleArg;
      }
      else if (strcmp(argv[i], "-undirected") == 0) 
      {
         parameters->directed = FALSE;
      }
      else if (strcmp(argv[i], "-valuebased") == 0) 
      {
         parameters->valueBased = TRUE;
      }
      else if (strcmp(argv[i], "-mdl") == 0) 
      {
         i++;
         argumentExists = sscanf(argv[i], "%lf", &doubleArg);
         if ((argumentExists != 1) || (doubleArg <= (double) 0.0) || 
             (doubleArg >= (double) 1.0))
         {
            fprintf(stderr, "%s: Information Theoretic (MDL) threshold must be greater than 0.0 and less than 1.0\n", argv[0]);
            exit(1);
         }
         parameters->mdl = TRUE;
         parameters->mdlThreshold = doubleArg;
      }
      else if (strcmp(argv[i], "-prob") == 0) 
      {
         i++;
         sscanf(argv[i], "%lu", &ulongArg);
         if (ulongArg < 2)
         {
            fprintf(stderr, "%s: you must include a value greater than 1 as a parameter to the probabilistic anomaly detection method.\n", argv[0]);
            exit(1);
         }
         parameters->prob = TRUE;
         parameters->iterations = ulongArg;  // overrides -iterations specification
         parameters->maxAnomalousScore = 1.0;  // overrides default of MAX_DOUBLE
      }
      else if (strcmp(argv[i], "-mps") == 0) 
      {
         i++;
         argumentExists = sscanf(argv[i], "%lf", &doubleArg);
         if ((argumentExists != 1) || (doubleArg <= (double) 0.0) || 
             (doubleArg >= (double) 1.0))
         {
            fprintf(stderr, "%s: Maximum Partial Substructure (MPS) threshold must be greater than 0.0 and less than 1.0\n", argv[0]);
            exit(1);
         }
         parameters->mps = TRUE;
         parameters->mpsThreshold = doubleArg;
      }
      else if (strcmp(argv[i], "-maxAnomalousScore") == 0) 
      {
         i++;
         argumentExists = sscanf(argv[i], "%lf", &doubleArg);
         if ((argumentExists != 1) || (doubleArg <= (double) 0.0) || 
             (doubleArg >= (double) MAX_DOUBLE))
         {
            fprintf(stderr, "%s: maximum anomalous score must be greater than 0.0 and less than %lf\n", 
	            argv[0],MAX_DOUBLE);
            exit(1);
         }
	 //
	 // NOTE:  This check assumes that the user has specified a max
	 // anomalous score AFTER specifying they want to run the GBAD-P 
	 // algorithm.
	 //
         if (((doubleArg <= (double) 0.0) || (doubleArg >= 1.0)) &&
	     (parameters->prob))
         {
            fprintf(stderr, "%s: maximum anomalous score must be greater than 0.0 and less than 1.0\n",argv[0]);
            exit(1);
         }
         parameters->maxAnomalousScore = doubleArg;
      }
      else if (strcmp(argv[i], "-minAnomalousScore") == 0) 
      {
         i++;
         argumentExists = sscanf(argv[i], "%lf", &doubleArg);
         if ((argumentExists != 1) || (doubleArg < (double) 0.0) || 
             (doubleArg >= (double) MAX_DOUBLE))
         {
            fprintf(stderr, "%s: minimum anomalous score must be greater than or equal to 0.0 and less than %lf\n", argv[0],MAX_DOUBLE);
            exit(1);
         }
         parameters->minAnomalousScore = doubleArg;
      }
      else if (strcmp(argv[i], "-norm") == 0) 
      {
         i++;
         sscanf(argv[i], "%lu", &ulongArg);
         if (ulongArg < 1)
         {
            fprintf(stderr, "%s: you must specify a value of 1 or greater.\n", argv[0]);
            exit(1);
         }
         parameters->norm = ulongArg;
      }
      else if (strcmp(argv[i], "-noOpt") == 0) 
      {
         parameters->optimize = FALSE;
      }
      // GUI coloring
      else if(strcmp(argv[i], "-dot") == 0)
      {
         i++;
         strcpy(parameters->dotFileName, argv[i]);
         parameters->dotToFile = TRUE;
      }
      else 
      {
         fprintf(stderr, "%s: unknown option %s\n", argv[0], argv[i]);
         exit(1);
      }
      i++;
   }

   if ((parameters->mdl) || (parameters->prob) || (parameters->mps)) 
      parameters->noAnomalyDetection = FALSE;

   if (parameters->iterations == 0)
      parameters->iterations = MAX_UNSIGNED_LONG; // infinity

   // initialize log2Factorial[0..1]
   parameters->log2Factorial = (double *) malloc(2 * sizeof(double));
   if (parameters->log2Factorial == NULL)
      OutOfMemoryError("GetParameters:parameters->log2Factorial");
   parameters->log2FactorialSize = 2;
   parameters->log2Factorial[0] = 0; // lg(0!)
   parameters->log2Factorial[1] = 0; // lg(1!)

   // read graphs from input file
   strcpy(parameters->inputFileName, argv[argc - 1]);
   parameters->labelList = AllocateLabelList();
   parameters->posGraph = NULL;
   parameters->numPosEgs = 0;
   parameters->posEgsVertexIndices = NULL;

   ReadInputFile(parameters);
   if (parameters->evalMethod == EVAL_MDL)
   {
      parameters->posGraphDL = MDL(parameters->posGraph,
                                  parameters->labelList->numLabels, parameters);
   }

   // read predefined substructures
   parameters->numPreSubs = 0;
   if (parameters->predefinedSubs)
      ReadPredefinedSubsFile(parameters);

   parameters->incrementList = malloc(sizeof(IncrementList));
   parameters->incrementList->head = NULL;

   // create output file, if given
   if (parameters->outputToFile) 
   {
      outputFile = fopen(parameters->outFileName, "w");
      if (outputFile == NULL) 
      {
         printf("ERROR: unable to write to output file %s\n",
                parameters->outFileName);
         exit(1);
      }
      fclose(outputFile);
   }  

   if (parameters->numPosEgs == 0)
   {
      fprintf(stderr, "ERROR: no positive graphs defined\n");
      exit(1);
   }

   // Check bounds on discovered substructures' number of vertices
   if (parameters->maxVertices == 0)
      parameters->maxVertices = parameters->posGraph->numVertices;
   if (parameters->maxVertices < parameters->minVertices)
   {
      fprintf(stderr, "ERROR: minsize exceeds maxsize\n");
      exit(1);
   }

   // Set limit accordingly
   if (parameters->limit == 0)
   {
      parameters->limit = parameters->posGraph->numEdges / 2;
   }

   return parameters;
}


//******************************************************************************
// NAME: PrintParameters
//
// INPUTS: (Parameters *parameters)
//
// RETURN: (void)
//
// PURPOSE: Print selected parameters.
//******************************************************************************

void PrintParameters(Parameters *parameters)
{
   printf("Parameters:\n");
   printf("  Input file..................... %s\n",parameters->inputFileName);
   printf("  Predefined substructure file... %s\n",parameters->psInputFileName);
   printf("  Output file.................... %s\n",parameters->outFileName);
   printf("  Dot file....................... %s\n",parameters->dotFileName);
   printf("  Beam width..................... %lu\n",parameters->beamWidth);
   printf("  Compress....................... ");
   PrintBoolean(parameters->compress);
   printf("  Evaluation method.............. ");
   switch(parameters->evalMethod) 
   {
      case 1: printf("MDL\n"); break;
      case 2: printf("size\n"); break;
      case 3: printf("setcover\n"); break;
   }

   if (parameters->mdl) {
      printf("  Anomaly Detection method....... Information Theoretic\n");
      printf("  Information Theoretic threshold %lf\n", parameters->mdlThreshold);
   }
   if (parameters->prob) {
      printf("  Anomaly Detection method....... Probabilistic\n");
   }
   if (parameters->mps)
   {
      printf("  Anomaly Detection method....... Maximum Partial\n");
      printf("  Maximum Partial Sub threshold.. %lf\n", parameters->mpsThreshold);
   }
   if (parameters->noAnomalyDetection) 
      printf("  Anomaly Detection method....... NONE\n");
   if (!parameters->noAnomalyDetection) {
      if (parameters->maxAnomalousScore) {
         if (parameters->maxAnomalousScore == MAX_DOUBLE)
            printf("  Max Anomalous Score............ MAX\n");
         else
            printf("  Max Anomalous Score............ %lf\n",parameters->maxAnomalousScore);
      }
      if (parameters->minAnomalousScore) 
         printf("  Min Anomalous Score............ %lf\n",parameters->minAnomalousScore);
      if (parameters->norm) 
         printf("  Normative Pattern.............. %lu\n",parameters->norm);
   }

   printf("  'e' edges directed............. ");
   PrintBoolean(parameters->directed);
   printf("  Iterations..................... ");
   if (parameters->iterations == 0)
      printf("infinite\n");
   else 
      printf("%lu\n", parameters->iterations);
   printf("  Limit.......................... %lu\n", parameters->limit);
   printf("  Minimum size of substructures.. %lu\n", parameters->minVertices);
   printf("  Maximum size of substructures.. %lu\n", parameters->maxVertices);
   printf("  Number of best substructures... %lu\n", parameters->numBestSubs);
   printf("  Output level................... %lu\n", parameters->outputLevel);
   printf("  Allow overlapping instances.... ");
   PrintBoolean(parameters->allowInstanceOverlap);
   printf("  Prune.......................... ");
   PrintBoolean(parameters->prune);
   if (!parameters->noAnomalyDetection) 
   {
      printf("  Optimized (Anomaly Detection).. ");
      PrintBoolean(parameters->optimize);
   }
   printf("  Threshold...................... %lf\n", parameters->threshold);
   printf("  Value-based queue.............. ");
   PrintBoolean(parameters->valueBased);
   printf("\n");

   printf("Read %lu total positive graphs\n", parameters->numPosEgs);
   if (parameters->numPreSubs > 0)
      printf("Read %lu predefined substructures\n", parameters->numPreSubs);
   printf("\n");
}


//******************************************************************************
// NAME: FreeParameters
//
// INPUTS: (Parameters *parameters)
//
// RETURN: (void)
//
// PURPOSE: Free memory allocated for parameters.  Note that the
//          predefined substructures are de-allocated as soon as they are
//          processed, and not here.
//******************************************************************************

void FreeParameters(Parameters *parameters)
{
   FreeGraph(parameters->posGraph);
   FreeLabelList(parameters->labelList);
   free(parameters->posEgsVertexIndices);
   free(parameters->log2Factorial);
   free(parameters);
}

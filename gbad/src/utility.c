//******************************************************************************
// utility.c
//
// Miscellaneous utility functions.
//
// Date      Name       Description
// ========  =========  ========================================================
// 08/12/09  Eberle     Initial version, taken from SUBDUE 5.2.1
//
//******************************************************************************

#include "gbad.h"


//******************************************************************************
// NAME: OutOfMemoryError
//
// INPUTS: (char *context)
//
// RETURN: (void)
//
// PURPOSE: Print out of memory error with context, and then exit.
//******************************************************************************

void OutOfMemoryError(char *context)
{
   printf("ERROR: out of memory allocating %s.\n", context);
   exit(1);
}


//******************************************************************************
// NAME: PrintBoolean
//
// INPUTS: (BOOLEAN boolean)
//
// RETURN: (void)
//
// PURPOSE: Print BOOLEAN input as 'TRUE' or 'FALSE'.
//******************************************************************************

void PrintBoolean(BOOLEAN boolean)
{
   if (boolean)
      printf("true\n");
   else 
      printf("false\n");
}


//******************************************************************************
// NAME:  CopySub
//
// INPUTS:  (Substructure *sub) - input substructure
//
// RETURN:  (Substructure *sub) - copy of input substructure
//
// PURPOSE:  Make new copy of existing substructure.
//******************************************************************************

Substructure *CopySub(Substructure *sub)
{
   Substructure *newSub;

   newSub = (Substructure *) malloc(sizeof(Substructure));

   newSub->definition = CopyGraph(sub->definition);
   newSub->posIncrementValue = sub->posIncrementValue;
   newSub->value = sub->value;
   newSub->numInstances = sub->numInstances;
   newSub->instances = NULL;

   return(newSub);
}

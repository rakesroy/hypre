/*BHEADER**********************************************************************
 * (c) 1998   The Regents of the University of California
 *
 * See the file COPYRIGHT_and_DISCLAIMER for a complete copyright
 * notice, contact person, and disclaimer.
 *
 * $Revision$
 *********************************************************************EHEADER*/
/******************************************************************************
 *
 *****************************************************************************/

#include "headers.h"

/*==========================================================================*/
/*==========================================================================*/
/**
  Augments measures by some random value between 0 and 1.

  {\bf Input files:}
  headers.h

  @return Error code.

  @param S [IN]
  parent graph matrix in CSR format
  @param measure_array [IN/OUT]
  measures assigned to each node of the parent graph

  @see hypre_AMGIndepSet */
/*--------------------------------------------------------------------------*/

int
hypre_InitParAMGIndepSet( hypre_ParCSRMatrix *S,
                          double             *measure_array )
{
   hypre_CSRMatrix *S_diag = hypre_ParCSRMatrixDiag(S);
   int              S_num_nodes = hypre_CSRMatrixNumRows(S_diag);
   int              first_index = hypre_ParCSRMatrixFirstRowIndex(S);
   int              i;
   int              ierr = 0;

   hypre_SeedRand(2747);
   for (i = 0; i < first_index; i++)
	hypre_Rand();
   for (i = 0; i < S_num_nodes; i++)
   {
      measure_array[i] += hypre_Rand();
   }

   return (ierr);
}

/*==========================================================================*/
/*==========================================================================*/
/**
  Select an independent set from a graph.  This graph is actually a
  subgraph of some parent graph.  The parent graph is described as a
  matrix in compressed sparse row format, where edges in the graph are
  represented by nonzero matrix coefficients (zero coefficients are
  ignored).  A positive measure is given for each node in the
  subgraph, and this is used to pick the independent set.  A measure
  of zero must be given for all other nodes in the parent graph.  The
  subgraph is a collection of nodes in the parent graph.

  Positive entries in the `IS\_marker' array indicate nodes in the
  independent set.  All other entries are zero.

  The algorithm proceeds by first setting all nodes in `graph\_array'
  to be in the independent set.  Nodes are then removed from the
  independent set by simply comparing the measures of adjacent nodes.

  {\bf Input files:}
  headers.h

  @return Error code.

  @param S [IN]
  parent graph matrix in CSR format
  @param measure_array [IN]
  measures assigned to each node of the parent graph
  @param graph_array [IN]
  node numbers in the subgraph to be partitioned
  @param graph_array_size [IN]
  number of nodes in the subgraph to be partitioned
  @param IS_marker [IN/OUT]
  marker array for independent set

  @see hypre_InitAMGIndepSet */
/*--------------------------------------------------------------------------*/

int
hypre_ParAMGIndepSet( hypre_ParCSRMatrix *S,
		      hypre_CSRMatrix    *S_ext,
                      double             *measure_array,
                      int                *graph_array,
                      int                 graph_array_size,
                      int                *IS_marker        )
{
   int		   *col_map_offd = hypre_ParCSRMatrixColMapOffd(S);
   hypre_CSRMatrix *S_diag      = hypre_ParCSRMatrixDiag(S);
   int             *S_diag_i    = hypre_CSRMatrixI(S_diag);
   int             *S_diag_j    = hypre_CSRMatrixJ(S_diag);
   double          *S_diag_data = hypre_CSRMatrixData(S_diag);
   hypre_CSRMatrix *S_offd      = hypre_ParCSRMatrixOffd(S);
   int             *S_offd_i    = hypre_CSRMatrixI(S_offd);
   int             *S_offd_j;
   double          *S_offd_data;
   int             *S_ext_i;
   int             *S_ext_j;
   double          *S_ext_data;

   int		    local_num_vars = hypre_CSRMatrixNumRows(S_diag);
   int		    num_cols_offd = hypre_CSRMatrixNumCols(S_offd);
   int		    col_1 = hypre_ParCSRMatrixFirstColDiag(S);
   int		    col_n = col_1 + hypre_CSRMatrixNumCols(S_diag);
   int		    ic, jc;
   int              i, j, ig, jS;
                   
   int              ierr = 0;

   /*-------------------------------------------------------
    * Initialize IS_marker by putting all nodes in
    * the independent set.
    *-------------------------------------------------------*/

   if (hypre_CSRMatrixNumCols(S_offd))
   {
	S_offd_j = hypre_CSRMatrixJ(S_offd);
	S_offd_data = hypre_CSRMatrixData(S_offd);
	S_ext_i = hypre_CSRMatrixI(S_ext);
	S_ext_j = hypre_CSRMatrixJ(S_ext);
	S_ext_data = hypre_CSRMatrixData(S_ext);
   }

   for (ig = 0; ig < graph_array_size; ig++)
   {
      i = graph_array[ig];
      if (measure_array[i] > 1)
      {
         IS_marker[i] = 1;
      }
   }

   /*-------------------------------------------------------
    * Remove nodes from the initial independent set
    *-------------------------------------------------------*/

   for (ig = 0; ig < graph_array_size; ig++)
   {
    i = graph_array[ig];

    if (i < local_num_vars)
    {
      if (measure_array[i] > 1)
      {
         for (jS = S_diag_i[i]; jS < S_diag_i[i+1]; jS++)
         {
            j = S_diag_j[jS];
            
            /* only consider valid graph edges */
            if ( (measure_array[j] > 1) && (S_diag_data[jS]) )
            {
               if (measure_array[i] > measure_array[j])
               {
                  IS_marker[j] = 0;
               }
               else if (measure_array[j] > measure_array[i])
               {
                  IS_marker[i] = 0;
               }
            }
         }
         for (jS = S_offd_i[i]; jS < S_offd_i[i+1]; jS++)
         {
            j = local_num_vars+S_offd_j[jS];
            
            /* only consider valid graph edges */
            if ( (measure_array[j] > 1) && (S_offd_data[jS]) )
            {
               if (measure_array[i] > measure_array[j])
               {
                  IS_marker[j] = 0;
               }
               else if (measure_array[j] > measure_array[i])
               {
                  IS_marker[i] = 0;
               }
            }
         }
      }
    }
    else
    {
      if (measure_array[i] > 1)
      {
	 ic = i - local_num_vars;
         for (jS = S_ext_i[ic]; jS < S_ext_i[ic+1]; jS++)
         {
          j = S_ext_j[jS];
          if (j >= col_1 && j < col_n)
	  {
	    jc = j - col_1; 
            /* only consider valid graph edges */
            if ( (measure_array[jc] > 1) && (S_ext_data[jS]) )
            {
               if (measure_array[i] > measure_array[jc])
               {
                  IS_marker[jc] = 0;
               }
               else if (measure_array[jc] > measure_array[i])
               {
                  IS_marker[i] = 0;
               }
            }
	  }
	  else
	  {
	    for (jc = 0; jc < num_cols_offd; jc++)
	    {
	      if (j == col_map_offd[jc])
	      {
		jc += local_num_vars;
            /* only consider valid graph edges */
                if ((measure_array[jc] > 1) && (S_ext_data[jS]))
                {
                   if (measure_array[i] > measure_array[jc])
                   {
                      IS_marker[jc] = 0;
                   }
                   else if (measure_array[jc] > measure_array[i])
                   {
                      IS_marker[i] = 0;
                   }
                }
		break;
              }
            }
	  }
        }
      }
    }
   }
            
   return (ierr);
}
         

/*
//@HEADER
// *****************************************************************************
//
//  HPCGraph: Graph Computation on High Performance Computing Systems
//              Copyright (2016) Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions?  Contact  George M. Slota   (gmslota@sandia.gov)
//                      Siva Rajamanickam (srajama@sandia.gov)
//                      Kamesh Madduri    (madduri@cse.psu.edu)
//
// *****************************************************************************
//@HEADER
*/

#include <cstring>
#include <getopt.h>
#include <mpi.h>
#include <omp.h>
#include <time.h>

int  procid, nprocs;
bool verbose, debug, verify, output;

#include "comms.h"
#include "dist_graph.h"
#include "harmonic.h"
#include "io_pp.h"
#include "kcore.h"
#include "labelprop.h"
#include "pagerank.h"
#include "scc.h"
#include "wcc.h"

void
print_usage(char** argv)
{
  printf("To run: %s [graphfile] [options]\n", argv[0]);
  printf("\t Use -h for list of options\n\n");
}

void
print_usage_full(char** argv)
{
  printf("To run: %s [graphfile] [options]\n\n", argv[0]);
  printf("Options:\n");
  printf("\t-a\n");
  printf("\t\tRun all analytics\n");
  printf("\t-w\n");
  printf("\t\tRun weakly connected components\n");
  printf("\t-s\n");
  printf("\t\tRun strongly connected components\n");
  printf("\t-l\n");
  printf("\t\tRun label propagation\n");
  printf("\t-p\n");
  printf("\t\tRun PageRank\n");
  printf("\t-c\n");
  printf("\t\tRun harmonic centrality\n");
  printf("\t-k\n");
  printf("\t\tRun approximate k-core\n");
  printf("\t-o [outfile]\n");
  printf("\t\tAdjust output file [default: 'out.algorithm']\n");
  printf("\t-i [comma separated input vertex id list]\n");
  printf("\t\tVertex ids to analyze for harmonic centrality [default: none]\n");
  printf("\t-p [part file]\n");
  printf("\t\tPartition file to use for mapping vertices to tasks\n");
  printf("\t-t [#]\n");
  printf("\t\tAdjust iteration count for label prop, PageRank, k-core [default: 20]\n");
  printf("\t-f\n");
  printf("\t\tRun verification routines\n");
  printf("\t-v\n");
  printf("\t\tRun verification routines\n");
  printf("\t-d\n");
  printf("\t\tRun verification routines\n");
}

int
main(int argc, char** argv)
{
  srand(time(0));
  setbuf(stdout, 0);

  verbose = false;
  debug   = false;
  verify  = false;
  output  = false;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &procid);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

  if (argc < 2) {
    if (procid == 0)
      print_usage(argv);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Abort(MPI_COMM_WORLD, 1);
  }

  char* input_filename = strdup(argv[1]);

  bool run_scc = false;

  uint64_t  num_to_output = 0;
  char*     output_file   = NULL;
  char*     temp_out      = NULL;
  uint64_t* input_list    = NULL;
  char*     part_list     = NULL;

  graph_gen_data_t ggi;
  dist_graph_t     g;
  mpi_data_t       comm;
  queue_data_t     q;

  init_comm_data(&comm);
  load_graph_edges_32(input_filename, &ggi);

  if (nprocs > 1) {
    exchange_out_edges(&ggi, &comm);
    exchange_in_edges(&ggi, &comm);
    create_graph(&ggi, &g);
    relabel_edges(&g);
    if (part_list != NULL)
      repart_graph(&g, &comm, part_list);
  } else {
    create_graph_serial(&ggi, &g);
  }
  get_max_degree_vert(&g);
  init_queue_data(&g, &q);

  num_to_output = 1;
  input_list    = (uint64_t*)malloc(1 * sizeof(uint64_t));
  input_list[0] = g.max_degree_vert;

  if (num_to_output < 0)
    num_to_output = g.n;
  if (output_file == NULL)
    output_file = strdup("out");
  temp_out = (char*)malloc((strlen(output_file) + 128) * sizeof(char));

  if (run_scc) {
    temp_out[0] = '\0';
    strcat(temp_out, output_file);
    strcat(temp_out, ".scc");
    scc_dist(&g, &comm, &q, g.max_degree_vert, temp_out);
  }

  clear_graph(&g);
  clear_comm_data(&comm);
  clear_queue_data(&q);
  free(input_filename);
  free(temp_out);
  free(output_file);
  if (input_list != NULL)
    free(input_list);

  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Finalize();

  return 0;
}

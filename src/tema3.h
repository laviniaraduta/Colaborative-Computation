#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define N_CLUSTERS 4
#define TAG 0

void print_topology(int rank, int *coordinators, int procs);
void spread_coordinators(int rank, int *coordinators, int *coordinator, int *num_workers, int **workers);
void complete_topology_no_error(int rank, int *coordinators, int procs, int num_workers, int *workers);
void spread_topology_no_error(int rank, int *coordinators, int coordinator, int procs, int num_workers, int *workers);
void spread_work_no_error(
    int rank, int coordinator,
    double work_size, int num_workers,
    int *workers, int N,
    int **to_compute, int *index);
int get_partial_array(int rank, int num_workers, int N, double work_size, int *workers, int index, int *to_compute);


void combine_results(int rank, int N, int *to_compute, int start_index);

void complete_topology_error(int rank, int *coordinators, int procs, int num_workers, int *workers);
void spread_topology_error(int rank, int *coordinators, int coordinator, int procs, int num_workers, int *workers);
void spread_work_error(
    int rank, int coordinator,
    double work_size, int num_workers,
    int *workers, int N,
    int **to_compute, int *index);
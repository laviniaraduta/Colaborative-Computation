#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "tema3.h"

int main (int argc, char *argv[])
{
    int procs, rank;
    int N = -1, error_type, coordinator = -1;
    int *coordinators, *to_compute = NULL;
    int num_workers = 0, *workers = NULL, index = 0, n = 0, start_index = 0;

    MPI_Status status;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // inca nu se stiu coordonatorii pentru fiecare proces
    coordinators = (int *)malloc(procs * sizeof(int));
    // recv_coordinators = (int *)malloc(procs * sizeof(int));
    for (int i = 0; i < procs; i++) {
        coordinators[i] = -1;
    }

    if (argc != 3) {
        printf("Error, number of params - 2\n");
        MPI_Finalize();
        exit(0);
    } else {
        if (rank == 0) {
            N = atoi(argv[1]);
        }
        error_type = atoi(argv[2]);
        // printf("N = %d, error_type = %d\n", N, error_type);
    }
    
    spread_coordinators_no_error(rank, coordinators, &coordinator, &num_workers, &workers);
    complete_topology_no_error(rank, coordinators, procs, num_workers, workers);
    spread_topology_no_error(rank, coordinators, coordinator, procs, num_workers, workers);
    
    if (rank >= 0 && rank < N_CLUSTERS) {
        if (rank == 0) {
            MPI_Send(&N, 1, MPI_INT, N_CLUSTERS - 1, TAG, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, N_CLUSTERS - 1);
        } else if (rank == N_CLUSTERS - 1) {
            MPI_Recv(&N, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD, &status);
            MPI_Send(&N, 1, MPI_INT, rank - 1, TAG, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank - 1);
        } else {
            MPI_Recv(&N, 1, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD, &status);
            if (rank != 1) {
                MPI_Send(&N, 1, MPI_INT, rank - 1, TAG, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, rank - 1);
            }
        }
    }
    
    double work_size = (double)N / (procs - N_CLUSTERS);

    spread_work_no_error(rank, coordinator, work_size, num_workers, workers, N, &to_compute, &index);
    // MPI_Barrier(MPI_COMM_WORLD);
    start_index = get_partial_array_no_error(rank, num_workers, N, work_size, workers, index, to_compute);
    // MPI_Barrier(MPI_COMM_WORLD);
    combine_results_no_error(rank, N, to_compute, start_index);

    MPI_Finalize();

    if (rank >= 0 && rank < N_CLUSTERS) {
        free(workers);
    }
    free(coordinators);
}




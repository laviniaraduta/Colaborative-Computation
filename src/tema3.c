#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "tema3.h"

int count_workers(int *coordinators, int procs)
{
    int num_workers = 0;
    for (int i = 0; i < procs; i++) {
        if (coordinators[i] != -1) {
            num_workers++;
        }
    }
    return num_workers;
}

void print_topology(int rank, int *coordinators, int procs) {
    printf("%d -> ", rank);
    for (int i = 0; i < N_CLUSTERS; i++) {
        int count = 0;
        char workers[100] = "";
        for (int j = 0; j < procs; j++) {
            if (coordinators[j] == i) {
                if (count == 0) {
                    sprintf(workers, "%s%d", workers, j);
                } else {
                    sprintf(workers, "%s,%d", workers, j);
                }
                count++;
            }
        }
        if (count != 0) {
            printf("%d:", i);
            printf("%s " , workers);
        }
    }
    printf("\n");
}

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

    // inca nu se stiu coordonatorii pentru fiecare proces, deci completez cu -1
    coordinators = (int *)malloc(procs * sizeof(int));
    for (int i = 0; i < procs; i++) {
        coordinators[i] = -1;
    }

    if (argc != 3) {
        printf("Error, number of params - 2\n");
        MPI_Finalize();
        exit(0);
    } else {
        if (rank == MASTER) {
            N = atoi(argv[1]);
        }
        error_type = atoi(argv[2]);
    }
    
    int total_workers = 0;
    double work_size = 0;
    
    if (error_type == 0) {
        spread_N(rank, &N);
        spread_coordinators(rank, coordinators, &coordinator, &num_workers, &workers);
        complete_topology_no_error(rank, coordinators, procs, num_workers, workers);
        spread_topology_no_error(rank, coordinators, coordinator, procs, num_workers, workers);
        
        total_workers = count_workers(coordinators, procs);
 
        if (rank >= MASTER && rank < N_CLUSTERS) {
            work_size = (double)N / total_workers;
        }
        spread_work_no_error(rank, coordinator, work_size, num_workers, workers, N, &to_compute, &index);
        start_index = get_partial_array(rank, num_workers, N, work_size, workers, index, to_compute);
        combine_results(rank, N, to_compute, start_index);

    } else if (error_type == 1) {
        spread_N(rank, &N);
        spread_coordinators(rank, coordinators, &coordinator, &num_workers, &workers);
        complete_topology_error(rank, coordinators, procs, num_workers, workers);
        spread_topology_error(rank, coordinators, coordinator, procs, num_workers, workers);
        
        total_workers = count_workers(coordinators, procs);
        if (rank >= MASTER && rank < N_CLUSTERS) {
            work_size = (double)N / total_workers;
        }

        spread_work_error(rank, coordinator, work_size, num_workers, workers, N, &to_compute, &index);
        start_index = get_partial_array(rank, num_workers, N, work_size, workers, index, to_compute);
        combine_results(rank, N, to_compute, start_index);

    } else if (error_type == 2) {
        spread_N_partition(rank, &N);
        spread_coordinators(rank, coordinators, &coordinator, &num_workers, &workers);
        if (rank == OUT_CLUSTER) {
            print_topology(rank, coordinators, procs);
        }
        complete_topology_partition(rank, coordinators, procs, num_workers, workers);
        spread_topology_partition(rank, coordinators, coordinator, procs, num_workers, workers);

        total_workers = count_workers(coordinators, procs);
        if (rank >= MASTER && rank < N_CLUSTERS) {
            work_size = (double)N / total_workers;
        }
        if (rank != OUT_CLUSTER) {
            spread_work_partition(rank, coordinator, work_size, num_workers, workers, N, &to_compute, &index);
            start_index = get_partial_array(rank, num_workers, N, work_size, workers, index, to_compute);
            combine_results_partition(rank, N, to_compute, start_index);
        }
    } else {
        printf("Error, wrong error type\n");
    }
    
    MPI_Finalize();

    if (rank >= MASTER && rank < N_CLUSTERS) {
        free(workers);
    }
    free(coordinators);
}




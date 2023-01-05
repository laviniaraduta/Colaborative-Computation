#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "tema3.h"

/* Clusterul 0 este deconectat de celelalte clustere */

// coordonatorul 0 trimite numarul N din argumentele programului
void spread_N_partition(int rank, int *N) {
    MPI_Status status;
    if (rank >= MASTER && rank < N_CLUSTERS && rank != OUT_CLUSTER) {
        if (rank == MASTER) {
            MPI_Send(N, 1, MPI_INT, N_CLUSTERS - 1, TAG, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, N_CLUSTERS - 1);
            fflush(stdout);
        } else if (rank == N_CLUSTERS - 1) {
            MPI_Recv(N, 1, MPI_INT, MASTER, TAG, MPI_COMM_WORLD, &status);
            MPI_Send(N, 1, MPI_INT, rank - 1, TAG, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank - 1);
            fflush(stdout);
        } else {
            MPI_Recv(N, 1, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD, &status);
            if (rank != 2) {
                MPI_Send(N, 1, MPI_INT, rank - 1, TAG, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, rank - 1);
                fflush(stdout);
            }
        }
    }
}

// procesul 2 incepe sa trimita vectorul sau de coordonatori spre 3
// procesele completeaza nodurile pentru care sunt ele coordonatori si 
// trimit mai departe spre 0 unde se afisaza prima topologie
// procesul 1 nu primeste si nu trimte nimic, fiind izolat
void complete_topology_partition(int rank, int *coordinators, int procs, int num_workers, int *workers) {
    MPI_Status status;
    if (rank >= MASTER && rank < N_CLUSTERS) {
        if (rank == MASTER) {
            MPI_Recv(coordinators, procs, MPI_INT, N_CLUSTERS - 1, TAG, MPI_COMM_WORLD, &status);
            for (int i = 0; i < num_workers; i++) {
                coordinators[workers[i]] = rank;
            }
            print_topology(rank, coordinators, procs);
        } else if (rank == 2) {
            MPI_Send(coordinators, procs, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank + 1);
            fflush(stdout); 
        } else if (rank == N_CLUSTERS - 1) {
            MPI_Recv(coordinators, procs, MPI_INT, rank - 1, TAG, MPI_COMM_WORLD, &status);
            for (int i = 0; i < num_workers; i++) {
                coordinators[workers[i]] = rank;
            }
            MPI_Send(coordinators, procs, MPI_INT, MASTER, TAG, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, MASTER);
            fflush(stdout); 
        }
    }
}

// coordonatorul 0 trimite topologia completa spre 3 cu destinatia 2
// iar fiecare coordonator o trimite workerilor lui
void spread_topology_partition(int rank, int *coordinators, int coordinator, int procs, int num_workers, int *workers) {
    MPI_Status status;
    if (rank >= MASTER && rank < N_CLUSTERS) {
        if (rank == MASTER) {
            MPI_Send(coordinators, procs, MPI_INT, N_CLUSTERS - 1, TAG, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, N_CLUSTERS - 1);
            fflush(stdout);
        } else if (rank != OUT_CLUSTER) {
            if (rank == N_CLUSTERS - 1) {
                MPI_Recv(coordinators, procs, MPI_INT, MASTER, TAG, MPI_COMM_WORLD, &status);
                print_topology(rank, coordinators, procs);
            } else {
                MPI_Recv(coordinators, procs, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD, &status);
                print_topology(rank, coordinators, procs);
            }
            
            if (rank != 2) {
                MPI_Send(coordinators, procs, MPI_INT, rank - 1, TAG, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, rank - 1);
                fflush(stdout);
            }
        }

        for (int i = 0; i < num_workers; i++) {
            MPI_Send(coordinators, procs, MPI_INT, workers[i], TAG, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, workers[i]);
            fflush(stdout);
        }
    
    } else {
        MPI_Recv(coordinators, procs, MPI_INT, coordinator, TAG, MPI_COMM_WORLD, &status);
        print_topology(rank, coordinators, procs);
    }
}

// trimit apoi vectorul pe care se aplica inmultirile
// trimit apoi cati workeri au primit deja sarcinile ca sa stiu indexul de start
// doar workerii care nu aprtin clusterului 1 participa la inmultiri si primesc
// doar bucata de vector de care se ocupa
void spread_work_partition(int rank, int coordinator,
    double work_size, int num_workers, int *workers,
    int N, int **to_compute, int *index) {
    int n = 0;
    MPI_Status status;

    if (rank >= MASTER && rank < N_CLUSTERS) {
        if (rank == MASTER) {
            *to_compute = (int *)malloc(N * sizeof(int));
            for (int i = 0; i < N; i++) {
                (*to_compute)[i] = N - i - 1;
            }
            MPI_Send(*to_compute, N, MPI_INT, N_CLUSTERS - 1, TAG, MPI_COMM_WORLD);
            MPI_Send(&num_workers, 1, MPI_INT, N_CLUSTERS - 1, TAG, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, N_CLUSTERS - 1);
            fflush(stdout);
        } else {
            *to_compute = (int *)malloc(N * sizeof(int));
            if (rank == N_CLUSTERS - 1) {
                MPI_Recv(*to_compute, N, MPI_INT, MASTER, TAG, MPI_COMM_WORLD, &status);
                MPI_Recv(index, 1, MPI_INT, MASTER, TAG, MPI_COMM_WORLD, &status);
            } else {
                MPI_Recv(*to_compute, N, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD, &status);
                MPI_Recv(index, 1, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD, &status);
            }

            if (rank != 2) {
                MPI_Send(*to_compute, N, MPI_INT, rank - 1, TAG, MPI_COMM_WORLD);
                int new_index = *index + num_workers;
                MPI_Send(&new_index, 1, MPI_INT, rank - 1, TAG, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, rank - 1);
                fflush(stdout);
            }
        }
        for (int i = 0; i < num_workers; i++) {
            int idx = *index + i;
            int start = idx * work_size;
            int end = fmin((idx + 1) * work_size, N);
            n = end - start;
            MPI_Send(&n, 1, MPI_INT, workers[i], TAG, MPI_COMM_WORLD);
            MPI_Send(*to_compute + start, n, MPI_INT, workers[i], TAG, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, workers[i]);
            fflush(stdout);
        }
    } else if (coordinator != OUT_CLUSTER) {
        MPI_Recv(&n, 1, MPI_INT, coordinator, TAG, MPI_COMM_WORLD, &status);
        int *recv = (int *)malloc(n * sizeof(int));
        MPI_Recv(recv, n, MPI_INT, coordinator, TAG, MPI_COMM_WORLD, &status);
        for (int i = 0; i < n; i++) {
            recv[i] *= 5;
        }
        MPI_Send(&n, 1, MPI_INT, coordinator, TAG, MPI_COMM_WORLD);
        MPI_Send(recv, n, MPI_INT, coordinator, TAG, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, coordinator);
        fflush(stdout);
    }
}


// procesul 0 primeste toate rezultatele partiale (de la 2 si 3) si le combina
void combine_results_partition(int rank, int N, int *to_compute, int start_index) {
    int *recv_compute;
    MPI_Status status;
    if (rank >= MASTER && rank < N_CLUSTERS) {
        if (rank == MASTER) {
            recv_compute = (int *)malloc(N * sizeof(int));
            for (int i = 2; i < N_CLUSTERS; i++) {
                MPI_Recv(recv_compute, N, MPI_INT, N_CLUSTERS - 1, TAG, MPI_COMM_WORLD, &status);
            
                for (int j = start_index; j < N; j++) {
                    if (to_compute[j] == N - j - 1) {
                        to_compute[j] = recv_compute[j];
                    }
                }
            }
            printf("Rezultat: ");
            for (int i = 0; i < N; i++) {
                printf("%d ", to_compute[i]);
            }
            printf("\n");
            fflush(stdout);

        } else {
            recv_compute = (int *)malloc(N * sizeof(int));  
            for (int i = 2; i < rank; i++) {
                MPI_Recv(recv_compute, N, MPI_INT, rank - 1, TAG, MPI_COMM_WORLD, &status);
                if (rank == N_CLUSTERS - 1) {
                    MPI_Send(recv_compute, N, MPI_INT, MASTER, TAG, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, MASTER);
                    fflush(stdout); 
                } else {
                    MPI_Send(recv_compute, N, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, rank + 1);
                    fflush(stdout);
                }
            }
            if (rank == N_CLUSTERS - 1) {
                MPI_Send(to_compute, N, MPI_INT, MASTER, TAG, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, MASTER);
                fflush(stdout);
            } else {
                MPI_Send(to_compute, N, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, rank + 1);
                fflush(stdout);
            }
        }
    }
}

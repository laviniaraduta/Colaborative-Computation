#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "tema3.h"

/* NU exista conectivitate intre coordonatorii 0 si 1 */

// procesul 1 incepe sa trimita vectorul sau de coordonatori spre 2 (1-2-3-0)
// procesele completeaza nodurile pentru care sunt ele coordonatori si 
// trimit mai departe spre 0 unde se afisaza prima topologie
void complete_topology_error(int rank, int *coordinators, int procs, int num_workers, int *workers) {
    MPI_Status status;
    if (rank >= MASTER && rank < N_CLUSTERS) {
        if (rank == MASTER) {
            MPI_Recv(coordinators, procs, MPI_INT, N_CLUSTERS - 1, TAG, MPI_COMM_WORLD, &status);
            for (int i = 0; i < num_workers; i++) {
                coordinators[workers[i]] = rank;
            }
            print_topology(rank, coordinators, procs);
        } else if (rank == 1) {
            MPI_Send(coordinators, procs, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank + 1);
            fflush(stdout); 
        } else {
            MPI_Recv(coordinators, procs, MPI_INT, rank - 1, TAG, MPI_COMM_WORLD, &status);
            for (int i = 0; i < num_workers; i++) {
                coordinators[workers[i]] = rank;
            }
            if (rank == N_CLUSTERS - 1) {
                MPI_Send(coordinators, procs, MPI_INT, MASTER, TAG, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, MASTER);
                fflush(stdout); 
            } else {
                MPI_Send(coordinators, procs, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, rank + 1);
                fflush(stdout);
            }
        }
    }
}

// coordonatorul 0 are topologia completa si o trimite workerilor lui
// si mai apoi catre 3, urmand sa ajunga pas cu pas catre 1 
// fiecare coordonator trimite topologia primita workerilor lui
void spread_topology_error(int rank, int *coordinators, int coordinator, int procs, int num_workers, int *workers) {
    MPI_Status status;
    if (rank >= MASTER && rank < N_CLUSTERS) {
        if (rank == MASTER) {
            MPI_Send(coordinators, procs, MPI_INT, N_CLUSTERS - 1, TAG, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, N_CLUSTERS - 1);
            fflush(stdout);
        } else {
            if (rank == N_CLUSTERS - 1) {
                MPI_Recv(coordinators, procs, MPI_INT, MASTER, TAG, MPI_COMM_WORLD, &status);
                print_topology(rank, coordinators, procs);
            } else {
                MPI_Recv(coordinators, procs, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD, &status);
                print_topology(rank, coordinators, procs);
            }
            
            if (rank != 1) {
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

// procesul 0 genereaza vectorul pe care o sa se aplice inmultirile
// vectorul este trimis 0-3-2-1
// pentru a sti de unde sa imparta vectorul, fiecare coordonator primeste si
// indexul global al primului worker din clusterul lui 
// fiecare worker primeste doar partea sa din vector pe care o prelucreaza
// si o trimit inapoi coordonatorului
void spread_work_error(int rank, int coordinator,
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

            if (rank != 1) {
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
    } else {
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

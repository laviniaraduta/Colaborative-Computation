#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "tema3.h"

// procesul 1 incepe sa trimita vectorul sau de coordonatori spre 2
// procesele completeaza nodurile pentru care sunt ele coordonatori si 
// trimit mai departe spre 0 unde se afisaza prima topologie
void complete_topology_error(int rank, int *coordinators, int procs, int num_workers, int *workers) {
    MPI_Status status;
    if (rank >= 0 && rank < N_CLUSTERS) {
        if (rank == 0) {
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
                MPI_Send(coordinators, procs, MPI_INT, 0, TAG, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, 0);
                fflush(stdout); 
            } else {
                MPI_Send(coordinators, procs, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, rank + 1);
                fflush(stdout);
            }
        }
    }
}

// coordonatorul 0 trimite topologia completa spre 3 cu destinatia 1
// iar fiecare coordonator o trimite workerilor lui
void spread_topology_error(int rank, int *coordinators, int coordinator, int procs, int num_workers, int *workers) {
    MPI_Status status;
    if (rank >= 0 && rank < N_CLUSTERS) {
        if (rank == 0) {
            MPI_Send(coordinators, procs, MPI_INT, N_CLUSTERS - 1, TAG, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, N_CLUSTERS - 1);
            fflush(stdout);
            for (int i = 0; i < num_workers; i++) {
                MPI_Send(coordinators, procs, MPI_INT, workers[i], TAG, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, workers[i]);
                fflush(stdout);
            }
        } else {
            if (rank == N_CLUSTERS - 1) {
                MPI_Recv(coordinators, procs, MPI_INT, 0, TAG, MPI_COMM_WORLD, &status);
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

            for (int i = 0; i < num_workers; i++) {
                MPI_Send(coordinators, procs, MPI_INT, workers[i], TAG, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, workers[i]);
                fflush(stdout);
            }
        } 
    } else {
        MPI_Recv(coordinators, procs, MPI_INT, coordinator, TAG, MPI_COMM_WORLD, &status);
        print_topology(rank, coordinators, procs);
    }
}

// trimit apoi vectorul pe care se aplica inmultirile
// trimit apoi cati workeri au primit deja sarcinile ca sa stiu indexul de start
void spread_work_error(
    int rank, int coordinator,
    double work_size, int num_workers,
    int *workers, int N,
    int **to_compute, int *index) {
    int n = 0;
    MPI_Status status;

    if (rank >= 0 && rank < N_CLUSTERS) {
        if (rank == 0) {
            *to_compute = (int *)malloc(N * sizeof(int));
            for (int i = 0; i < N; i++) {
                (*to_compute)[i] = N - i - 1;
            }
            // printf(N = %d, procs = %d Numarul de calcule: %d\n", N, procs, N / (procs - 4));
            // trimite mai departe catre coordonatorul 1 vectorul de numere
            MPI_Send(*to_compute, N, MPI_INT, N_CLUSTERS - 1, TAG, MPI_COMM_WORLD);
            MPI_Send(&num_workers, 1, MPI_INT, N_CLUSTERS - 1, TAG, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, N_CLUSTERS - 1);
            fflush(stdout);
        } else {
            // coordonatorii primesc de la coordonatorul anterior N, vectorul de numere
            // si numarul de workeri care au primit deja sarcini (indexul de start)
            // si trimit mai departe
            *to_compute = (int *)malloc(N * sizeof(int));
            if (rank == N_CLUSTERS - 1) {
                MPI_Recv(*to_compute, N, MPI_INT, 0, TAG, MPI_COMM_WORLD, &status);
                MPI_Recv(index, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD, &status);
            } else {
                MPI_Recv(*to_compute, N, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD, &status);
                MPI_Recv(index, 1, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD, &status);

            }
            // trimit mai departe catre coordonatorul urmator vectorul de numere
            if (rank != 1) {
                MPI_Send(*to_compute, N, MPI_INT, rank - 1, TAG, MPI_COMM_WORLD);
                int new_index = *index + num_workers;
                MPI_Send(&new_index, 1, MPI_INT, rank - 1, TAG, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, rank - 1);
                fflush(stdout);
            }
        }
        // trimit workerilor numarul de numere pe care trebuie sa aplice inmultirea
        // si bucata lor de vector
        for (int i = 0; i < num_workers; i++) {
            int idx = *index + i;
            int start = idx * work_size;
            int end = fmin((idx + 1) * work_size, N);
            n = end - start;
            // printf("worker = %d - start = %d, end = %d\n",workers[i], start, end);
            MPI_Send(&n, 1, MPI_INT, workers[i], TAG, MPI_COMM_WORLD);
            MPI_Send(*to_compute + start, n, MPI_INT, workers[i], TAG, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, workers[i]);
            fflush(stdout);
        }
    } else {
        // workerii primesc de la coordonatorul lor numarul de calcule
        // si fac calculele
        // apoi trimit inapoi coordonatorului rezultatul
        MPI_Recv(&n, 1, MPI_INT, coordinator, TAG, MPI_COMM_WORLD, &status);
        // printf("sunt worker %d si primesc %d numere de la %d\n", rank, n, coordinator); 
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

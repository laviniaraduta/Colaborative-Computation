#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "tema3.h"

/* Exista conectivitate intre coordonatorii 0 si 1 */


// procesul 0 este singurul care stie cat este N la inceput
// asa ca il trimite in inel catre ceilalti coordonatori
// il trimite 0-3-2-1 (pt a putea fi folosit si in cazul intreruperii 0-1)
void spread_N(int rank, int *N) {
    MPI_Status status;
    if (rank >= MASTER && rank < N_CLUSTERS) {
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
            if (rank != 1) {
                MPI_Send(N, 1, MPI_INT, rank - 1, TAG, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, rank - 1);
                fflush(stdout);
            }
        }
    }
}

// coordonatorii citesc fisierele si afla proprii workeri, apoi completeaza 
// vectorul de coordonatori cu propriile informatii
// workerii afla ce proces este coordonatorul lor
void spread_coordinators(int rank, int *coordinators, int *coordinator, int *num_workers, int **workers) {
    MPI_Status status;
    if (rank >= MASTER && rank < N_CLUSTERS) {
        FILE *fp;
        char input_file_name[13];
        sprintf(input_file_name, "cluster%d.txt", rank);
        fp = fopen(input_file_name, "r");
	    fscanf(fp, "%d", num_workers);

        *workers = (int *)malloc(*num_workers * sizeof(int));
        for (int i = 0; i < *num_workers; i++) {
            fscanf(fp, "%d", &(*workers)[i]);
            MPI_Send(&rank, 1, MPI_INT, (*workers)[i], TAG, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, (*workers)[i]);
            coordinators[(*workers)[i]] = rank;
        }

        fclose(fp);
    } else {
        MPI_Recv(coordinator, 1, MPI_INT, MPI_ANY_SOURCE, TAG, MPI_COMM_WORLD, &status);
    }
}

// incepe completarea topologiei de catre fiecare coordonator
// coordonatorul 0 trimite in inel topologia proprie (0-1-2-3-0), care e completata pas cu pas
// cand vectorul de coordonatori ajunge inapoi la 0, topologia e completa si poate fi afisata
void complete_topology_no_error(int rank, int *coordinators, int procs, int num_workers, int *workers) {
    MPI_Status status;
    if (rank >= MASTER && rank < N_CLUSTERS) {
        if (rank == MASTER) {
            MPI_Send(coordinators, procs, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank + 1);
            MPI_Recv(coordinators, procs, MPI_INT, N_CLUSTERS - 1, TAG, MPI_COMM_WORLD, &status);
            print_topology(rank, coordinators, procs);
        } else {
            MPI_Recv(coordinators, procs, MPI_INT, rank - 1, TAG, MPI_COMM_WORLD, &status);
            for (int i = 0; i < num_workers; i++) {
                coordinators[workers[i]] = rank;
            }
            if (rank == N_CLUSTERS - 1) {
                MPI_Send(coordinators, procs, MPI_INT, MASTER, TAG, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, MASTER);
            } else {
                MPI_Send(coordinators, procs, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, rank + 1);
            }
        }
    }
}

// cand are topologia completa procesul 0 o trimite catre workerii lui si catre 
// restul de coordonatori, respactand ordinea din inel
// fiecare coordonator trimitr topologia workerilor lui
void spread_topology_no_error(int rank, int *coordinators, int coordinator, int procs, int num_workers, int *workers) {
    MPI_Status status;
    if (rank >= MASTER && rank < N_CLUSTERS) {
        if (rank == MASTER) {
            MPI_Send(coordinators, procs, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank + 1);
        } else {
            MPI_Recv(coordinators, procs, MPI_INT, rank - 1, TAG, MPI_COMM_WORLD, &status);
            print_topology(rank, coordinators, procs);

            if (rank != N_CLUSTERS - 1) {
                MPI_Send(coordinators, procs, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, rank + 1);
            }

        } 
        for (int i = 0; i < num_workers; i++) {
            MPI_Send(coordinators, procs, MPI_INT, workers[i], TAG, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, workers[i]);
        }
    } else {
        MPI_Recv(coordinators, procs, MPI_INT, coordinator, TAG, MPI_COMM_WORLD, &status);
        print_topology(rank, coordinators, procs);
    }
}

// procesul 0 genereaza vectorul pe care o sa se aplice inmultirile
// vectorul este trimis apoi respactand ordinea din inel catre toti coordonatorii
// pentru a sti de unde sa imparta vectorul, fiecare coordonator primeste si
// indexul global al primului worker din clusterul lui 
// fiecare worker primeste doar partea sa din vector pe care o prelucreaza
// si o trimit inapoi coordonatorului
void spread_work_no_error(int rank, int coordinator,
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
            MPI_Send(*to_compute, N, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD);
            MPI_Send(&num_workers, 1, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank + 1);
        } else {
            *to_compute = (int *)malloc(N * sizeof(int));
            MPI_Recv(*to_compute, N, MPI_INT, rank - 1, TAG, MPI_COMM_WORLD, &status);
            MPI_Recv(index, 1, MPI_INT, rank - 1, TAG, MPI_COMM_WORLD, &status);

            if (rank != N_CLUSTERS - 1) {
                MPI_Send(*to_compute, N, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD);
                int new_index = *index + num_workers;
                MPI_Send(&new_index, 1, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, rank + 1);
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
    }
}

// fiecare coordonator primeste rezultatele de la workerii lui si completeaza 
// un vector cu rezultate partiale, pe care il va trimite catre master
int get_partial_array(int rank, int num_workers, int N, double work_size, int *workers, int index, int *to_compute) {
    MPI_Status status;
    int n, start_index = 0;
    if (rank >= MASTER && rank < N_CLUSTERS) {
        int count = index * work_size;
        for (int i = 0; i < num_workers; i++) {
            MPI_Recv(&n, 1, MPI_INT, workers[i], TAG, MPI_COMM_WORLD, &status);
            if (rank == MASTER) {
                start_index += n;
            }
            int *recv = (int *)malloc(n * sizeof(int)); 
            MPI_Recv(recv, n, MPI_INT, workers[i], TAG, MPI_COMM_WORLD, &status);
            for (int j = 0; j < n; j++) {
                to_compute[count++] = recv[j];
            }
        }
    }
    return start_index;
}


// procesul 0 primeste toate rezultatele partiale si le combina
void combine_results(int rank, int N, int *to_compute, int start_index) {
    int *recv_compute;
    MPI_Status status;
    if (rank >= MASTER && rank < N_CLUSTERS) {
        if (rank == MASTER) {
            recv_compute = (int *)malloc(N * sizeof(int));
            for (int i = 1; i < N_CLUSTERS; i++) {
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
            for (int i = 1; i < rank; i++) {
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

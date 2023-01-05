#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "tema3.h"

void print_topology(int rank, int *coordinators, int procs) {
    printf("%d -> ", rank);
    for (int i = 0; i < N_CLUSTERS; i++) {
        printf("%d:", i);
        int count = 0;
        for (int j = 0; j < procs; j++) {
            if (coordinators[j] == i) {
                if (count == 0) {
                    printf("%d", j);
                } else {
                    printf(",%d", j);
                }
                count++;
            }
        }
        printf(" ");
    }
    printf("\n");
}

void spread_coordinators(int rank, int *coordinators, int *coordinator, int *num_workers, int **workers) {
    MPI_Status status;
    int aux;
    if (rank >= 0 && rank < N_CLUSTERS) {
        // coordonatorii citesc fisierele
        // anunta workerii ca ei sunt coordonatorii lor
        // completeaza vectorul de coordonatori
        FILE *fp;
        char input_file_name[13];
        sprintf(input_file_name, "cluster%d.txt", rank);
        // printf("Coordonator %d, input_file_name = %s\n", rank, input_file_name);
        fp = fopen(input_file_name, "r");
	    fscanf(fp, "%d", &aux);
        printf("%d\n", aux);
        *num_workers = aux;
        // printf("Coordonator %d, num_workers = %d\n", rank, num_workers);

        *workers = (int *)malloc(*num_workers * sizeof(int));
        for (int i = 0; i < *num_workers; i++) {
            fscanf(fp, "%d", &aux);
            (*workers)[i] = aux;
            MPI_Send(&rank, 1, MPI_INT, (*workers)[i], 0, MPI_COMM_WORLD);
            coordinators[(*workers)[i]] = rank;
            printf("M(%d,%d)\n", rank, (*workers)[i]);
            // printf("Coordonator %d, workers[%d] = %d\n", rank, i, workers[i]);
        }

        fclose(fp);
    } else {
        MPI_Recv(&aux, 1, MPI_INT, MPI_ANY_SOURCE, TAG, MPI_COMM_WORLD, &status);
        *coordinator = aux;
        // printf("Worker %d, coordonator = %d\n", rank, *coordinator);
    }
}

// se trimite in inel vectorul de coordonatori care se completeaza
// fiecare coordonator se trece ca fiind coordonator pentru fiecare worker de-al sau
// cand ajunge la coordonatorul 0 ar trebui ca toti coordonatorii sa fi trimis
// lista de workeri de care au grija
void complete_topology_no_error(int rank, int *coordinators, int procs, int num_workers, int *workers) {
    MPI_Status status;
    if (rank >= 0 && rank < N_CLUSTERS) {
        if (rank == 0) {
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
                MPI_Send(coordinators, procs, MPI_INT, 0, TAG, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, 0);
            } else {
                MPI_Send(coordinators, procs, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, rank + 1);
            }
        }
    }
}

void spread_topology_no_error(int rank, int *coordinators, int coordinator, int procs, int num_workers, int *workers) {
    // coordonatorul 0 trimite apoi in inel topologia completa
    // iar fiecare coordonator o trimite workerilor lui
    MPI_Status status;
    if (rank >= 0 && rank < N_CLUSTERS) {
        if (rank == 0) {
            MPI_Send(coordinators, procs, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank + 1);
            for (int i = 0; i < num_workers; i++) {
                MPI_Send(coordinators, procs, MPI_INT, workers[i], TAG, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, workers[i]);
            }
        } else {
            MPI_Recv(coordinators, procs, MPI_INT, rank - 1, TAG, MPI_COMM_WORLD, &status);
            print_topology(rank, coordinators, procs);

            if (rank != N_CLUSTERS - 1) {
                MPI_Send(coordinators, procs, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, rank + 1);
            }

            for (int i = 0; i < num_workers; i++) {
                MPI_Send(coordinators, procs, MPI_INT, workers[i], TAG, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, workers[i]);
            }
        } 
    } else {
        MPI_Recv(coordinators, procs, MPI_INT, coordinator, TAG, MPI_COMM_WORLD, &status);
        print_topology(rank, coordinators, procs);
    }
}

// trimit apoi vectorul pe care se aplica inmultirile
// trimit apoi cati workeri au primit deja sarcinile ca sa stiu indexul de start
void spread_work_no_error(
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
            MPI_Send(*to_compute, N, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD);
            MPI_Send(&num_workers, 1, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank + 1);
        } else {
            // coordonatorii primesc de la coordonatorul anterior N, vectorul de numere
            // si numarul de workeri care au primit deja sarcini (indexul de start)
            // si trimit mai departe
            *to_compute = (int *)malloc(N * sizeof(int));
            MPI_Recv(*to_compute, N, MPI_INT, rank - 1, TAG, MPI_COMM_WORLD, &status);
            MPI_Recv(index, 1, MPI_INT, rank - 1, TAG, MPI_COMM_WORLD, &status);
            // trimit mai departe catre coordonatorul urmator vectorul de numere
            if (rank != N_CLUSTERS - 1) {
                MPI_Send(*to_compute, N, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD);
                int new_index = *index + num_workers;
                MPI_Send(&new_index, 1, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, rank + 1);
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
    }
}

int get_partial_array(int rank, int num_workers, int N, double work_size, int *workers, int index, int *to_compute) {
    MPI_Status status;
    int n, start_index = 0;
    // fiecare coordonator primeste de la workeri rezultatele partiale
    if (rank >= 0 && rank < N_CLUSTERS) {
        int count = index * work_size;
        for (int i = 0; i < num_workers; i++) {
            MPI_Recv(&n, 1, MPI_INT, workers[i], TAG, MPI_COMM_WORLD, &status);
            if (rank == 0) {
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

void combine_results(int rank, int N, int *to_compute, int start_index) {
    // procesul 0 primeste toate rezultatele partiale si le combina
    int *recv_compute;
    MPI_Status status;
    if (rank >= 0 && rank < N_CLUSTERS) {
        if (rank == 0) {
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
                    MPI_Send(recv_compute, N, MPI_INT, 0, TAG, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, 0);
                    fflush(stdout); 
                } else {
                    MPI_Send(recv_compute, N, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, rank + 1);
                    fflush(stdout);
                }
            }
            if (rank == N_CLUSTERS - 1) {
                MPI_Send(to_compute, N, MPI_INT, 0, TAG, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, 0);
                fflush(stdout);
            } else {
                MPI_Send(to_compute, N, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, rank + 1);
                fflush(stdout);
            }
        }
    }
}

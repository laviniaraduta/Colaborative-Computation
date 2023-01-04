#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define N_CLUSTERS 4
#define TAG 0

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
    if (rank >= 0 && rank < N_CLUSTERS) {
        // coordonatorii citesc fisierele
        // anunta workerii ca ei sunt coordonatorii lor
        // completeaza vectorul de coordonatori
        FILE *fp;
        char input_file_name[13];
        

        sprintf(input_file_name, "cluster%d.txt", rank);
        // printf("Coordonator %d, input_file_name = %s\n", rank, input_file_name);
        fp = fopen(input_file_name, "r");
	    fscanf(fp, "%d", num_workers);
        // printf("Coordonator %d, num_workers = %d\n", rank, num_workers);

        *workers = (int *)malloc(*num_workers * sizeof(int));
        for (int i = 0; i < *num_workers; i++) {
            fscanf(fp, "%d", &(*workers)[i]);
            MPI_Send(&rank, 1, MPI_INT, (*workers)[i], 0, MPI_COMM_WORLD);
            coordinators[(*workers)[i]] = rank;
            printf("M(%d,%d)\n", rank, (*workers)[i]);
            // printf("Coordonator %d, workers[%d] = %d\n", rank, i, workers[i]);
        }

        fclose(fp);
    } else {
        MPI_Recv(coordinator, 1, MPI_INT, MPI_ANY_SOURCE, TAG, MPI_COMM_WORLD, &status);
        // printf("Worker %d, coordonator = %d\n", rank, coordonator);
    }
}

// se trimite in inel vectorul de coordonatori care se completeaza
// fiecare coordonator se trece ca fiind coordonator pentru fiecare worker de-al sau
// cand ajunge la coordonatorul 0 ar trebui ca toti coordonatorii sa fi trimis
// lista de workeri de care au grija
void complete_topology(int rank, int *coordinators, int procs, int num_workers, int *workers) {
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

void spread_topology(int rank, int *coordinators, int coordinator, int procs, int num_workers, int *workers) {
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

void spread_work() {
    // todo
}

int main (int argc, char *argv[])
{
    int procs, rank;
    int N = -1, error_type, coordinator = -1;
    int *coordinators, *to_compute = NULL, *recv_compute = NULL;
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
        error_type = atoi(argv[2]);
        // printf("N = %d, error_type = %d\n", N, error_type);
    }
    
    spread_coordinators(rank, coordinators, &coordinator, &num_workers, &workers);
    complete_topology(rank, coordinators, procs, num_workers, workers);
    spread_topology(rank, coordinators, coordinator, procs, num_workers, workers);
    
    // trimit N pentru ca in afara de procesul 0, restul nu stiu cate numere trebuie sa calculeze
    // trimit apoi vectorul pe care se aplica inmultirile
    // trimit apoi cati workeri au primit deja sarcinile ca sa stiu indexul de start

    if (rank >= 0 && rank < N_CLUSTERS) {
        if (rank == 0) {
            N = atoi(argv[1]);
            to_compute = (int *)malloc(N * sizeof(int));
            for (int i = 0; i < N; i++) {
                to_compute[i] = N - i - 1;
            }
            printf("N = %d, procs = %d Numarul de calcule: %d\n", N, procs, N / (procs - 4));
            // trimite mai departe catre coordonatorul 1 vectorul de numere
            MPI_Send(&N, 1, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD);
            MPI_Send(to_compute, N, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD);
            MPI_Send(&num_workers, 1, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, rank + 1);

            // trimite calculele catre workeri
            for (int i = 0; i < num_workers; i++) {
                int idx = index + i;
                int start = idx * (double)N / (procs - N_CLUSTERS);
                int end = fmin((idx + 1) * (double)N / (procs - N_CLUSTERS), N);
                n = end - start;
                printf("worker = %d - start = %d, end = %d\n",workers[i], start, end);
                MPI_Send(&n, 1, MPI_INT, workers[i], TAG, MPI_COMM_WORLD);
                MPI_Send(to_compute + start, n, MPI_INT, workers[i], TAG, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, workers[i]);
            }
        } else {
            // coordonatorii primesc de la coordonatorul anterior N, vectorul de numere
            // si numarul de workeri care au primit deja sarcini (indexul de start)
            // si trimit mai departe
            MPI_Recv(&N, 1, MPI_INT, rank - 1, TAG, MPI_COMM_WORLD, &status);
            to_compute = (int *)malloc(N * sizeof(int));
            MPI_Recv(to_compute, N, MPI_INT, rank - 1, TAG, MPI_COMM_WORLD, &status);
            MPI_Recv(&index, 1, MPI_INT, rank - 1, TAG, MPI_COMM_WORLD, &status);

            // trimit mai departe catre coordonatorul urmator vectorul de numere
            if (rank != N_CLUSTERS - 1) {
                MPI_Send(&N, 1, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD);
                MPI_Send(to_compute, N, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD);
                int new_index = index + num_workers;
                MPI_Send(&new_index, 1, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, rank + 1);
            } 

            // trimit workerilor numarul de numere pe care trebuie sa aplice inmultirea
            // si bucata lor de vector
            for (int i = 0; i < num_workers; i++) {
                int idx = index + i;
                int start = idx * (double)N / (procs - N_CLUSTERS);
                int end = fmin((idx + 1) * (double)N / (procs - N_CLUSTERS), N);
                printf("worker = %d - start = %d, end = %d\n",workers[i], start, end);
                n = end - start;
                MPI_Send(&n, 1, MPI_INT, workers[i], TAG, MPI_COMM_WORLD);
                MPI_Send(to_compute + start, n, MPI_INT, workers[i], TAG, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, workers[i]);
            }
        }
    } else {
        // workerii primesc de la coordonatorul lor numarul de calcule
        // si fac calculele
        // apoi trimit inapoi coordonatorului rezultatul
        MPI_Recv(&n, 1, MPI_INT, coordinator, TAG, MPI_COMM_WORLD, &status);
        printf("sunt worker %d si primesc %d numere de la %d\n", rank, n, coordinator); 
        int *recv = (int *)malloc(n * sizeof(int));
        MPI_Recv(recv, n, MPI_INT, coordinator, TAG, MPI_COMM_WORLD, &status);

        for (int i = 0; i < n; i++) {
            recv[i] *= 5;
        }
        MPI_Send(&n, 1, MPI_INT, coordinator, TAG, MPI_COMM_WORLD);
        MPI_Send(recv, n, MPI_INT, coordinator, TAG, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, coordinator);
    }
        
    MPI_Barrier(MPI_COMM_WORLD);

    // fiecare coordonator primeste de la workeri rezultatele partiale
    if (rank >= 0 && rank < N_CLUSTERS) {
        int count = index * (double)N / (procs - N_CLUSTERS);
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
        printf("rank = %d, am primit de la workeri: ", rank);
        for (int i = 0; i < N; i++) {
            printf("%d ", to_compute[i]);
        }
        printf("\n"); 
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank >= 0 && rank < N_CLUSTERS) {
        if (rank == 0) {
            recv_compute = (int *)malloc(N * sizeof(int));
            printf("start_index = %d\n", start_index);
            printf("Vectorul initial este: ");
            for (int i = 0; i < N; i++) {
                printf("%d ", to_compute[i]);
            }
            printf("\n");
            for (int i = 1; i < N_CLUSTERS; i++) {
                MPI_Recv(recv_compute, N, MPI_INT, N_CLUSTERS - 1, TAG, MPI_COMM_WORLD, &status);
                while (to_compute[start_index] != recv_compute[start_index]) {
                    to_compute[start_index] = recv_compute[start_index];
                    start_index++;
                }
            }
            printf("Rezultat: ");
            for (int i = 0; i < N; i++) {
                printf("%d ", to_compute[i]);
            }
            printf("\n");
        } else {
            recv_compute = (int *)malloc(N * sizeof(int));  
            for (int i = 1; i < rank; i++) {
                MPI_Recv(recv_compute, N, MPI_INT, rank - 1, TAG, MPI_COMM_WORLD, &status);
                // printf("sunt %d si Am primit: ", rank);
                // for (int j = 0; j < N; j++) {
                //     printf("%d ", recv_compute[j]);
                // }
                printf("\n");
                if (rank == N_CLUSTERS - 1) {
                    // printf("sunt %d si trimit catre 0: ", rank);
                    // for (int j = 0; j < N; j++) {
                    //     printf("%d ", recv_compute[j]);
                    // }
                    // printf("\n");
                    MPI_Send(recv_compute, N, MPI_INT, 0, TAG, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, 0);
                } else {
                    // printf("sunt %d si trimit catre %d: ", rank, rank + 1);
                    // for (int j = 0; j < N; j++) {
                    //     printf("%d ", recv_compute[j]);
                    // }
                    // printf("\n");
                    MPI_Send(recv_compute, N, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, rank + 1);
                }
            }
            if (rank == N_CLUSTERS - 1) {
                MPI_Send(to_compute, N, MPI_INT, 0, TAG, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, 0);
            } else {
                MPI_Send(to_compute, N, MPI_INT, rank + 1, TAG, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, rank + 1);
            }

        }
    }


    MPI_Finalize();

    if (rank >= 0 && rank < N_CLUSTERS) {
        free(workers);
    }
    free(coordinators);
}




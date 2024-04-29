#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/mman.h>

typedef struct {
    int* leftList;
    int leftSize;
    int* rightList;
    int rightSize;
    int* resultList;
    int resultSize;
    int processCount;
} MergeSortData;
MergeSortData **mergeSortDataArray;

int *processCount;


int countElements(const char *string) {
    int elements = 1;
    while (*string) {
        if (*string == ',') elements++;
        string++;
    }
    return elements;
}

int *createArrayWithElements(const char *string, int elements) {
    int *array = mmap(NULL, elements * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (array == MAP_FAILED) {
        printf("Memory allocation error.\n");
        exit(1);
    }

    char *stringCopy = strdup(string);
    char *token = strtok(stringCopy, ",");
    int i = 0;
    while (token != NULL && i < elements) {
        array[i++] = atoi(token);
        token = strtok(NULL, ",");
    }
    free(stringCopy);

    return array;
}

void merge(int arr[], int l, int m, int r, int processId) {
    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;

    int L[n1], R[n2];

    for (i = 0; i < n1; i++)
        L[i] = arr[l + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[m + 1 + j];

    i = 0;
    j = 0;
    k = l;
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            arr[k] = L[i];
            i++;
        } else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }

    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }

    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }

    if (processId != -1){

        memcpy(mergeSortDataArray[processId]->leftList, L, n1 * sizeof(int));
        mergeSortDataArray[processId]->leftSize = n1;

        memcpy(mergeSortDataArray[processId]->rightList, R, n2 * sizeof(int));
        mergeSortDataArray[processId]->rightSize = n2;

        memcpy(mergeSortDataArray[processId]->resultList, arr + l, (r - l + 1) * sizeof(int));
        mergeSortDataArray[processId]->resultSize = r - l + 1;

        mergeSortDataArray[processId]->processCount = processId;

    }
}

void printSublist(int arr[], int l, int r, int processNumber) {
    printf("Proceso %d: ", processNumber);
    for (int i = l; i < r; i++) {
        printf("%d,", arr[i]);
    }
    if (r >= l) {
        printf("%d", arr[r]);  // Imprime el último número sin coma
    }
    printf("\n");
}

void mergeSort(int arr[], int l, int r, int numParallelProcesses, int processId) {
    if (l < r) {
        int m = l + (r - l) / 2;

        if (numParallelProcesses > 1) {
            pid_t pid_left, pid_right;

            pid_left = fork();
            if (pid_left < 0) {
                perror("fork");
                exit(1);
            } else if (pid_left == 0) {
                // Proceso hijo izquierdo
                (*processCount)++;
                printSublist(arr, l, m, *processCount);
                mergeSort(arr, l, m, numParallelProcesses / 2, *processCount);
                
                exit(0);
            } else {
                // Proceso padre
                waitpid(pid_left, NULL, 0);  // Espera al proceso hijo izquierdo

                pid_right = fork();
                if (pid_right < 0) {
                    perror("fork");
                    exit(1);
                } else if (pid_right == 0) {
                    // Proceso hijo derecho
                    (*processCount)++;
                    printSublist(arr, m + 1, r, *processCount);
                    mergeSort(arr, m + 1, r, numParallelProcesses / 2, *processCount);
                    
                    exit(0);
                }

                waitpid(pid_right, NULL, 0);  // Espera al proceso hijo derecho
            }

            // Proceso padre espera a ambos hijos
            waitpid(pid_left, NULL, 0);
            waitpid(pid_right, NULL, 0);
        } else {
            // Límite de procesos alcanzado, ordenar secuencialmente
            mergeSort(arr, l, m, 1, -1);
            mergeSort(arr, m + 1, r, 1, -1);
        }

        merge(arr, l, m, r, processId);
    }
}

void printMergeSortDataArray() {
    int i, j;
    for (i = *processCount; i >= 0; i--) {
        MergeSortData* data = mergeSortDataArray[i];
        if (data->leftSize > 0 && data->rightSize > 0) {
            printf("proceso %d: lista izquierda {", i);
            for (j = 0; j < data->leftSize; j++) {
                printf("%d", data->leftList[j]);
                if (j < data->leftSize - 1) {
                    printf(",");
                }
            }
            printf("}, lista derecha {");
            for (j = 0; j < data->rightSize; j++) {
                printf("%d", data->rightList[j]);
                if (j < data->rightSize - 1) {
                    printf(",");
                }
            }
            printf("} => {");
        } else {
            printf("proceso %d lista ordenada: {", i);
        }
        for (j = 0; j < data->resultSize; j++) {
            printf("%d", data->resultList[j]);
            if (j < data->resultSize - 1) {
                printf(",");
            }
        }
        printf("}\n");
    }
}

void createMemoryForLists(MergeSortData* data, int elements) {
    data->leftList = mmap(NULL, elements * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (data->leftList == MAP_FAILED) {
        perror("mmap failed for leftList");
        return;
    }

    data->rightList = mmap(NULL, elements * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (data->rightList == MAP_FAILED) {
        perror("mmap failed for rightList");
        munmap(data->leftList, elements * sizeof(int));
        return;
    }

    data->resultList = mmap(NULL, elements * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (data->resultList == MAP_FAILED) {
        perror("mmap failed for resultList");
        munmap(data->leftList, elements * sizeof(int));
        munmap(data->rightList, elements * sizeof(int));
        return;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <numero_de_procesos_a_lanzar> <lista_de_numeros_separados_por_commas>\n", argv[0]);
        return 1;
    }

    int numParallelProcesses = atoi(argv[1]);

    int elements = countElements(argv[2]);
    int *numbers = createArrayWithElements(argv[2], elements);

    mergeSortDataArray = mmap(NULL, numParallelProcesses * sizeof(MergeSortData*), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    for (int i = 0; i < numParallelProcesses; i++) {
        mergeSortDataArray[i] = mmap(NULL, sizeof(MergeSortData), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        createMemoryForLists(mergeSortDataArray[i], elements);
    }

    processCount = mmap(NULL, sizeof *processCount, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *processCount = 0;

    printf("===mapeos===\n");
    printSublist(numbers, 0, elements - 1, *processCount);
    mergeSort(numbers, 0, elements - 1, numParallelProcesses, 0);

    printf("\n===procesamiento===\n");
    printMergeSortDataArray();

    munmap(numbers, elements * sizeof(int));
    munmap(mergeSortDataArray, elements * sizeof(MergeSortData));

    return 0;
}

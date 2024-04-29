
#include <stdio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

typedef struct {
    int pid;
    int signal;
    int time;
} Data;

int main() {
    FILE *file = fopen("archivos.txt", "r");
    if (file == NULL) {
        perror("Error al abrir el archivo");
        return 1;
    }

    Data data;
    int count = 0;
    while (fscanf(file, "%d%d%d", &data.pid, &data.signal, &data.time) == 3) {
        count++;
    }

    rewind(file);

    Data *table = malloc(count * sizeof(Data));
    if (table == NULL) {
        perror("Error de asignación de memoria");
        fclose(file);
        return 1;
    }

    int i = 0;
    while (fscanf(file, "%d%d%d", &table[i].pid, &table[i].signal, &table[i].time) == 3) {
        i++;
    }

    fclose(file);

    pid_t child_pid;
    for (i = 0; i < count; i++) {
        if ((child_pid = fork()) == 0) {
            sleep(table[i].time);
            if (kill(table[i].pid, table[i].signal) == -1) {
                perror("Fallo al enviar la señal");
            } else {
                printf("Señal enviada a PID %d: %d\n", table[i].pid, table[i].signal);
            }
            exit(0);
        } else if (child_pid < 0) {
            perror("No se pudo crear el proceso hijo");
            free(table);
            return 1;
        }
    }

    int status;
    for (i = 0; i < count; i++) {
        pid_t child = wait(&status);
    }

    free(table);
    return 0;
}

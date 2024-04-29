
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

void handler(int signal) {
    switch(signal) {
        case SIGINT:
            printf("Señal SIGINT detectada\n");
            break;
        case SIGTERM:
            printf("Señal SIGTERM detectada\n");
            break;
        case SIGQUIT:
            printf("Señal SIGQUIT detectada\n");
            break;
        case SIGALRM:
            printf("Señal SIGALRM detectada\n");
            break;
        case SIGUSR1:
            printf("Señal SIGUSR1 detectada\n");
            break;
        case SIGUSR2:
            printf("Señal SIGUSR2 detectada\n");
            break;
        default:
            printf("Señal detectada\n");
            break;
    }
}

int main() {
    int i;
    for (i = 1; i < 64; i++) { // Comenzamos desde 1, ya que la señal 0 es SIGNULL
        if (i != SIGKILL && i != SIGSTOP) { // No se puede manipular SIGKILL ni SIGSTOP
            signal(i, handler);
        }
    }
    
    printf("Programa 2 ejecutándose. PID: %d\n", getpid());

    while(1) {
        sleep(5);
    }

    return 0;
}

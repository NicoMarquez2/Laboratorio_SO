// Código del archivo p_c.c

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>

#define MAX_SCRIPTS 10
#define MAX_LENGTH 256

sem_t sem_producer;
sem_t sem_consumer;

char scripts[MAX_SCRIPTS][MAX_LENGTH];
int current_script = 0;
int producer_finished = 0;

void *producer(void *arg) {
    FILE *file = fopen("scripts.txt", "r");
    if (!file) {
        perror("Error al abrir scripts.txt");
        return NULL;
    }
    
    char line[MAX_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        sem_wait(&sem_producer);
        line[strcspn(line, "\n")] = 0; // Eliminar salto de línea
        strncpy(scripts[current_script], line, MAX_LENGTH);
        printf("[Productor] Notificando al consumidor sobre el nuevo script: %s\n", scripts[current_script]);
        current_script++;
        sem_post(&sem_consumer);
    }
    
    fclose(file);
    sem_wait(&sem_producer);
    producer_finished = 1;
    printf("[Productor] Finalizado\n");
    sem_post(&sem_consumer);
    return NULL;
}

void *consumer(void *arg) {
    FILE *log_file = fopen("logs.txt", "w");
    if (!log_file) {
        perror("Error al abrir logs.txt");
        return NULL;
    }

    while (1) {
        sem_wait(&sem_consumer);
        if (producer_finished && current_script == 0) {
            fprintf(log_file, "[Consumidor] El productor ha finalizado. No hay más scripts para ejecutar.\n");
            fflush(log_file);
            break;
        }
        if (current_script > 0) {
            current_script--;
            fprintf(log_file, "[Consumidor] Ejecutando el script: %s\n", scripts[current_script]);
            fflush(log_file);
            printf("[Debug] Ejecutando el script: %s\n", scripts[current_script]);
            // Simulando ejecución del script
            sleep(1);
        }
        sem_post(&sem_producer);
    }

    fclose(log_file);
    return NULL;
}

int main() {
    pthread_t producer_thread, consumer_thread;

    sem_init(&sem_producer, 0, 1);
    sem_init(&sem_consumer, 0, 0);

    pthread_create(&producer_thread, NULL, producer, NULL);
    pthread_create(&consumer_thread, NULL, consumer, NULL);

    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);

    sem_destroy(&sem_producer);
    sem_destroy(&sem_consumer);

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>

int main() {
    int p, q;
    char line[256];
    char script_name[64];
    char execute[64];
    int script_count = 0;

    const int SIZE = 50;
    int shm_fd;
    char *ptr;

    // Crear semáforos
    sem_t *s_prod = sem_open("s_prod.txt", O_CREAT, 0666, 1);
    sem_t *s_shmem = sem_open("s_shmem.txt", O_CREAT, 0666, 1);
    sem_t *s_cons = sem_open("s_cons.txt", O_CREAT, 0666, 0);

    // Crear memoria compartida
    shm_fd = shm_open("sh_mem", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, SIZE);
    ptr = (char*) mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    // Abrir archivo de scripts
    FILE *f = fopen("/home/nico/Documentos/Laboratorio_SO/scripts.txt", "r");
    if (f == NULL) {
        perror("Error al abrir el archivo de entrada");
        return 1;
    }

    FILE *s = NULL;

    // Crear el proceso productor
    p = fork();

    if (p < 0) {  // Error al crear el proceso
        perror("Error al crear el proceso productor");
        return 1;
    } else if (p == 0) {  // Proceso productor
        printf("ENTRO PRODUCTOR\n");
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "//", 2) == 0) {
                // Cerrar el archivo de script actual antes de crear uno nuevo
                if (s != NULL) {
                    fclose(s);
                    chmod(script_name, S_IRWXU);
                    
                    // Colocar nombre en memoria compartida y notificar al consumidor
                    sem_wait(s_prod);
                    sem_wait(s_shmem);
                    strncpy(ptr, script_name, SIZE);
                    sem_post(s_shmem);
                    sem_post(s_cons);
                }

                script_count++;
                snprintf(script_name, sizeof(script_name), "/home/nico/Documentos/Laboratorio_SO/script%d.sh", script_count);
                
                // Abrir un nuevo archivo para escribir el script
                s = fopen(script_name, "w");
                if (s == NULL) {
                    perror("Error al crear el archivo de script");
                    fclose(f);
                    exit(1);
                }
            } else {
                // Escribir en el archivo de script actual
                if (s != NULL) {
                    fputs(line, s);
                }
            }
        }

        // Cerrar el último archivo de script al finalizar la lectura
        if (s != NULL) {
            fclose(s);
            chmod(script_name, S_IRWXU);

            // Notificar al consumidor sobre el último archivo
            sem_wait(s_prod);
            sem_wait(s_shmem);
            strncpy(ptr, script_name, SIZE);
            sem_post(s_shmem);
            sem_post(s_cons);
        }

        fclose(f);
        exit(0);  // Terminar el proceso productor
    }

    // Crear el proceso consumidor
    q = fork();

    if (q < 0) {  // Error al crear el proceso
        perror("Error al crear el proceso consumidor");
        return 1;
    } else if (q == 0) {  // Proceso consumidor
        printf("ENTRO CONSUMIDOR\n");
        while (1) {
            sem_wait(s_cons);
            
            sem_wait(s_shmem);
            strncpy(execute, ptr, SIZE);
            sem_post(s_shmem);
            
            printf("Ejecutando el script: %s\n", execute);
            system(execute);  // Ejecutar el script usando system()
            sleep(5);         // Pausa entre ejecuciones
            sem_post(s_prod);
        }
        exit(0);  // Terminar el proceso consumidor
    }

    // Liberar recursos en el proceso principal
    munmap(ptr, SIZE);
    close(shm_fd);
    shm_unlink("sh_mem");

    sem_close(s_prod);
    sem_close(s_shmem);
    sem_close(s_cons);
    sem_unlink("s_prod.txt");
    sem_unlink("s_shmem.txt");
    sem_unlink("s_cons.txt");

    printf("Archivos de script generados correctamente.\n");

    // Esperar a que los hijos terminen
    wait(NULL);  // Espera al productor
    wait(NULL);  // Espera al consumidor

    return 0;
}

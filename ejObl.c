#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>

int main() {
	int p;
    char line[256];
    char scripts[20][256];
	int script_count = 0;

	const int SIZE =  50;
	char shm_fd;
	char *ptr;

	//Declaro semáforos
	sem_t *s_prod = sem_open("s_prod.txt", O_CREAT, 644, 1);;
	sem_t *s_shmem = sem_open("s_shmem.txt", O_CREAT, 644, 1);
	sem_t *s_cons = sem_open("s_cons.txt", O_CREAT, 644, 0);

	//Creo memoria compartida
	shm_fd = shm_open("sh_mem", O_CREAT | O_RDWR, 0666);
	ftruncate(shm_fd, SIZE);
	ptr = (char*) mmap (0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0); 

	//Abro archivo de scripts
    FILE *f = fopen("/home/tecnoinf/Desktop/SO_BV/scripts.txt", "r");

    if (f == NULL) {
        perror("Error al abrir el archivo de entrada");
        return 1;
    }

    FILE *s = NULL;

    // Leer línea por línea desde el archivo de entrada
    while (fgets(line, sizeof(line), f)) {
        // Verificar si encontramos el delimitador de separación
        if (strncmp(line, "//", 2) == 0) {
            // Si ya había un archivo abierto, cerrarlo
            if (s != NULL) {
                fclose(s);
            }

            // Incrementar el contador de scripts
            script_count++;

            // Crear un nombre de archivo nuevo, por ejemplo: script1.sh, script2.sh, ...
            char script_name[64];
            snprintf(script_name, sizeof(script_name), "/home/tecnoinf/Desktop/SO_BV/script%d.sh", script_count);

            // Abrir un nuevo archivo para escribir
            s = fopen(script_name, "w");
            if (s == NULL) {
                perror("Error al crear el archivo de script");
                fclose(f);
                return 1;
            }
        } else {
            // Si no encontramos el delimitador, escribir la línea en el archivo actual
            if (s != NULL) {
                fputs(line, s);
            }
        }
    }

    // Cerrar el archivo de entrada y el último archivo de script
    if (s != NULL) {
        fclose(s);
    }
    fclose(f);

    printf("Archivos de script generados correctamente.\n");

    return 0;
}

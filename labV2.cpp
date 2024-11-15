#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <list>
#include <string>
#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>    
#include <sys/stat.h>    
#include <semaphore.h> 

using namespace std;

int main() {
    int p, q;
    char line[256];
    char script_name[64];
    char execute[64];
    int script_count = 0;

    // Definir semáforos para la sincronización entre productor y consumidor
    sem_t *s_buffer = sem_open("/s_buffer", O_CREAT, 0666, 1);  // Semáforo para proteger el acceso al buffer
    sem_t *s_cons = sem_open("/s_cons", O_CREAT, 0666, 0);       // Semáforo para el consumidor (espera a que haya algo en el buffer)

    if (s_buffer == SEM_FAILED || s_cons == SEM_FAILED) {
        perror("Error al crear los semáforos");
        return 1;
    }

    // Crear el archivo de entrada
    FILE *f = fopen("/home/tecnoinf/Desktop/SO_BV/Laboratorio_SO/scripts.txt", "r");
    if (f == NULL) {
        perror("Error al abrir el archivo de entrada");
        return 1;
    }

    // Crear buffer (lista de strings) para compartir entre los procesos
    list<string> buffer;
    
    // Crear proceso productor
    p = fork();
    if (p < 0) {  // Error al crear el proceso productor
        perror("Error al crear el proceso productor");
        return 1;
    } else if (p == 0) {  // Proceso productor
        printf("ENTRO PRODUCTOR\n");
        FILE *s = NULL;
        
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "//", 2) == 0) {
                // Si ya había un archivo abierto, cerrarlo
                if (s != NULL) {
                    fclose(s);
                    chmod(script_name, S_IRWXU);  // Asegurar permisos del archivo creado
                    printf("PONGO EN BUFFER: %s\n", script_name);
                    
                    // Colocar nombre en el buffer y notificar al consumidor
                    sem_wait(s_buffer);  // Esperar a que el consumidor consuma el anterior
                    buffer.push_back(script_name);  // Poner el nombre del archivo en el buffer
                    sem_post(s_buffer);  // Liberar el semáforo para el buffer
                    printf("PRODUCTOR NOTIFICA CONSUMIDOR\n");
                    sem_post(s_cons);    // Notificar al consumidor que hay algo para consumir
                    
                }

                script_count++;
                snprintf(script_name, sizeof(script_name), "/home/tecnoinf/Desktop/SO_BV/Laboratorio_SO/script%d.sh", script_count);
                printf("ARCHIVO: %s\n", script_name);
                
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

        // Cerrar el último archivo de script
        if (s != NULL) {
            fclose(s);
            chmod(script_name, S_IRWXU);  // Asegurar permisos del archivo creado

            // Colocar nombre en el buffer y notificar al consumidor
            sem_wait(s_buffer);
            buffer.push_back(script_name);  // Poner el nombre del archivo en el buffer
            sem_post(s_buffer);

            sem_post(s_cons); // Notificar al consumidor
            printf("PRODUCTOR FINALIZA\n");
        }

        fclose(f);
        exit(0);  // Terminar el proceso productor
    }

    // Crear proceso consumidor
    q = fork();
    if (q < 0) {  // Error al crear el proceso consumidor
        perror("Error al crear el proceso consumidor");
        return 1;
    } else if (q == 0) {  // Proceso consumidor
        printf("ENTRO CONSUMIDOR\n");
        while (1) {
            sem_wait(s_cons);  // Esperar a que haya algo en el buffer
            printf("CONSUMIDOR DESPIERTA\n");

            // Tomar el nombre del script del buffer
            sem_wait(s_buffer);  // Acceder al buffer de manera segura
            if (buffer.empty()) {
                printf("BUFFER VACÍO: El consumidor no tiene elementos para consumir.\n");
                sem_post(s_buffer);
                continue; // Si el buffer está vacío, esperar
            }
            string script_name = buffer.front();  // Obtener el primer script
            buffer.pop_front();  // Eliminar el script del buffer
            sem_post(s_buffer);  // Liberar el buffer

            // Verificar que el script está disponible antes de ejecutarlo
            printf("CONSUMIDOR: EJECUTANDO EL SCRIPT: %s\n", script_name.c_str());

            // Ejecutar el script
            int status = system(script_name.c_str());  // Ejecutar el script usando system()
            
            if (status == -1) {
                printf("Error al ejecutar el script: %s\n", script_name.c_str());
            } else {
                printf("Script ejecutado correctamente: %s\n", script_name.c_str());
            }

            remove(script_name.c_str());  // Eliminar el script después de ejecutarlo
            sleep(5);  // Pausa entre ejecuciones
        }
        exit(0);  // Terminar el proceso consumidor
    }

    // Esperar a que los hijos terminen
    wait(NULL);  // Espera al productor
    wait(NULL);  // Espera al consumidor

    // Liberar recursos
    sem_close(s_buffer);
    sem_close(s_cons);
    sem_unlink("s_buffer");
    sem_unlink("s_cons");

    printf("Archivos de script generados y ejecutados correctamente.\n");

    return 0;
}

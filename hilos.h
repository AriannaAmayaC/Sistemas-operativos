/* hilos.h
 * 
 * Declaraciones de las funciones, definición de constantes, variables y 
 * semáforos necesarios para el funcionamiento del modo que utiliza
 * hilos para obtener el nivel de concurrencia.
 * 
 * Autores:
 *   Arianna Amaya, 12-11115.
 * 
 * Fecha: 23 de junio de 2016.
 */

#ifndef __hilos__
#define __hilos__

#include <pthread.h>
#include <semaphore.h>
#include "shell.h"

/***MUTEX****/
#define READ 0
#define WRITE 1

/************Definición de estructuras************/

struct tareaH_struct{
	char instruccion;
	char archivo[MAX_NOMBRE];
	char string [MAX_STRING];
};

typedef struct tareaH_struct tarea_h;

int comunicacion_h();
/*Crea las listas de tareas pendientes, listas y con error que usará el maestro
 y los esclavos para comunicarse. Además, inicializa los semaforos necesarios
 para las regiones críticas*/

int crear_trabajadores_h(int n_concurrencia, pthread_t thread[]);
/*Crea la cantidad de hilos que indica n_concurrencia*/

int tipo_comando_h(char ** arg, int nro_arg, char*raiz_sistema);
/*Determina que comando fue introducido por el usuario y llama a la función 
 ejecutar_comando_h() con el tipo de instruccion requerida*/
 
int fin_trabajadores(int n_concurrencia);
/*Envía la instrucción DQUIT a todos los hilos para que terminen su ejecución*/

void limpiar_mem_h();
/*Destruye los semáforos y libera el espacio de memoria de las listas
 utilizadas*/

/***Funciones del maestro***/
int ejecutar_comando_h(char** arg, int nro_arg, char* raiz_sistema,char instruccion);
/*Funcion principal del maestro. Se encarga de llamar a la función recursiva 
correspondiente con "instruccion". Espera que todas las tareas pendientes 
sean realizadas, las ordena e imprime*/

void esperar(); 
/*Bloquea al maestro hasta que todas las tareas pendientes sean realizadas*/

int dwc_r_h(char *ruta, char *extension);
/*Función recursiva que utiliza el maestro para moverse desde la raíz del 
 sistema y agregar a la lista de tareas pendientes todos los archivos
 regulares que tengan "extension"*/

int dgrep_r_h(char *ruta, char *string);
/*Función recursiva que utiliza el maestro para moverse desde la raíz del 
 sistema y agregar a la lista de tareas pendientes todos los archivos regulares
 indicando un string que debe ser buscado dentro de dichos archivos*/
 
int enviar_datos_h(char * archivo, char inst, char * string);
/*Función que utiliza el maestro para agregar una nueva tarea a la lista
de tareas pendientes*/

/***Funciones de los esclavos***/
void * rutina_hilos(void *arg);
/*Función principal de los esclavos. Cuando el padre ha agregado una nueva 
 tarea a tareas pendientes y es su turno, pueden leerla y de acuerdo al 
 tipo de instrucción ejecutar la funcion correspondiente*/

int ejecutar_wc_h(char * archivo);
/*Llama a la funcion wc() para calcular el número de líneas y de palabras 
de "archivo" y agrega el resultado a la lista de tareas listas o de errores,
según sea el caso*/

int ejecutar_grep_h(char * archivo, char * string);
/*Llama a la funcion grep() para determinar si "archivo" contiene en al 
 menos una de sus líneas "string" y agrega el resultado a la lista de 
 tareas listas o de errores, según sea el caso*/

void finalizar(int error);
/*Es utilizada para finalizar el programa cuando ocurre un error irrecuperable
 en la ejecución de los esclavos*/

#endif

/* procesos.h
 * 
 * Declaraciones de las funciones, definición de constantes, variables y 
 * semáforos necesarios para el funcionamiento de los modos que utilizan
 * procesos para obtener el nivel de concurrencia.
 * 
 * Autores:
 *   Arianna Amaya, 12-11115.
 * 
 * Fecha: 23 de junio de 2016.
 */

#ifndef __procesos__
#define __procesos__


#include <sys/mman.h>
#include <pthread.h>

#include "shell.h"

/***MUTEX****/
#define READ 0
#define WRITE 1


/*****Funciones*****/

int memoria_compartida(pthread_mutex_t ** mutex);
/*Crea un espacio de memoria que compartan el maestro y los esclavos
 donde se declaran e inicializan los mutex necesarios para la sincronización
 de la comunicación*/

int crear_trabajadores(int n_concurrencia,int modo,int * maestro_pipe,
pthread_mutex_t* mutex,int memoria_compartida, char*raiz_sistema);
/*Crea la cantidad de procesos indicada por n_concurrencia y los pipes 
 que se utilizarán para la comunicación*/
 
void cerrar(int entrada, int salida, int memoria_compartida);
/*Cierra los extremos del pipe y la memoria compartida*/

int tipo_comando(char ** arg, int nro_arg, char*raiz_sistema, int mestro_pipe[],
int n_concurrencia);
/*Determina que comando fue introducido por el usuario y llama a la función 
 ejecutar_comando con el tipo de instruccion requerida*/

/***Funciones del maestro***/

int ejecutar_comando (char** arg, int nro_arg, char* raiz_sistema, int maestro_pipe[],
int n_concurrencia, char instruccion);
/*Funcion principal del maestro. Se encarga de crear las listas de datos
 y errores, llama a la función recursiva correspondiente con "instruccion".
 Ordena las listas y las imprime*/

int dwc_r(char*ruta, char*ext, int maestro_pipe[], int n_concurrencia,
int *enviados, int *recibidos, lista *datos, lista* errores);
/*Función recursiva que utiliza el maestro para moverse desde la raíz del 
 sistema y enviar a los esclavos todos los archivos regulares que tengan 
 extension "ext"*/

int dgrep_r(char*ruta, char*palabra, int maestro_pipe[], int n_concurrencia,
int * enviados, int *recibidos, lista *datos, lista *errores);
/*Función recursiva que utiliza el maestro para moverse desde la raíz del 
 sistema y enviar a los esclavos todos los archivos regulares indicando 
 un string que debe ser buscado dentro de dichos archivos*/

/*Manejo de datos*/

int enviar_datos(int maestro_pipe[], char * archivo, char instruccion, char * string);
/*Función que utiliza el maestro para escribir en el pipe una nueva tarea*/

int recibir_datos(int maestro_pipe[], int leer, int * recibidos, int enviados,
					lista *datos, lista* errores, char inst);
/*Función que utiliza el maestro para leer del pipe el resultado de una 
 tarea*/

/***Funciones de los esclavos***/

int rutina_esclavos (int modo, int entrada, int salida, pthread_mutex_t* mutex, 
int memoria_compartida);
/*Función principal de los esclavos. Cuando el padre ha enviado una nueva 
 tarea por el pipe y es su turno, pueden leerla y de acuerdo al tipo de 
 instrucción ejecutar la funcion correspondiente*/

int ejecutar_wc(char * archivo, int salida, pthread_mutex_t* mutex, int modo);
/*Llama a la funcion wc() o wc_sist() según indique modo, para calcular 
 el número de líneas y de palabras de "archivo" y enviar por el pipe el 
 resultado*/

int ejecutar_grep(char * archivo, char* string, int salida, pthread_mutex_t* mutex,
int modo);
/*Llama a la funcion grep() o grep_sist() según indique modo, para determinar
 si "archivo" contiene en al menos una de sus líneas "string" y enviar por
 el pipe el resultado*/


#endif

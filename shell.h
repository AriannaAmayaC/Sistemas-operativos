/* shell.h
 * 
 * Declaraciones de las funciones que usa el shell, definición de constantes y
 * de códigos de error.
 * 
 * Autores:
 *   Arianna Amaya, 12-11115.
 * 
 * Fecha: 23 de junio de 2016.
 */

#ifndef __shell__
#define __shell__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

//Otras
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdbool.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <stdint.h>
#include "listas.h"


/***TAMANOS****/
#define MAX_COMANDO 50
#define MAX_RUTA 50
#define MAX_ARCHIVO 1024
#define MAX_STRING 40

/***TIPOS DE RUTA****/
#define PADRE 0
#define ABSOLUTA 1
#define RELATIVA 2


/****TIPOS DE ARCHIVO****/
#define REGULAR 0
#define DIRECTORIO 1
#define	LINKR 2  //Link a un archivo regular
#define LINKD 3 //Link a un directorio

/****TIPOS DE INSTRUCCIONES****/
#define DWC 0
#define DGREP 1
#define DQUIT 2


/*****Errores*****/
#define NOVALIDO -1
#define E_PARAMETROS -2
#define E_MEMORY -3
#define E_OPEN -4
#define E_READ -5
#define E_WRITE -6
#define E_CLOSE -7
#define E_STAT -8
#define E_OPENDIR -9
#define E_EXEC -10
#define E_FORK - 11
#define E_LOCK - 12
#define E_UNLOCK - 13
#define E_PCREATE -14
#define E_COM -15
#define E_SEM -16
#define E_MUTEX -17
#define E_MSHARED -18



/****Tipos de datos utilizados en la función grep****/
typedef struct {
	int fd; //File descriptor del archivo que se lee
	char datos[MAX_ARCHIVO]; //buffer
	int proximo; //indica cual será la proxima posición de datos[] a usar
	int limite; //indica cual es el tamaño usado de datos[]
} lector;

/*****Funciones*****/

void remover_espacios(int tam, char * buffer);
/*Dado el tamaño del string, y un string terminado en "\n\0", r
remueve espacios innecesarios del string*/

int datos_entrada(char * buffer, char *** arg, int* nro_arg);
/*Separa y organiza los datos pasados a tráves del buffer en un arreglo de 
strings*/

int leer_entrada(char *** arg, int* nro_arg);
/*Obtiene los datos desde la entrada estandar y los almacena en arg*/

void borrar_entrada(char ** arg, int nro_arg);
/*Elimina la memoria dinámica asignada a los datos de entrada*/

int tipo_ruta(char *ruta);
/*Indica si la ruta es el padre, absoluta o relativa*/

int tipo_archivo(char * ruta);

char * directorio_actual(void);
/*Utiliza la llamada al sistema getcwd para obtener la posición actual. Se hacen
las verificaciones de memoria y errno*/

char * ruta_real(char* destino, char * raiz_sistema);
/*Devuelve la ruta absoluta de destino con respecto al origen real del sistema*/

bool tiene_extension( char * archivo, char * ext);
/*Devuelve true si el archivo dado tiene extension "ext", en caso contrio
 devuelve false*/

char * siguiente_ruta(char* destino, char * posicion_actual);
/*Función que retorna la ruta absoluta de destino*/

int wc (char *archivo, int * num_lineas, int * num_palabas);
/* Guarda el numero de lineas en num_lineas y el de palabras en num_palabras
 del archivo dado*/

int wc_sist(char* archivo,int* num_lineas,int* num_palabras);
/* Guarda el numero de lineas en num_lineas y el de palabras en num_palabras
 del archivo dado*/

int grep(char * archivo, char * string, bool* contiene);
/*Guarda true en contiene si el string dado aparece en al menos una línea
 del archivo, en caso contrario guarda false*/

void nuevo_lector(lector *l, int fd);
/*Función para inicializar los datos del tipo lector*/

int leer_linea(lector * l, char * buffer);
/*Función para leer fragmentos de un archivo dado y guarda en buffer una linea
de caracteres*/

int grep_sist(char*archivo, char*string, bool *contiene);
/*Guarda true en contiene si el string dado aparece en al menos una línea
 del archivo, en caso contrario guarda false*/

void print_error(int error);
/*Imprime el mensaje de error adecuado*/


#endif

/* listas.h
 * 
 * Declaraciones de funciones para listas. Permite crear, borrar, determinar
 * si la lista está vacía y su longitud, ordenarla alfanumericamente y 
 * mostrar por la salida estándar.
 * 
 * Autores:
 *   Arianna Amaya, 12-11115.
 * 
 * Fecha: 23 de junio de 2016.
 */

#ifndef __listas__
#define __listas__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/*******TAMAÑOS**********/
#define MAX_NOMBRE 1024

/*****TIPO DE LISTA******/
/*Listas con nodos cuyo apuntador datos es tipo resultado*/
#define LISTA_T1 0 //Con todos los campos válidos
#define LISTA_T2 1 //Con solo válido el campo archivo 


/************Definición de estructuras************/

struct nodo_struct{
	void * datos;
	struct nodo_struct * siguiente;	
};

typedef struct nodo_struct nodo;


struct resultado_struct{
	char archivo[MAX_NOMBRE];
	int num_lineas;
	int num_palabras;
};
typedef struct resultado_struct resultado;


/*Cabecera de la lista*/
typedef struct{
	nodo *inicio;
	nodo *fin;
} lista;


/************Definición de funciones************/

lista* crear_lista(void); 
/*Función que devuelve cabecera de una nueva lista*/

void borrar_lista(lista *l);
/*Libera el espacio de memoria utilizado por l*/

bool vacia_lista(lista l);
/*Devuelve true si la lista está vacía, en caso contrario devuelve false*/

int longitud_lista(lista l);
/*Devuelve la cantidad de nodos que tiene la lista l*/

void agregar_nodo(nodo * nuevo, lista * l);
/*Agrega un nuevo nodo a la lista*/

void * desencolar_lista(lista *l);
/*Devuelve la dirección del primer elemento de la lista*/

bool cambiar_nodos(nodo* auxiliar);
/*Retorna true si es necesario invertir el nodo actual y el siguiente para 
ordenar la lista, en caso contrario retorna false*/

void ordenar_lista(lista* l);
/*Ordena la lista l alfanúmericamente*/

void imprimir_lista(lista *datos, int tipo_lista, int desde);
/*Muestra por la salida estándar los elementos de la lista de acuerdo al
 tipo de lista*/
	
#endif

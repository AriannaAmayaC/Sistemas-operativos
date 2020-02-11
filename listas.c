/* listas.c
 * 
 * Implementación de subrutinas para el manejo de listas enlazadas.
 * 
 * Autores:
 *   Arianna Amaya, 12-11115.
 * 
 * Fecha: 23 de junio de 2016.
 */

#include "listas.h"


lista* crear_lista(void){
	lista *l;
	l=(lista*)malloc(sizeof(lista));
	if(l==NULL){
		return NULL;
	}
	l->inicio= NULL;
	l->fin = NULL;
	return l;
}

void borrar_lista(lista *l){
	nodo* auxiliar =l->inicio;
	while(auxiliar !=NULL){
		l->inicio = auxiliar->siguiente;
		free(auxiliar);
		auxiliar = l->inicio;
	}
	free(l);
}


bool vacia_lista(lista l){
	bool vacia= false;
	if(l.inicio == NULL){
		vacia = true;
	}
	return vacia;
}

/*Función longitud_lista: Se mueve secuencialmente por los nodos para
 contarlos*/
int longitud_lista(lista l){
	int contador=0;
	nodo* auxiliar =l.inicio;
	while(auxiliar!=NULL){
		auxiliar= auxiliar->siguiente;
		contador++;
	}
	return contador;
}

/*Función agregar_nodo: Se coloca el nuevo nodo al final de la lista*/
void agregar_nodo(nodo * nuevo, lista * l){
	if(vacia_lista(*l)){
		l->inicio = nuevo; 
		l->fin= l->inicio;
	}else{
		l->fin->siguiente = nuevo;
		l->fin = nuevo;
	}
	nuevo->siguiente = NULL;
}

/*Función desencolar_lista: Se elimina el primer nodo de la lista y se 
retorna los "datos" de dicho nodo */
void * desencolar_lista(lista *l){
	if(!vacia_lista(*l)){
		nodo * auxiliar = l->inicio;
		void* auxiliar_datos = auxiliar->datos;
		l->inicio = l->inicio->siguiente;
		free(auxiliar);
		return auxiliar_datos;
	}
	else
		return NULL;
}

/*Función cambiar_nodos: Se realiza un casting para poder acceder a los
 nombres de archivo almacenados en el nodo actual y el siguiente. Revisa 
 con la función strcmp si el nombre está primero, en orden alfanúmericamente
 que el nombre del siguiente*/
bool cambiar_nodos(nodo* auxiliar){
	resultado * r = auxiliar->datos;
	resultado * r_siguiente = auxiliar->siguiente->datos;
	if(strcasecmp(r->archivo, r_siguiente->archivo)>0)
			return true;
	return false;
}

/*Función ordenar_lista: Se mueve sobre toda la lista comparando el nodo
 actual y el siguiente para saber si es necesario cambiarlo. En caso 
 afirmativo intercambia los apuntadores "datos" de cada nodo. Se emplea
 una adaptación de bubble sort*/
void ordenar_lista(lista* l){
	nodo* auxiliar=l->inicio;
	void * temp;
	int j;
	int tamano= longitud_lista(*l);
	while(tamano!=0){
		for(j=0; j<tamano-1; j++){
			if(cambiar_nodos(auxiliar)){
				temp = auxiliar->datos;
				auxiliar->datos = auxiliar->siguiente->datos;
				auxiliar->siguiente->datos = temp;
			}
			auxiliar=auxiliar->siguiente;
		}
	auxiliar=l->inicio;	
	tamano--;
	}
}

/*Función imprimir_lista: Desencola cada elemento de la lista y muestras
 los valores contenidos en el campo "datos" de cada nodo. Se imprimen los
 campos correspondientes al tipo de lista. Al finalizar la lista queda 
 vacía*/
void imprimir_lista(lista * l, int tipo_lista, int desde){
	int i, tam;
	resultado * r;
	if(!vacia_lista(*l)){
		tam = longitud_lista(*l);
		for(i=0; i<tam; i++){
			r = (resultado *)desencolar_lista(l);
			if(tipo_lista == LISTA_T1)
				printf("%s\n %d %d\n", 
				&(r->archivo[desde]), r->num_lineas, r->num_palabras); 				
			else if(tipo_lista == LISTA_T2)
				puts(&(r->archivo[desde]));
			free(r);
		}
	}
	
}

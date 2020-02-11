/* hilos.c
 * 
 *	Implementación de subrutinas para el funcionamiento del modo que 
 *utiliza hilos para obtener el nivel de concurrencia.
 * 
 * Autores:
 *   Arianna Amaya, 12-11115.
 * 
 * Fecha: 23 de junio de 2016.
 */

#include "hilos.h"

/***Variables globales***/

/*Listas para la comunicación*/
lista * t_pendientes;
lista* t_listas;
lista* t_errores;

/*Semáforos para el control de las listas*/
pthread_mutex_t mutex_t_pendientes; 
pthread_mutex_t mutex_t_listas;
pthread_mutex_t mutex_t_errores;

sem_t sem; //Para indicar cuando hay datos disponibles en la lista de pendientes
sem_t listo; //Para indicar  cuando los trabajdores finalizaron las tareas

int enviados_p=0; //Número de tareas enviadas por el padre

/*comunicacion_h: Asignación de espacio de memoria a las listas e inicialización de los
 mutex y semáforos con sus funciones respectivas*/
int comunicacion_h(){
	t_pendientes = crear_lista();
	if(t_pendientes == NULL)
		return E_MEMORY;
	t_listas = crear_lista();
	if(t_listas == NULL)
		return E_MEMORY;
	t_errores = crear_lista();
	if(t_errores == NULL)
		return E_MEMORY;
	
	/*Semaforos para la RC*/
	int err= sem_init(&sem, 0, 0);
	if(err == -1)
		return E_SEM;
	err= sem_init(&listo, 0, 0);
	if(err == -1)
		return E_SEM;
	pthread_mutex_init(&mutex_t_pendientes,NULL); 
	pthread_mutex_init(&mutex_t_listas,NULL);
	pthread_mutex_init(&mutex_t_errores,NULL);
	return 0;
}

/*crear_trabajadores_h: Creación de hilos trabajadores con los atributos 
por defecto y "rutina_hilos" como start routine*/
int crear_trabajadores_h(int n_concurrencia, pthread_t thread[]){
	int i;
	int verificacion;
	for(i=0; i<n_concurrencia; i++){
		verificacion=pthread_create(&thread[i],NULL,rutina_hilos,NULL);
		if(verificacion!=0)
			return E_PCREATE;

	}
	return 0;
}

int tipo_comando_h(char ** arg, int nro_arg, char*raiz_sistema){
	int v;
	if (nro_arg == 0)
		return 0;
	else if(strcmp(arg[0], "dwc")==0) 
		v=ejecutar_comando_h(arg, nro_arg, raiz_sistema, DWC);
	else if(strcmp(arg[0], "dgrep")==0)
		v=ejecutar_comando_h(arg, nro_arg, raiz_sistema, DGREP);
	else if(strcmp(arg[0], "dquit")==0) //Salir del shell
		return DQUIT;
	else
		return NOVALIDO;
	return v;
}

/*fin_trabajadores: Envía la instrucción DQUIT a todos los esclavos*/
int fin_trabajadores(int n_concurrencia){
	int err, i;
	for(i=0; i<n_concurrencia; i++){
		err = enviar_datos_h(NULL, DQUIT, NULL);
		if(err<0)
			return E_MEMORY;
	}
	return 0;
}

/*limpiar_mem_h: Liberación de espacio de memoria de las listas con la 
funcion de listas.h. Destrucción de los mutex y semáforos con sus funciones
respectivas*/
void limpiar_mem_h(){
	pthread_mutex_destroy(&mutex_t_pendientes);
	pthread_mutex_destroy(&mutex_t_listas);
	pthread_mutex_destroy(&mutex_t_errores);
	sem_destroy(&sem);
	sem_destroy(&listo); 
	borrar_lista(t_pendientes);
	borrar_lista(t_listas);
	borrar_lista(t_errores);
}

/***Funciones del maestro***/

/*Funcion principal del maestro: Llama a las funciones recursivas 
 correspondientes. Luego de enviar todos los datos, el maestro espera hasta
 que los esclavos finalicen todas las tareas, esto se implementa con un
 semáforo "listo". Se utilizan las funciones de listas.h para ordenar e 
 imprimir las listas de datos y de errores*/
int ejecutar_comando_h(char** arg, int nro_arg, char* raiz_sistema,char instruccion){
	if(nro_arg!=2)
		return E_PARAMETROS;
	int verificacion;
	if(instruccion == DWC)
		verificacion = dwc_r_h(raiz_sistema, arg[1]);
	else
		verificacion = dgrep_r_h(raiz_sistema, arg[1]);
	if(verificacion<0)
		return verificacion;
	esperar();
	ordenar_lista(t_listas);
	ordenar_lista(t_errores);
	int tam_raiz = strlen(raiz_sistema);
	if(instruccion == DWC)
		imprimir_lista(t_listas, LISTA_T1, tam_raiz);
	else
		imprimir_lista(t_listas, LISTA_T2, tam_raiz);
	if(!vacia_lista(*t_errores)){
		printf("En los siguientes archivos ocurrió un error:\n");
		imprimir_lista(t_errores, LISTA_T2, tam_raiz);
	}
	return 0;
}

void esperar(){
	int i;
	for(i=0; i<enviados_p; i++)
		sem_wait(&listo);
	enviados_p =0; //Incializar valores enviados
}

/*Función dcw_r_h: Se desplaza por los directorios desde la raíz del sistema.
 Emplea las funciones tipo_archivo() y tiene_extension() para determinar que
 archivos debe enviar.*/
int dwc_r_h(char *ruta, char *extension){
	char * ruta_auxiliar;
	int tipo = tipo_archivo(ruta);
	if (tipo < 0)
		return tipo;
	if(tipo != DIRECTORIO) //Caso base: termina recursividad sino es un Directorio
		return 0;
	DIR * directorio = opendir(ruta);
	if(directorio==NULL)
		return E_OPENDIR;
	struct dirent *entrada = readdir(directorio);
	while (entrada!=NULL){
		//Se excluye caso "." y ".." para no caer en un loop infinito
		if((strcmp(entrada->d_name,"..")!=0)&&(strcmp(entrada->d_name,".")!=0)){ 
			ruta_auxiliar = siguiente_ruta(entrada->d_name, ruta); //Devuelve la ruta absoluta
			if(ruta_auxiliar==NULL)
				return E_MEMORY;
			if(tipo_archivo(ruta_auxiliar) == REGULAR){ //Archivo regular
				if(tiene_extension(entrada->d_name, extension)){
    	   			if(enviar_datos_h(ruta_auxiliar, DWC, NULL)!=0)
    	   				return E_MEMORY;
    	   			enviados_p++;
    	   		}
    	   	}	
   	  		dwc_r_h(ruta_auxiliar, extension);
   	  		free(ruta_auxiliar);
   	  	}
   	   entrada =readdir(directorio);
    }
    if(closedir(directorio)==-1)
    	return E_CLOSE;
    return 0;
}

/*Función dgrep_r_h: Se desplaza por los directorios desde la raíz del sistema.
 Emplea las funciones tipo_archivo() para determinar que archivos debe enviar.*/
int dgrep_r_h(char *ruta, char *string){
	char * ruta_auxiliar;
	int tipo = tipo_archivo(ruta);
	if (tipo < 0)
		return tipo;
	if(tipo != DIRECTORIO) //Caso base: termina recursividad sino es un Directorio
		return 0;
	DIR * directorio = opendir(ruta);
	if(directorio==NULL)
		return E_OPENDIR;
	struct dirent *entrada = readdir(directorio);
	while (entrada!=NULL){
		//Se excluye caso "." y ".." para no caer en un loop infinito
		if((strcmp(entrada->d_name,"..")!=0)&&(strcmp(entrada->d_name,".")!=0)){ 
			ruta_auxiliar = siguiente_ruta(entrada->d_name, ruta); //Devuelve la ruta absoluta
			if(ruta_auxiliar==NULL)
				return E_MEMORY;
			if(tipo_archivo(ruta_auxiliar) == REGULAR){ //Archivo regular
    	   			if(enviar_datos_h(ruta_auxiliar, DGREP, string)!=0)
    	   				return E_MEMORY;
    	   			enviados_p++;
    	  
    	   	}
   	  		dgrep_r_h(ruta_auxiliar, string);
   	  		free(ruta_auxiliar);
   	  	}
   	   entrada =readdir(directorio);
    }
    if(closedir(directorio)==-1)
    	return E_CLOSE;
	return 0;	
}

/*Función enviar_datos_h: Reserva espacio de memoria del tamaño de
 "tarea". Copia los datos dados y agrega a la lista de tareas 
 pendientes.*/
int enviar_datos_h(char * archivo, char inst, char * string){
	//Preparo el nuevo nodo para la lista
	nodo * nuevo = malloc(sizeof(nodo));
	if(nuevo == NULL)
		return E_MEMORY;
	nuevo->datos = (tarea_h*)malloc(sizeof(tarea_h));
	if(nuevo->datos == NULL)
		return E_MEMORY;
	tarea_h * r = (tarea_h *)nuevo->datos;
	r->instruccion = inst;
	if(inst != DQUIT){
		if(inst == DGREP)
			strcpy(r->string, string);
		strcpy(r->archivo, archivo);
	}
	int err_read= pthread_mutex_lock(&mutex_t_pendientes);
	if (err_read)
		return E_LOCK;
	agregar_nodo(nuevo, t_pendientes);
	err_read= pthread_mutex_unlock(&mutex_t_pendientes);
	if (err_read)
		return E_UNLOCK;
	sem_post(&sem);
	return 0;
}

/***Funciones de los esclavos**/
/*Función rutina_hilos: Es la función que recibe pthread_create como 
 párametro. Utiliza un semáforo "sem" que le indica a los esclavos cuando
 hay tareas pendientes y pueden leer*/
void * rutina_hilos(void *arg){
	int err_read;
	int * err= malloc(sizeof(int)); //Para enviar el status final
	*err = 0;
	tarea_h * tarea = NULL;
	while(1){
		if(sem_wait(&sem) == -1){
			*err = E_SEM;
			break;
		}
		err_read= pthread_mutex_lock(&mutex_t_pendientes);
		if (err_read){
			*err = E_LOCK;
			break;
		}
		/*Región crítica*/
		tarea = (tarea_h *)desencolar_lista(t_pendientes);
		err_read= pthread_mutex_unlock(&mutex_t_pendientes);
		if (err_read){
			*err = E_UNLOCK;
			break;
		}
		if (tarea->instruccion == DWC)
			*err=ejecutar_wc_h(tarea->archivo);
		else if (tarea->instruccion == DGREP)
			*err=ejecutar_grep_h(tarea->archivo, tarea->string);
		else //fin del programa
			break;
		if(*err<0)
			break;
		free(tarea);
		tarea = NULL;
		sem_post(&listo); //Para notificar al maestro que otra tarea está lista
		
	}
	if (tarea != NULL)
		free(tarea);
	if(*err<0)
		finalizar(*err);
	return (void*)err;
}

/*Función ejecutar_wc_h: Llama a la funcion wc para obtener el numero de
 líneas y de palabras del archivo dado. Crea un nuevo nodo con datos del 
 tipo "resultado" guarda los valores correspondientes. Si ocurre un error
 en el wc se agrega el archivo a la lista de errores, en caso contrario 
 se agrega a tarea listas*/
int ejecutar_wc_h(char * archivo){
	int num_lineas, num_palabras;
	int error=0;
	int verificacion, err_write;
	verificacion = wc(archivo, &num_lineas, &num_palabras);
	if(verificacion<0){
		error = 1;
	} 
	//Preparo el nuevo nodo para la lista
	nodo * nuevo = malloc(sizeof(nodo));
	if(nuevo == NULL)
		return E_MEMORY;
	nuevo->datos = (resultado*)malloc(sizeof(resultado));
	if(nuevo->datos == NULL)
		return E_MEMORY;
	resultado * r = (resultado *)nuevo->datos;
	strcpy(r->archivo, archivo);
	/*Escritura de los trabajadores*/	
	if(error){
		err_write = pthread_mutex_lock(&mutex_t_errores);
		if (err_write)
			return E_LOCK;
		agregar_nodo(nuevo, t_errores);
		err_write=pthread_mutex_unlock(&mutex_t_errores);
		if (err_write)
			return E_UNLOCK;
	}else{
		r->num_lineas = num_lineas;
		r->num_palabras = num_palabras;
		err_write = pthread_mutex_lock(&mutex_t_listas);
		if (err_write)
			return E_LOCK;
		agregar_nodo(nuevo, t_listas);
		err_write=pthread_mutex_unlock(&mutex_t_listas);
		if (err_write)
			return E_UNLOCK;
	}
	return 0;	
}

/*Función ejecutar_wc_h: Llama a la funcion grep para saber si el archivo 
 dado contiene el string indicado. Crea un nuevo nodo con datos del 
 tipo "resultado" guarda los valores correspondientes. Si ocurre un error
 en el grep se agrega el archivo a la lista de errores, en caso contrario 
 se agrega a tarea listas*/
int ejecutar_grep_h(char * archivo, char * string){
	bool contiene;
	int error, verificacion, err_write;
	verificacion= grep(archivo, string, &contiene);
	if(verificacion<0)
		error = 1;
	else 
		error = 0;
	//Preparo el nuevo nodo para la lista
	if(contiene){
		nodo * nuevo = malloc(sizeof(nodo));
		if(nuevo == NULL)
			return E_MEMORY;
		nuevo->datos = (resultado*)malloc(sizeof(resultado));
		if(nuevo->datos == NULL)
			return E_MEMORY;
		resultado * r = (resultado *)nuevo->datos;
		strcpy(r->archivo, archivo);
		/*Escritura de los trabajadores*/	
		if(error){
			err_write = pthread_mutex_lock(&mutex_t_errores);
			if (err_write)
				return E_LOCK;
			agregar_nodo(nuevo, t_errores);
			err_write=pthread_mutex_unlock(&mutex_t_errores);
			if (err_write)
				return E_UNLOCK;
		}else{
			err_write = pthread_mutex_lock(&mutex_t_listas);
			if (err_write)
				return E_LOCK;
			agregar_nodo(nuevo, t_listas);
			err_write=pthread_mutex_unlock(&mutex_t_listas);
			if (err_write)
				return E_UNLOCK;
		}
	}
	return 0;	
}


void finalizar(int error){
	print_error(error);
}

/* procesos.c
 * 
 * Implementación de subrutinas para el funcionamiento de los modos que 
 * utilizan procesos para obtener el nivel de concurrencia.
 * 
 * Autores:
 *   Arianna Amaya, 12-11115.
 * 
 * Fecha: 23 de junio de 2016.
 */

#include "procesos.h"

/*Función memoria_compartida: Crea un espacio de memoria del tamaño de un
 arreglo de 2 mutex con los permisos indicados. */
int memoria_compartida(pthread_mutex_t ** mutex){
	/*S_IRWXU | S_IRWXG Permiso para leer, escribir y ejecutar por el usuario y
	 el grupo*/
	int memoria = shm_open("/memoria_para_procesos", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG);
	if (memoria < 0)
		return E_MSHARED;
	/*Tamaño de la memoria compartida*/
	if (ftruncate(memoria, sizeof(pthread_mutex_t)*2) < 0)
		return E_MSHARED;
	
	/*PROT_READ | PROT_WRITE Las páginas pueden ser leídas y escritas*/
	*mutex = mmap(NULL, sizeof(pthread_mutex_t)*2, PROT_READ | PROT_WRITE, MAP_SHARED,
				memoria, 0);
	if (*mutex < 0)
		return E_MSHARED;
	
	pthread_mutexattr_t atributos;
	pthread_mutexattr_init(&atributos);
	pthread_mutexattr_setpshared(&atributos, PTHREAD_PROCESS_SHARED);
	int err=pthread_mutex_init(&(*mutex)[READ], &atributos);
	if(err) 
		return E_MUTEX;
	err=pthread_mutex_init(&(*mutex)[WRITE], &atributos);
	if(err) 
		return E_MUTEX;
	if(pthread_mutexattr_destroy(&atributos)!=0)
		return E_MUTEX;
	return memoria;
}

/*Creación de dos pipes para la comunicación y un abanico de procesos 
 trabajadores que llaman a su rutina principal "rutina_esclavos"*/
int crear_trabajadores(int n_concurrencia, int modo, int * maestro_pipe,
						pthread_mutex_t* mutex, int memoria_compartida, 
						char*raiz_sistema){
	int fd [2][2]; 
	pipe(fd[0]); // Un pipe para que el maestro escriba 
	pipe(fd[1]); //Un pipe para que los esclavos escriban
	pid_t hijo;
	
	int i;
	for(i=0; i<n_concurrencia; i++){ //Creación de un abanico de procesos
		hijo = fork();
		if (hijo == -1)
			return E_FORK;
		else if(hijo==0){ 
			close(fd[0][1]);
			close(fd[1][0]);
			free(raiz_sistema);
			rutina_esclavos(modo, fd[0][0],fd[1][1], mutex, memoria_compartida);
			break;
		}
	}
	close(fd[0][0]);
	close(fd[1][1]);
	maestro_pipe[READ] = fd[1][0];
	maestro_pipe[WRITE] = fd[0][1];
	return 0;
}

int tipo_comando(char**arg, int nro_arg, char*raiz_sistema, int maestro_pipe[],
				int n_concurrencia){
	int v;
	if (nro_arg == 0)
		return 0;
	else if(strcmp(arg[0], "dwc")==0) 
		v=ejecutar_comando(arg, nro_arg, raiz_sistema, maestro_pipe, n_concurrencia, DWC);
	else if(strcmp(arg[0], "dgrep")==0)
		v=ejecutar_comando(arg, nro_arg, raiz_sistema, maestro_pipe, n_concurrencia, DGREP);
	else if(strcmp(arg[0], "dquit")==0) //Salir del shell
		return DQUIT;
	else
		return NOVALIDO;
	return v;
}


/*****Funciones del maestro******/

/*Funcion principal del maestro: Llama a las funciones recursivas 
 correspondientes. Se lleva un registro de las tareas enviadas por el maestro
 y se compara con las recibidas para determinar cuando los esclavos finalizan
 de procesar los archivos. Se utilizan las funciones de listas.h para ordenar 
 e imprimir las listas de datos y de errores*/
int ejecutar_comando(char** arg, int nro_arg, char* raiz_sistema, int maestro_pipe[],
			int n_concurrencia,char instruccion){
	if(nro_arg!=2)
		return E_PARAMETROS;
	lista * datos = crear_lista(); /*Contiene los resultados del comando*/
	if(datos == NULL)
		return E_MEMORY;
	lista * errores = crear_lista(); /*Contiene los archivos en los cuales ocurrió 
										un error y no pudo terminar de procesarse*/
	if(datos == NULL)
		return E_MEMORY;
	int enviados =0, recibidos =0;
	int verificacion;
	if(instruccion == DWC)
		verificacion = dwc_r(raiz_sistema, arg[1], maestro_pipe, n_concurrencia,
								&enviados, &recibidos, datos, errores);
	else
		verificacion = dgrep_r(raiz_sistema, arg[1], maestro_pipe, n_concurrencia,
								&enviados, &recibidos, datos, errores);
	if(verificacion<0)
		return verificacion;
	if (enviados != recibidos) //Se reciben las tareas restantes
		verificacion = recibir_datos(maestro_pipe, (enviados-recibidos), &recibidos,
									enviados, datos, errores, instruccion);
	if(verificacion<0)
		return verificacion;
	ordenar_lista(datos);
	ordenar_lista(errores);
	int tam_raiz = strlen(raiz_sistema);
	if(instruccion == DWC)
		imprimir_lista(datos, LISTA_T1, tam_raiz);
	else
		imprimir_lista(datos, LISTA_T2, tam_raiz);
	if(!vacia_lista(*errores)){
		printf("En los siguientes archivos ocurrió un error:\n");
		imprimir_lista(errores, LISTA_T2, tam_raiz);
	}
	borrar_lista(datos);
	borrar_lista(errores);
	return 0;
}

/*Función dcw_r: Se desplaza por los directorios desde la raíz del sistema.
 Emplea las funciones tipo_archivo() y tiene_extension() para determinar que
 archivos debe enviar. Cada "n_concurrencia" tareas enviadas procede a recibir
 resultados para evitar que se produzca interbloqueo por llenarse los pipes */
int dwc_r(char*ruta, char*ext, int maestro_pipe[], int n_concurrencia, 
			int * enviados, int *recibidos, lista *datos, lista *errores){
	int verificacion;
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
			ruta_auxiliar = siguiente_ruta(entrada->d_name, ruta);
			if(ruta_auxiliar==NULL)
				return E_MEMORY;
			if(tipo_archivo(ruta_auxiliar) == REGULAR){ //Archivo regular
				if(tiene_extension(entrada->d_name, ext)){
    	   			if(enviar_datos(maestro_pipe, ruta_auxiliar, DWC, NULL)!=0)
    	   				return E_WRITE;
    	   			(*enviados)++;
    	   			if ((*enviados % n_concurrencia) == 0){ //Para evitar que el pipe se llene
    	   				verificacion=recibir_datos(
    	   					maestro_pipe, n_concurrencia, 
    	   					recibidos, *enviados, datos, errores, DWC
    	   				);
    	   				if(verificacion<0)
    	   					return verificacion;
    	   			}
    	   		}
    	   	}	
   	  		dwc_r(ruta_auxiliar, ext, maestro_pipe, n_concurrencia, enviados, recibidos, datos, errores);
   	  		free(ruta_auxiliar);
   	  	}
   	   entrada =readdir(directorio);
    }
    if(closedir(directorio)==-1)
    	return E_CLOSE;
    return 0;
}


/*Función dgrep_r: Se desplaza por los directorios desde la raíz del sistema.
 Emplea la funcion tipo_archivo() para determinar que archivos debe enviar.
 Cada "n_concurrencia" tareas enviadas procede a recibir resultados para evitar
 que se produzca interbloqueo por llenarse los pipes */
int dgrep_r(char*ruta, char*string, int maestro_pipe[], int n_concurrencia,
			int * enviados, int *recibidos, lista *datos, lista *errores){
	int verificacion;
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
			ruta_auxiliar = siguiente_ruta(entrada->d_name, ruta); 
			if(ruta_auxiliar==NULL)
				return E_MEMORY;
			if(tipo_archivo(ruta_auxiliar) == REGULAR){ //Archivo regular
    	   			if(enviar_datos(maestro_pipe, ruta_auxiliar, DGREP, string)!=0)
    	   				return E_WRITE;
    	   			(*enviados)++;
    	   			//Para evitar que el pipe se llene
    	   			if ((*enviados % n_concurrencia) == 0){ 
    	   				verificacion=recibir_datos(
    	   					maestro_pipe, n_concurrencia, 
    	   					recibidos, *enviados, datos, errores, DGREP
    	   				); //TODO
    	   				if(verificacion<0)
    	   					return verificacion;
    	   			}
    	   	}
   	  		dgrep_r(ruta_auxiliar, string, maestro_pipe, n_concurrencia, enviados,
					recibidos, datos, errores);
   	  		free(ruta_auxiliar);
   	  	}
   	   entrada =readdir(directorio);
    }
    if(closedir(directorio)==-1)
    	return E_CLOSE;
    return 0;
}

/*Manejo de datos*/
/*Función enviar_datos: Escribe en el pipe la nueva tarea. Cada vez que 
 se realiza una escritura se revisa si el pipe ha sido cerrado por algún
 error con los esclavos*/
int enviar_datos(int maestro_pipe[], char * archivo, char instruccion, char * string){
	ssize_t len = write(maestro_pipe[WRITE], (void *)&instruccion, sizeof(char));
	if(len == -1)
		return E_WRITE;
	if(len ==0) //El pipe de escritura de los esclavos fue cerrado por algún error
		return E_COM;
	if(instruccion == DGREP){
		int tam_string = strlen(string);
		len = write(maestro_pipe[WRITE], (void *)&tam_string, sizeof(int));
		if(len == -1)
			return E_WRITE;
		if(len == 0)
			return E_COM;
		len = write(maestro_pipe[WRITE], (void *)string, tam_string);
		if(len == -1)
			return E_WRITE;
		if(len == 0)
			return E_COM;
	}
	int tam_archivo = strlen(archivo);
	len = write(maestro_pipe[WRITE], (void *)&tam_archivo, sizeof(int));
	if(len == -1)
		return E_WRITE;
	if(len == 0)
		return E_COM;
	len = write(maestro_pipe[WRITE], (void *)archivo, tam_archivo);
	if(len == -1)
		return E_WRITE;
	if(len == 0)
		return E_COM;
	return 0;
}

/*Función recibir_datos:  Cada vez que se realiza una lectura se revisa si
 el pipe ha sido cerrado por algún error con los esclavos. Los datos leídos
 son alamacenados en un nuevo nodo que es añadido a la lista de datos o de 
 errores según sea el caso*/
int recibir_datos(int maestro_pipe[], int leer, int*recibidos, 
					int enviados, lista* datos, lista* errores, char inst){
	int tam;
	ssize_t len;
	int i = enviados-leer;
	int error;
	while(i<enviados){
		nodo * nuevo = malloc(sizeof(nodo));
		if(nuevo == NULL)
			return E_MEMORY;
		nuevo->datos = (resultado*)malloc(sizeof(resultado));
		if(nuevo->datos == NULL)
			return E_MEMORY;
		resultado * r = (resultado *)nuevo->datos;
		len = read(maestro_pipe[READ], (void *)&tam , sizeof(int)); 
		if(len == -1)
			return E_READ;
		if(len == 0)
			return E_COM;
		len = read(maestro_pipe[READ], (void *)r->archivo , tam);
		if(len == -1)
			return E_READ;
		if(len == 0)
			return E_COM;
		r->archivo[tam]='\0';
		len = read(maestro_pipe[READ],(void*)&error, sizeof(int));
		if(len == -1)
			return E_READ;
		if(len == 0)
			return E_COM;
		if(error == 1){
			agregar_nodo(nuevo, errores);
		}else{
			bool contiene;
			if (inst == DWC){
				len = read(maestro_pipe[READ],(void*)&(r->num_lineas),sizeof(int));
				if(len == -1)
					return E_READ;
				if(len == 0)
					return E_COM;	
				len = read(maestro_pipe[READ],(void*)&(r->num_palabras),sizeof(int));
				if(len == -1)
					return E_READ;
				if(len == 0)
					return E_COM;
				contiene = true;
			} else {
				len = read(maestro_pipe[READ], (void *)&contiene, sizeof(bool));
				if(len == -1)
					return E_READ;
				if(len == 0)
					return E_COM;
			}
			if (contiene) {
				agregar_nodo(nuevo, datos);
			} else {
				free(nuevo->datos);
				free(nuevo);
			}
		}
		i++;
		(*recibidos)++;
	}
	return 0;
}

/*****Funciones de los esclavos******/

/*Función rutina_esclavos: Es la función que llaman los esclavos después 
 de su creación. Utiliza un mutex que le indica a los esclavos cuando es
 su turno para leer. Si el maestro no ha escrito nada en el pipe los
 esclavos se mantien bloquedos.*/
int rutina_esclavos(int modo, int entrada, int salida, pthread_mutex_t* mutex,
					int memoria_compartida){
	int err_read, err =0;
	ssize_t len;
	char instruccion;
	int tam_archivo, tam_string;
	char nombre_archivo [MAX_NOMBRE], string[MAX_STRING];
	while(1){
		/*Lectura de los trabajdores*/
		err_read = pthread_mutex_lock(&mutex[READ]);
		if (err_read){
			err = E_LOCK;
			break;
		}
		/*Inicio de la región critica*/
		len = read(entrada, (void *)&instruccion, sizeof(char));
		if(len == 0)  //El pipe de escritura ha sido cerrado.
			break;
		if(len ==-1){
			err = E_READ;
			break;
		}
		if(instruccion == DGREP){
			len = read(entrada, (void *)&tam_string, sizeof(int));
			if(len ==-1){
				err = E_READ;
				break;
			}
			len = read(entrada, (void *)string, tam_string); 
			if(len ==-1){
				err = E_READ;
				break;
			}
			string[tam_string]='\0';
		}
		len = read(entrada, (void *)&tam_archivo, sizeof(int)); 
		if(len ==-1){
			err = E_READ;
			break;
		}
		len = read(entrada, (void *)nombre_archivo, tam_archivo);
		if(len ==-1){
			err = E_READ;
			break;
		}
		nombre_archivo[tam_archivo]='\0';
		/*Fin de la región crítica*/
		err_read=pthread_mutex_unlock(&mutex[READ]);	
		if (err_read){
			err = E_UNLOCK;
			break;
		}
		if(instruccion == DWC)
			err=ejecutar_wc(nombre_archivo,salida,mutex,modo);
		else
			err=ejecutar_grep(nombre_archivo,string,salida,mutex,modo);
		if(err<0)
			break;
	}
	err_read=pthread_mutex_unlock(&mutex[READ]);	
	if (err_read)
			err = E_UNLOCK;
	cerrar(entrada, salida, memoria_compartida);
	exit(err);
}
		
void cerrar(int entrada, int salida, int memoria_compartida){
	close(entrada);
	close(salida);	
	close(memoria_compartida);
}

/*Función ejecutar_wc: Llama a la funcion wc() o wc_sist() para obtener 
 el numero de líneas y de palabras del archivo dado. Envía el resultado 
 por el pipe.*/
int ejecutar_wc(char * archivo,int salida,pthread_mutex_t* mutex,int modo){
	int num_lineas, num_palabras;
	int error=0;
	int verificacion;
	if(modo == 0) 
		verificacion = wc(archivo, &num_lineas, &num_palabras);
	else
		verificacion = wc_sist(archivo, &num_lineas, &num_palabras);
	if(verificacion<0){
		error = 1;
	}
	int tam = strlen(archivo); 
	/*Escritura de los trabajadores*/	
	int err_write = pthread_mutex_lock(&mutex[WRITE]);
	if (err_write)
		return E_LOCK;	
	/*Inicio de región critica*/
	write(salida, (void *)&tam, sizeof(int));
	write(salida, (void *)archivo, tam);
	write(salida, (void *)&error, sizeof(int));
	if(!error){
		write(salida, (void *)&num_lineas, sizeof(int)); 
		write(salida, (void *)&num_palabras, sizeof(int));
	}
	/*Fin de región crítica*/
	err_write=pthread_mutex_unlock(&mutex[WRITE]);
	if (err_write)
		return E_UNLOCK;
	return 0;	
}

/*Función ejecutar_grep: Llama a la funcion grep() o grep_sist() para saber
 si el archivo dado contiene el string indicado. Envía el resultado por 
 el pipe.*/
int ejecutar_grep(char*archivo, char*string, int salida, pthread_mutex_t* mutex,
				int modo){
	bool contiene;
	int error;
	int verificacion;
	if(modo == 0)
		verificacion= grep(archivo, string, &contiene);
	else
		verificacion= grep_sist(archivo, string, &contiene);
	if(verificacion<0)
		error = 1;
	else 
		error = 0;
	int tam = strlen(archivo); 
	/*Escritura de los trabajadores*/	
	int err_write = pthread_mutex_lock(&mutex[WRITE]);
	if (err_write)
		return E_LOCK;	
	/*Inicio de región critica*/
	write(salida, (void *)&tam, sizeof(int));
	write(salida, (void *)archivo, tam);
	write(salida, (void *)&error, sizeof(int));
	if (!error)
		write(salida, (void *)&contiene, sizeof(bool));
	/*Fin de región crítica*/
	err_write=pthread_mutex_unlock(&mutex[WRITE]);
	if (err_write)
		return E_UNLOCK;
	return 0;	
}


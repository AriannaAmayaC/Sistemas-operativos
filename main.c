/* main.c
 * 
 * Rutina principal de dsheel. 
 * 
 * Autores:
 *   Arianna Amaya, 12-11115.
 * 
 * Fecha: 21 de Junio de 2016.
 */
#include "procesos.h"
#include "hilos.h"

void main_procesos(char * raiz_sistema, int n_concurrencia, int modo);

void main_hilos(char * raiz_sistema, int n_concurrencia);

/*Se utiliza la función getopt para obtener los valores de las opciones
 m y n. Está función organiza argv[] de tal forma que quedan las opciones
 al principio y al final los demás parámetros*/
int main(int argc, char *argv[]){
	int modo = 0; //Valor por defecto
	int n_concurrencia =1; // Valor por defecto
	int opt;
	opterr = 0;
	while ((opt = getopt(argc, argv, ":n:m:")) != -1){
		switch(opt) {
			case 'm':
				modo =atoi(optarg);
				break;
			case 'n':
				n_concurrencia=atoi(optarg);
				break;
			case '?':
				printf("Opción desconocida %c\n", optopt);
				exit(1);
			case ':':
				printf("Falta el parámetro de la opción %c\n", optopt);
				exit(1);
		}
	}
	/*optind apunta al primer elemnto después de las opciones*/
	if (optind != argc - 1) { 
		printf("Uso incorrecto. Llame al programa con el siguiente formato:\n");
		printf("#dshell <directorio> [-n concurrencyLevel] [-m mode]\n");
		return -1;
	} 
	int cambio_dir= chdir(argv[optind]);//Se cambia a la raíz del sistema
	if(cambio_dir== -1){
		perror("");
		return -1;
	}
	char* raiz_sistema = directorio_actual(); //Se guarda la ruta absoluta
	if(raiz_sistema == NULL){
		print_error(E_MEMORY);
		return -1;
	}
	if ((modo == 0)||(modo ==1))
		main_procesos(raiz_sistema, n_concurrencia, modo);
	else if(modo == 2)
		main_hilos(raiz_sistema, n_concurrencia);
	else {
		printf("Modo no válido\n");
		return -1;
	}
	return 0;
}

/*Programa principal correspondiente a los modos que utilizan procesos para obtener 
el nivel de concurrencia*/
void main_procesos(char * raiz_sistema, int n_concurrencia, int modo){
	char* prompt= "prompt>";
	char** arg;
	int nro_arg;
	int verificacion;
	
	/*Comunicacion*/
	int fd[2];
	pthread_mutex_t* mutex;
	int memoria = memoria_compartida(&mutex);
	if(memoria<0)
		print_error(memoria);
	verificacion= crear_trabajadores(n_concurrencia,modo,&(*fd),mutex,memoria,raiz_sistema);
		if(verificacion<0)
			print_error(verificacion);
	/*Shell*/
	while(1){
		printf("%s ", prompt);
		fflush(stdout);
		leer_entrada(&arg, &nro_arg);
		verificacion =tipo_comando(arg, nro_arg, raiz_sistema, fd, n_concurrencia);
		borrar_entrada(arg, nro_arg);
		if((verificacion == DQUIT)|| verificacion == E_COM){
			free(raiz_sistema);
			break;
		}
		if(verificacion<0)
			print_error(verificacion);	
	}
	cerrar(fd[READ], fd[WRITE], memoria);
	pthread_mutex_destroy(mutex);
	shm_unlink("/memoria_para_procesos");
	int i, status;
	for(i=0; i<n_concurrencia; i++){
		wait(&status);
		if(status) 
			print_error(WEXITSTATUS(status));
	}
	
}

/*Programa principal correspondiente al que utiliza hilos para obtener el
 nivel de concurrencia*/
void main_hilos(char * raiz_sistema, int n_concurrencia){
	char* prompt= "prompt>";
	char** arg;
	int nro_arg;
	int verificacion;
	
	/*Comunicacion*/
	verificacion = comunicacion_h();
	if(verificacion<0)
		print_error(verificacion);
	pthread_t thread[n_concurrencia];
	verificacion= crear_trabajadores_h(n_concurrencia, thread);
	if(verificacion<0)
		print_error(verificacion);
	
	/*Shell*/
	while(1){
		printf("%s ", prompt);
		fflush(stdout);
		leer_entrada(&arg, &nro_arg);
		verificacion =tipo_comando_h(arg, nro_arg, raiz_sistema);
		borrar_entrada(arg, nro_arg);
		if(verificacion == DQUIT){
			verificacion = fin_trabajadores(n_concurrencia);
			free(raiz_sistema);
			break;
		}
		if(verificacion<0)
			print_error(verificacion);
		
	}
	
	int i;
	void*status;
	pthread_join(thread[i], &status);
	//Verificar error
}

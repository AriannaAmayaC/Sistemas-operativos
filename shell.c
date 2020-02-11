/* shell.c
 * 
 * Implementación de los comandos y subrutinas para el funcionamiento del 
 * shell.
 * 
 * Autores:
 *   Arianna Amaya, 12-11115.
 * 
 * Fecha: 23 de junio de 2016.
 */
#include "shell.h"


/*Función remover_espacios: Remueve espacios innecesarios de los comandos*/
void remover_espacios(int tam, char * buffer) {
	char copia[tam+1];
	int i;
	int j = 0;
	bool copiar_sig = false;
	
	if (tam == 1)
		return;
	//Cuando consigue una serie de espacios, copia solo el primero
	for (i = 0; buffer[i] != '\0'; i++) {
		if (buffer[i] == ' '){
			if (copiar_sig) {
				copiar_sig = false;
				copia[j] = buffer[i];
				j++;
			}
		} else {
			copiar_sig = true;
			copia[j] = buffer[i];
			j++;
		}
	}

	//si se copió un espacio antes del \n, se elimina
	if (copia[j-2] == ' '){
		copia[j-2] = '\n';
		copia[j-1] = '\0';
	} else
		copia[j] = '\0';
	strcpy(buffer, copia);
}

/*Función datos_entrada: Se recorre el buffer para contar las palabras y su 
tamaño respectivo. Luego se asigna dinámica y se almacena cada palabra en un
string del arreglo arg*/
int datos_entrada(char * buffer, char *** arg, int* nro_arg){
	int i=0, j=0;
	int tamano_buf = strlen(buffer);
	int tamano_palabra;
	int palabras = 0;
	if (buffer[0] == '\n'){
		*nro_arg = 0;
		return 0;
	}
	for (i = 0; i < tamano_buf; i++){
		if((buffer[i]==' ') || (buffer[i]=='\n'))
			palabras++;
	}
	*arg= (char **)malloc(sizeof(char*)*palabras);
	if(*arg==NULL) 
		return E_MEMORY;
	for(i=0; i<palabras; i++){
		tamano_palabra = 0;
		while(1){
			if((buffer[j]==' ')|| (buffer[j]=='\n')){ 
				j++;
				break;
			}
			tamano_palabra++;
			j++;
		}
		(*arg)[i] = (char*)malloc(sizeof(char)*(tamano_palabra+1));
		if((*arg)[i]==NULL){ 
			return E_MEMORY;
		}
	}
	
	j=0;
	int k;
	for (i = 0; i < palabras; i++) {
		k = 0;
		while (buffer[j] != ' ' && buffer[j] != '\n') {
			(*arg)[i][k] = buffer[j];
			j++;
			k++;
		}
		(*arg)[i][k] = '\0';
		j++;
	}
	*nro_arg = palabras;
	return 0;
}

/*Función leer_entrada: Con fgets se obtienen los datos desde la entrada estándar.
Si se introduce más datos que MAX_COMANDO se reasigna memoria. Los datos son 
organizados con la función datos_entrada()*/
int leer_entrada(char *** arg, int* nro_arg){
	char * buffer = malloc(sizeof(char)*MAX_COMANDO+1);
	if(buffer == NULL)
		return E_MEMORY;
	fgets(buffer, MAX_COMANDO+1, stdin);
	int tamano_s= strlen(buffer);
	int tamano_nuevo;
	while(1){
		if (buffer[tamano_s-1]!='\n'){
			tamano_nuevo = tamano_s*2;
			buffer = realloc(buffer, sizeof(char)*tamano_nuevo+1);
			if(buffer == NULL)
				return E_MEMORY;
			fgets(&buffer[tamano_s], (tamano_nuevo-tamano_s+1) , stdin);
			tamano_s =strlen(buffer);
		} else 
			break;
	}
	remover_espacios(tamano_s, buffer);
	datos_entrada(buffer, arg, nro_arg);
	free(buffer);
	return 0;
}

void borrar_entrada(char ** arg, int nro_arg){
	int i;
	for(i=0; i<nro_arg; i++){
		free(arg[i]);
	}
	if (nro_arg != 0)
		free(arg);
}

int tipo_ruta(char *ruta){
	if(strcmp(ruta, "..")==0)
		return PADRE;
	else if(ruta[0]== '/')
		return ABSOLUTA;
	else 
		return RELATIVA;	
}

/*Función tipo_de_archivo: Utiliza macros para determinar si el archivo es un
link simbolico, un archivo regular o un directorio*/
int tipo_archivo(char * ruta){ 
	struct stat sb;
	if(lstat(ruta, &sb)==-1)
		return E_STAT;
	if(S_ISREG(sb.st_mode))
			return REGULAR;
	if(S_ISDIR(sb.st_mode))
			return DIRECTORIO;
	if(S_ISLNK(sb.st_mode)){
		if(stat(ruta, &sb)==-1)
			return E_STAT;
		if(S_ISREG(sb.st_mode))
			return LINKR;
		else if(S_ISDIR(sb.st_mode))
			return LINKD;
	}
	return 4;	
}

char * directorio_actual(void){
	char * posicion_actual = malloc(MAX_RUTA);
	if(posicion_actual == NULL)
		return NULL;
	int tamano = MAX_RUTA;
	char * verificacion= getcwd(posicion_actual, tamano);
	while(1){	
		if(verificacion == NULL){
			if (errno == ERANGE ){
				tamano = 2*tamano;
				posicion_actual= realloc(posicion_actual, tamano);
				if(posicion_actual == NULL){
					free(posicion_actual);
					return NULL;	
				}
				verificacion= getcwd(posicion_actual, tamano);
			}
		}
		else 
			break;
	}
	return posicion_actual;
}

char * ruta_real(char* destino, char * raiz_sistema){
	char * destino_abs;
	int tipo = tipo_ruta(destino);
	if(tipo == ABSOLUTA){
		destino_abs=malloc(sizeof(char)*(strlen(destino)+strlen(raiz_sistema)+2));
		if(destino_abs == NULL)
			return NULL;
		strcpy(destino_abs, raiz_sistema);
		strcat(destino_abs, "/");
		strcat(destino_abs, destino);
		}
	else{
		destino_abs=malloc(sizeof(char)*((int) strlen(destino)+1));
		strcpy(destino_abs, destino);
		}
	return destino_abs;
}

/*Función siguiente_ruta: Asigna espacio de memoria y concatena la posición 
actual con el destino*/
char * siguiente_ruta(char* destino, char * posicion_actual){
	char * destino_abs;
	destino_abs=malloc(sizeof(char)*(strlen(destino)+strlen(posicion_actual)+2));
	if(destino_abs == NULL)
		return NULL;
	strcpy(destino_abs, posicion_actual);
	strcat(destino_abs, "/");
	strcat(destino_abs, destino);
	return destino_abs;
}

bool tiene_extension( char * archivo, char * ext){
	bool extension = true;
	int tam_archivo_actual =strlen(archivo);
	int tam_ext =strlen(ext);
	int i=0;
	if(tam_archivo_actual <= tam_ext)
		return false;
	while(i<tam_ext){
		if(archivo[tam_archivo_actual-i-1]!=ext[tam_ext-i-1]){
			extension = false;			
			break;
		}
	i++;
	}
	if(archivo[tam_archivo_actual-i-1]!='.')
		extension = false;
	
	return extension; 
}

/*Función wc: Lee frgamentos del archivos que almacena en un buffer. Se desplaza
sobre el buffer para contar las lineas, palabras y caracteres*/
int wc (char *archivo, int * num_lineas, int * num_palabras){
	int fd =open(archivo, O_RDONLY);
	if(fd==-1)
		return E_OPEN;
	char buffer[MAX_ARCHIVO];
	ssize_t len = read (fd, buffer, MAX_ARCHIVO);
	int i;
	bool contado= false; //Se utiliza para marcar una palabra como contada
	
	*num_lineas = 0;
	*num_palabras= 0;
	if(len ==0)
		return 0;
	else{ 
		while(len >0){
			for(i=0; i<len; i++){
				if(buffer[i]=='\n') //Cuenta una linea cada vez que encuentra \n
					(*num_lineas)++;
				/*Para contar las palabras se utiliza una máquina de estados.
				Cuando se lee un caracter se cambia el booleano contado.*/
				if((buffer[i]!='\n')&&(buffer[i]!='\t') &&(buffer[i]!=' ')){
					if(contado == false){ 
						(*num_palabras)++;
						contado = true;
					}
				}
				else
					contado = false;
			}		
			len = read (fd, buffer, MAX_ARCHIVO);
		}
		if(len == -1)
			return E_READ;
	}
	if(close(fd)==-1)
		return E_CLOSE;
	return 0;
}

/*Función wc: Se utiliza la función execlp() para ejecutar el comando wc
 del sistema. Se crea un pipe para que el hijo redireccione la salida
 estándar al extremo de escritura del pipe*/
int wc_sist(char* archivo,int* num_lineas,int* num_palabras){
	int fd[2];
	pipe(fd);
	pid_t hijo = fork();
	if(hijo == -1)
		return E_FORK;
	else if(hijo ==0){
		dup2(fd[1],1);
		close(fd[1]);
		close(fd[0]);
		if(execlp("wc","wc", archivo,(char*)NULL)==-1);
   	 		exit(1); 
	}
	else{
		close(fd[1]);
		char buffer[1024];
		ssize_t len = read(fd[0], buffer, 1024);
		if(len == -1)
			return E_READ;
		buffer[len]='\0';
		wait(NULL);
		sscanf(buffer, "%d %d", num_lineas, num_palabras);
		close(fd[0]);
	}
	return 0;
}

/*Función grep: Se diseñaron estructuras intermedias (escritor y lector) para que 
la función sea eficiente. Está función trabaja por líneas. Leer_linea() devuelve
una linea de caracteres que busca la primera ocurrencia del string con la
función strstr().*/
int grep(char * archivo, char * string, bool* contiene){
	*contiene = false;
	int fd_leer = open(archivo, O_RDONLY);
	if(fd_leer==-1)
		return E_OPEN;
	char buffer[MAX_ARCHIVO];
	lector entrada;  //Estructuras intermedias
	nuevo_lector(&entrada, fd_leer); 
	int len_buffer = leer_linea(&entrada, buffer);
	while(len_buffer > 0) {
		if(strstr(buffer, string)!=NULL){
			*contiene = true;
			break;
		}
		len_buffer = leer_linea(&entrada, buffer);
	}
	if(len_buffer < 0)
		return E_READ; //El mismo tipo de error retornado por leer_linea()
	if(close(fd_leer) == -1)
		return E_CLOSE;
	return 0;
}


void nuevo_lector(lector *l, int fd) {
	l->fd = fd;
	l->proximo = MAX_ARCHIVO;
	l->limite = MAX_ARCHIVO;
}

/*Función leer_lineas: Se lee por fragmentos de tamaño MAX_ARCHIVO. Se 
almacenan los datos hasta el primer /n en un buffer que será retornado. 
Cada vez que se lean todos los caracteres almacenados en datos[] se procede
a realizar otra lectura del archivo*/
int leer_linea(lector * l, char * buffer){
	int leidos = 0;
	while (1) {
		if (l->proximo == l->limite) {
			if (l->limite < MAX_ARCHIVO){
				buffer[l->limite] = '\0';
				return leidos;
			}
			else {
				l->limite = read(l->fd, l->datos, MAX_ARCHIVO);
				if(l->limite ==-1)
					return E_READ;
				l->proximo = 0;	
			}
		} else {
			buffer[leidos] = l->datos[l->proximo];
			leidos++;
			l->proximo++;
			if(leidos == MAX_ARCHIVO -1) {
				buffer[MAX_ARCHIVO-1] = '\0';
				return leidos ;
			}
			if (l->datos[l->proximo-1] == '\n'){
				buffer[leidos-1] = '\0';
				return leidos;
			}
		}
	}
}

/*Función wc: Se utiliza la función execlp() para ejecutar el comando grep
 del sistema. Se crea un pipe para que el hijo redireccione la salida
 estándar al extremo de escritura del pipe. Si el hijo escribe en el pipe
 es porque el archivo contiene el string*/
int grep_sist(char*archivo, char*string, bool *contiene){
	int fd[2];
	pipe(fd);
	pid_t hijo = fork();
	if(hijo == -1)
		return E_FORK;
	else if(hijo ==0){
		dup2(fd[1],1);
		close(fd[1]);
		close(fd[0]);
		if(execlp("grep","grep",string, archivo,(char*)NULL)==-1);
   	 			exit(1); 
	}
	else{
		close(fd[1]);
		char buffer[1024];
		ssize_t len = read(fd[0], buffer, 1024);
		if(len == -1)
			return E_READ;
		else if (len ==0)
			*contiene = false;
		else
			*contiene = true;
		wait(NULL);
		close(fd[0]);
	}
	return 0;	
}

/*Mensajes de error*/
char* msj_error[18] ={
	"",
	"Comando no valido",
	"Uso incorrecto. Error en la cantidad de párametros",
	"Error de asignación de memoria",
	"Error en la apertura de archivos",
	"Error en la lectura desde el pipe",
	"Error en la escritura desde el pipe",
	"Error en el cierre de archivos",
	"Error al analizar propiedades del archivo ",
	"Error en la apertura de directorios",
	"Error en exec()",
	"Error en fork()",
	"Error en mutex_lock()",
	"Error en mutex_unlock()",
	"Error en pcreate()",
	"Problema de comunicación con los hijos",
	"Error en los semáforos",
	"Error en los mutex"
};

bool terminar[18]={
	false,
	false,
	false,
	true,
	false,
	true,
	true,
	false,
	false,
	false,
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	true
};
	

void print_error(int error){
	if(error == E_MSHARED) {
		perror("");
		exit(1);
	} else
		puts(msj_error[abs(error)]);
	if (terminar[abs(error)])
		exit(1);
}

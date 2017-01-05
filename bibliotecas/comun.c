#include "comun.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h> 	//Para Mutex
#include <time.h> 		//Para Mutex
#include <sys/types.h> 	//Para Portabilidad
#include <fcntl.h> 		//Para open
#include <sys/ioctl.h> 	//Para tamanio de consola
#include <sys/stat.h>	//Para Stat
#include <unistd.h>		//Para Evitar Warnings

#include "../lib/commons/string.h"



//Define Interno, no les debe Importar a ustedes
#define NOMBRE_ARCHIVO_PRUEBA "PruebaTemporalParaPermisos.txt"


//Inicializo un Mutex para el Access. Le tuve que poner Mutex porque nadie me asegura que Access sea Thread Safe
static pthread_mutex_t Mutex_Interno_Access  = PTHREAD_MUTEX_INITIALIZER;
//Inicializo un Mutex para el Stat. Le tuve que poner Mutex porque nadie me asegura que Stat sea Thread Safe
static pthread_mutex_t Mutex_Interno_Stat  = PTHREAD_MUTEX_INITIALIZER;


int Comun_existeArchivo(const char *srutaLinuxArchivo) {
	//Veo si el archivo existe el archivo y si se tienen permisos de Lectura, Escritura y Ejecucion.
	int numeroError = 0;
	int numeroErrorAccess = 0;

	//Le tuve que poner Mutex porque nadie me asegura que Access sea Thread Safe
	numeroError = pthread_mutex_lock(&Mutex_Interno_Access);
	if (numeroError != 0) {
		Macro_Imprimir_Error("Problema al Pedir Lock Mutex. Numero Error:%d" , numeroError);
		return -1;
	}

	numeroErrorAccess = access(srutaLinuxArchivo , F_OK);

	numeroError = pthread_mutex_unlock(&Mutex_Interno_Access);
	if (numeroError != 0) {
		Macro_Imprimir_Error("Problema al Hacer UnLock Mutex. Numero Error:%d" , numeroError);
		return -1;
	}


	if ( numeroErrorAccess == 0) {
		//Se Abrio Correctamente, devuelvo 0.
		return 0;
	} else {
		//Hubo algun problema al ver si existe el archivo, devuelvo -1 y aviso por consola.
		Macro_ImprimirParaDebug("El archivo o No existe o No se tienen permisos de Lectura/Escritura/Ejecucion.\n");
		Macro_ImprimirParaDebug("La Ruta del archivo es: '%s'\n", srutaLinuxArchivo);
		Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());
		return -1;
	}
}

uint32_t Comun_obtener_tamanio_archivo( const char* sRutaArchivo ){
	uint32_t numeroError = 0;
	int numeroErrorStat = 0;

	//Uso stat para obtener el tamaño del archivo, tengo que usar una estructura auxiliar "sBufferAuxiliar"
	struct stat sBufferAuxiliar;

	//Le tuve que poner Mutex porque nadie me asegura que Stat sea Thread Safe
	numeroError = pthread_mutex_lock(&Mutex_Interno_Stat);
	Macro_Check_And_Handle_Error(numeroError != 0 ,"Problema al Pedir Lock Mutex. Numero Error:%d" , numeroError);

	//Obtengo el tamaño
	numeroErrorStat = stat(sRutaArchivo , &sBufferAuxiliar);

	numeroError = pthread_mutex_unlock(&Mutex_Interno_Stat);
	Macro_Check_And_Handle_Error(numeroError != 0 ,"Problema al Hacer UnLock Mutex. Numero Error:%d" , numeroError);

	Macro_Check_And_Handle_Error(numeroErrorStat == -1 , "Hubo un error y no se pudo determinar el tamaño del archivo. Error en el Stat");

	Macro_Check_And_Handle_Error(sBufferAuxiliar.st_size <= 0 , "Tamanio Invalido del Archivo, Tamanio: %ld" , (long int)sBufferAuxiliar.st_size);

	return sBufferAuxiliar.st_size;


Error_Handler:
	return -1;
}



int Comun_borrarArchivo(const char* pathArchivo) {

	int numeroError = 0;

	numeroError = Comun_existeArchivo(pathArchivo);
	if(numeroError == -1) {
		Macro_Imprimir_Error("El arhivo a borrar no existe en la ruta indicada");
		return -1;
	}

	numeroError = unlink(pathArchivo);
	if(numeroError == -1) {
		Macro_Imprimir_Error("Hubo un error al intentar borrar el archivo indicado");
		return -1;
	}

	return 0;
}

void Comun_ImprimirProgreso(char* Mensaje, size_t TamanioTotal, size_t estado) {
	struct winsize ws;
	static uint32_t progresoActual;
	int tamMensaje = strlen(Mensaje) + 2;
	// Para obtener el ancho de la consola
	static int cantBarras;

	ioctl(0 , TIOCGWINSZ , &ws);

	//ws.ws_col

	int tamanioBloqueTexto = 100 / (ws.ws_col - tamMensaje) + (((100 % (ws.ws_col - tamMensaje)) != 0) ? 1 : 0);
	int tamanioBloqueArch;
	if (TamanioTotal > (ws.ws_col - tamMensaje)) {
		tamanioBloqueArch = TamanioTotal / (ws.ws_col - tamMensaje);
	}

	if (estado == 0) {
		printf("%s [", Mensaje);
		printf(PANTALLA_POSICIONARSE_TABULADO("%d")"]\r" , (100 / tamanioBloqueTexto) - 1);

		printf(PANTALLA_POSICIONARSE_TABULADO("%d") , tamMensaje);
		progresoActual = 0;
		cantBarras = 0;
	}

	if ((estado > 0) && (cantBarras < 100/tamanioBloqueTexto)) {
		if (TamanioTotal <= (ws.ws_col - tamMensaje)) {
			for (cantBarras = 1; cantBarras < (100/tamanioBloqueTexto); cantBarras++){
				printf("\x1b[44m "ANSI_COLOR_RESET);
			}
			fflush(stdout);
			printf("\n");
			// Imprimo toda la barra
		} else {
			if (estado > tamanioBloqueArch) {
				// Imprimo varias barras
				int parteArchivo = 0;
				while (cantBarras < (100/tamanioBloqueTexto)) {
					parteArchivo += tamanioBloqueArch;
					if (parteArchivo >= estado) {
						progresoActual += (parteArchivo - estado);
						break;
					}
					cantBarras++;
					if (cantBarras >= 100/tamanioBloqueTexto)
						printf(ANSI_COLOR_RESET"\n");
					else
						printf("\x1b[44m "ANSI_COLOR_RESET);
					fflush(stdout);
					// verifico si llegue a completar el tamanioBloqueArch
				}
			} else if ((progresoActual+estado) >= tamanioBloqueArch){
				// Imprimo una barra
				cantBarras++;
				// si es la ultima barra imprimo un \n
				if (cantBarras >= 100/tamanioBloqueTexto) {
					printf(ANSI_COLOR_RESET"\n");
				} else {
					printf("\x1b[44m "ANSI_COLOR_RESET);
				}
				fflush(stdout);
				progresoActual -= tamanioBloqueArch;
				progresoActual += estado;
			} else {
				progresoActual += estado;
			}
		}
	}
}

void Comun_ImprimirMensajeConBarras(char* Mensaje) {
	struct winsize ws;
	int tamMensaje = strlen(Mensaje) + 2;
	// Para obtener el ancho de la consola
	ioctl(0 , TIOCGWINSZ , &ws);
	//ws.ws_col
	int CentroPantalla = (ws.ws_col) / 2;
	if (tamMensaje > ws.ws_col) {
		printf("%s", Mensaje);
		return;
	}
	int InicioMensaje = CentroPantalla - (tamMensaje / 2);
	int PosActual, PosFinal;

	printf("\r");
	PosFinal = InicioMensaje;
	for (PosActual = 0; PosActual <= PosFinal; PosActual++) {
		printf("\x1b[46m "ANSI_COLOR_RESET);
	}
	printf(" %s ", Mensaje);
	PosFinal = ws.ws_col;
	for (PosActual += tamMensaje; PosActual < PosFinal; PosActual++) {
		printf("\x1b[46m "ANSI_COLOR_RESET);
	}
	printf("\n");
}




int Comun_controlarPermisos() {
	//Variable para Controlar Errores, por defecto es 0 que significa que no hay error
	int iNumeroError = 0;

	//Controlo que pueda crear archivos creando un archivo temporal
	FILE* archivoTemporal = fopen(NOMBRE_ARCHIVO_PRUEBA, "w+");
	if (archivoTemporal == NULL) {
		Macro_ImprimirParaDebug("No hay Permisos para Crear Archivos \n");
		Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno())
		return -1;
	}
	//Controlo que se pueda cerrar el archivo
	iNumeroError = fclose(archivoTemporal);
	if (iNumeroError != 0) {
		Macro_ImprimirParaDebug("No hay Permisos para Cerrar Archivos, \n");
		Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno())
		return -1;
	}
	//Veo por permisos de Lectura, Escritura y Ejecucion.
	iNumeroError = Comun_existeArchivo(NOMBRE_ARCHIVO_PRUEBA);
	if (iNumeroError != 0) {
		//No hace falta imprimir Nada, se encarga la funcion "Comun_existeArchivo"
		return -1;
	}
	//Veo por permisos para borrar archivos
	iNumeroError = remove(NOMBRE_ARCHIVO_PRUEBA);
	if (iNumeroError != 0) {
		Macro_ImprimirParaDebug("No hay Permisos para Borrar Archivos \n");
		Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno())
		return -1;
	}

	//Si llegamos hasta aca, es que tenemos los permisos
	return 0;
}


char* Comun_obtenerRutaDirectorio(const char* rutaCompleta) {

	//Veo si me Pasaron un Char* NULL
	if (rutaCompleta == NULL) {
		Macro_ImprimirParaDebug("ERROR en 'Comun_obtenerRutaDirectorio', le pasastes un char* NULL. Te devuelvo NULL, asi te explota en algun otro lado y para todo \n");
		return NULL;
	}

	//Veo si es Vacio
	if (string_equals_ignore_case((char*) rutaCompleta , "")) {
		return string_duplicate("");
	}

	//Veo si es "/" Solito
	if (string_equals_ignore_case((char*) rutaCompleta , "/")) {
		return string_duplicate("/");
	}

	//Veo si no tiene ningun "/", entonces devuelvo Vacio, ""
	if (strrchr(rutaCompleta , '/') == NULL) {
		return string_duplicate("");
	}

	//Obtengo la longitud desde el inicio hasta el ultimo "/"
	int longitud = strrchr(rutaCompleta , '/') - rutaCompleta;

	//Veo Si el Unico "/" que hay es al principio
	if(longitud==0){
		return string_duplicate("/");
	}

	//Tomo toda la Ruta hasta el ultimo "/" que haya, lo que le sigue al "/" final "se descarta" y no lo devuelve
	char* stringPorDevolver = string_substring_until((char*) rutaCompleta , longitud );
	return stringPorDevolver;
}

void Comun_LiberarMemoria(void** punteroMemoria) {

	//Si no ha sido Utilizada la Memoria, no la Libero
	if (*punteroMemoria != NULL) {
		//Libero la Memoria
		free(*punteroMemoria);
		//Pongo el Puntero a NULL, para que Cualquier Referencia Futura a la Memoria Genere Error. Asi no puede referenciarse a Memoria Basura
		//Esto tambien evita que se libere 2 veces la memoria
		*punteroMemoria = NULL;
	}

	return;
}

void Comun_LiberarMemoriaDobleArray(void ***punteroMemoria, int sizeArray) {

	int posActual = 0;
	for (posActual = 0; posActual < sizeArray; posActual++) {
			free(*punteroMemoria[posActual]);
			*punteroMemoria[posActual] = NULL;
	}
	free(*punteroMemoria);
	*punteroMemoria = NULL;
	return;
}


void Comun_Pantalla_Separador_Destacar(const char* texto, ... ) {
	//Necesito juntar en un Unico String todo los char que se le pasen
	char* textoJuntado;

	//Utilizo una Lista de Argumentos Variables y Una funcion de las Commons para juntar los Strings
	va_list listaArgumentosVariables;
	va_start(listaArgumentosVariables, texto);

	textoJuntado = string_from_vformat(texto, listaArgumentosVariables);

	printf( "\n\n" ANSI_COLOR_BLUE "[========== %s ==========]" ANSI_COLOR_RESET "\n\n", textoJuntado );
	free(textoJuntado);
}

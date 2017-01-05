#include <stdbool.h>	//Para Tipos True y False
#include <sys/mman.h>	//Para MMap
#include <string.h>		//Para StrError
#include <errno.h>		//Para ErrNo
#include <stdio.h>		//Para los Printf
#include <stdlib.h>		//Para Abort
#include <pthread.h>	//Para Mutex


#include "../lib/commons/string.h"			//Para Funciones de Strings
#include "../lib/commons/temporal.h"		//Para Obtener Hora


#include "comun.h"	//Para Macros de Errores y Colores
#include "logs.h"



//Quiero que Cuando se ejecute con NODEBUG definido, se vallan los Mensajes de Trace, que son la Mayoria de los mensajes que llenan los Logs y no sirven para nada.
#ifdef NODEBUG
	#define NIVEL_DE_LOGUEO 	LOG_LEVEL_DEBUG
#else
	#define NIVEL_DE_LOGUEO 	LOG_LEVEL_TRACE
#endif


#define ANSI_COLOR_NEGRITA 	"\x1b[1m"


//Inicializa el Mutex de forma que pueda accederse entre thread,s procesos y procesos forkeados
static void INTERNA_iniciarlizarMutex();

//Se encarga del Manejo de Mutex para Loguear de a 1 por vez
static void INTERNA_loguearMensaje(const char* archivoLoguear , const char* textoPorLoguear);

//Se encarga de abrir, escribir y cerrar el archivo
static void INTERNA_escribirArchivo(const char* archivoLoguear , const char* textoPorLoguear);

//Me sirve para darle formato para HTMl segun el nivel de log
static char* INTERNA_obtenerTextoNivelLog(t_log_level nivel);




//Variables Globales Internas
pthread_mutex_t* INTERNA_MutexLogs = NULL;
bool INTERNA_mutexInicializado = false;






static char* INTERNA_obtenerTextoNivelLog(t_log_level nivel){
	switch(nivel){
		case LOG_LEVEL_TRACE:  		return "TRACE ";
		case LOG_LEVEL_DEBUG:  		return "DEBUG";
		case LOG_LEVEL_INFO:  		return " INFO ";
		case LOG_LEVEL_WARNING:  	return " WARN";
		case LOG_LEVEL_ERROR:  		return "ERROR";
		default:					return "MAL el Nivel de Log";
	}
}




void INTERNA_iniciarlizarMutex() {
	int numeroError = 0;		//Por Defecto sin Errores

	//Para que no se inicialize 2 veces. Es a prueba de Boludos
	if (INTERNA_mutexInicializado == false) {
		int tipoDeProteccion = PROT_READ | PROT_WRITE;		//Le digo que esta permitido Leer y escribir
		int flagsDeMemoria = MAP_SHARED | MAP_ANONYMOUS;     //Le Digo que Comparta la Memoria entre Threads y Procesos y que ignore los 2 ultimos parametros del MMap porque son para archivos

		//Se utiliza MMap y no un Shared Mutex o un Semaforo Binario, porque PTHREAD No asegura que funcione bien con Fork
		INTERNA_MutexLogs = mmap(NULL , sizeof(pthread_mutex_t) , tipoDeProteccion , flagsDeMemoria , -1 , 0);     //El -1 y 0 Finales son por un tema de portabilidad
		Macro_Check_And_Handle_Error((int )INTERNA_MutexLogs == -1 , "Fallo en el MMap al inicializar la Memoria Compartida entre Procesos");

		//Inicializamos el Mutex con los Valores por Defecto
		pthread_mutexattr_t attr;
		numeroError = pthread_mutexattr_init(&attr);
		Macro_Check_And_Handle_Error(numeroError != 0 , "Fallo en pthread_mutexattr_init");

		numeroError = pthread_mutexattr_setpshared(&attr , PTHREAD_PROCESS_SHARED);
		Macro_Check_And_Handle_Error(numeroError != 0 , "Fallo en pthread_mutexattr_setpshared");

		numeroError = pthread_mutex_init(INTERNA_MutexLogs , &attr);
		Macro_Check_And_Handle_Error(numeroError != 0 , "Fallo en pthread_mutex_init");

		//Para que no se inicialize 2 veces
		INTERNA_mutexInicializado = true;

	} else {
		printf(ANSI_COLOR_YELLOW "[WARNING] Estas intentado Re-Inicializar el Mutex de Logs.YA fue Inicializado" ANSI_COLOR_RESET "\n");
	}

	//Listo, volvemos
	return;


Error_Handler:
	printf(ANSI_COLOR_RED "[ERROR FATAL] No se pudo Inicializar el Mutex de Logs" ANSI_COLOR_RESET "\n");

	abort();     //Para Todo
	//Nota: Utilizo "ABORT" para frenar la ejecucion del programa, ademas permite que GDB lo atrape en Debug y vea el estado del programa.
}




void LOG_escribirSeparador(const char* archivoLoguear){
	// Comenado por Julian 04-11-2015
	//INTERNA_loguearMensaje(archivoLoguear, "<br><br><br>\n\n");
}



void LOG_escribir(const char* archivoLoguear , const char* nombreTareaFuncion , t_log_level nivelDeLogueo , const char* textoPorLoguear , ...) {
	char* textoJuntado = NULL;					//Necesito juntar en un Unico String todo los char que se le pasen, sino no puedo loguear
	char* inicioColor = NULL;					//Para darle color y formato en HTML
	char* finColor = NULL;
	char* textoCompleto = NULL;					//Donde quedara el String Completo, preparado para Loguear
	char* bufferIntermedio = NULL;				//Para Darle el Formato Deseado
	char* hora = NULL;
	unsigned int threadID = 0;

	//Comparo con el Nivel de Logueo Pedido contra el Establecido
	if (nivelDeLogueo < NIVEL_DE_LOGUEO) {
		return;     //No hago nada porque el Nivel de Logueo Establecido Ignora los demas
	}

	//Utilizo una Lista de Argumentos Variables y Una funcion de las Commons para juntar los Strings
	va_list listaArgumentosVariables;
	va_start(listaArgumentosVariables , textoPorLoguear);
	textoJuntado = string_from_vformat(textoPorLoguear , listaArgumentosVariables);

	//Veo con que nivel debo Imprimir
	switch (nivelDeLogueo) {
		case LOG_LEVEL_TRACE:
			inicioColor = string_duplicate("");
			finColor = string_duplicate("");
			break;

		case LOG_LEVEL_DEBUG:
			inicioColor = string_duplicate("");
			finColor = string_duplicate("");
			//inicioColor = string_duplicate("<b>");
			//finColor = string_duplicate("</b>");
			break;

		case LOG_LEVEL_INFO:
			inicioColor = string_duplicate("<B>");
			finColor = string_duplicate("");
			//inicioColor = string_duplicate("<b><font color=\"blue\">");
			//finColor = string_duplicate("</b> </font>");
			break;

		case LOG_LEVEL_WARNING:
			inicioColor = string_duplicate("<Y>");
			finColor = string_duplicate("");
			//inicioColor = string_duplicate("<b><font color=\"#996600\">");
			//finColor = string_duplicate("</b> </font>");
			break;

		case LOG_LEVEL_ERROR:
			inicioColor = string_duplicate("<R>");
			finColor = string_duplicate("");
			//inicioColor = string_duplicate("<b><font color=\"red\">");
			//finColor = string_duplicate("</b> </font>");
			break;

		default:
			printf(ANSI_COLOR_YELLOW "[WARNING] Me mandaste a imprimir un nivel de Logueo que no existe, revisa los Argumentos" ANSI_COLOR_RESET "\n");
			return;		//No Loguea Nada
	}

	//Lleno el Buffer Intermedio
	hora = temporal_get_string_time();
	threadID = process_get_thread_id();
	bufferIntermedio = string_from_format("%s [%s] %s (%d::%d) %s", hora, INTERNA_obtenerTextoNivelLog(nivelDeLogueo), nombreTareaFuncion, process_getpid(), threadID, textoJuntado);

	//Juntamos los Chars
	textoCompleto = string_new();
	string_append(&textoCompleto , inicioColor);
	string_append(&textoCompleto , bufferIntermedio);
	string_append(&textoCompleto , finColor);
	//string_append(&textoCompleto , "<br>");				//Salto de Linea en HTML

	//Ahora que arme el Texto, lo mando a loguear
	INTERNA_loguearMensaje(archivoLoguear , textoCompleto);

	//Libero la Memoria del va_list
	va_end(listaArgumentosVariables);

	//Libero la Memoria de los Char* Dinamicos. Estoy usando Free A proposito porque NO debe explotar nunca, por lo tanto quiero que explote si llegara a pasar algo raro.
	free(textoJuntado);     free(inicioColor);			free(finColor);
	free(hora);				free(bufferIntermedio);		free(textoCompleto);
	//TODO Mas faltan el de hora, bufferIntermedio, textoCompleto...

	return;
}



void LOG_escribirPantalla(const char* archivoLoguear, const char* nombreTareaFuncion, t_log_level nivelDeLogueo, const char* textoPorLoguear, ... ){
	char* textoJuntado = NULL;

	//Comparo con el Nivel de Logueo Pedido contra el Establecido
	if (nivelDeLogueo < NIVEL_DE_LOGUEO) {
		return;     //No hago nada porque el Nivel de Logueo Establecido Ignora los demas
	}


	//Utilizo una Lista de Argumentos Variables y Una funcion de las Commons para juntar los Strings
	va_list listaArgumentosVariables;
	va_start(listaArgumentosVariables , textoPorLoguear);
	textoJuntado = string_from_vformat(textoPorLoguear , listaArgumentosVariables);


	switch(nivelDeLogueo){
		case LOG_LEVEL_TRACE:
			printf("[%s] %s\n", log_level_as_string(nivelDeLogueo), textoJuntado);
			break;

		case LOG_LEVEL_DEBUG:
			printf(ANSI_COLOR_NEGRITA "[%s] %s" ANSI_COLOR_RESET "\n", log_level_as_string(nivelDeLogueo), textoJuntado);
			break;

		case LOG_LEVEL_INFO:
			printf( ANSI_COLOR_BLUE "[%s] %s" ANSI_COLOR_RESET "\n", log_level_as_string(nivelDeLogueo), textoJuntado);
			break;

		case LOG_LEVEL_WARNING:
			printf(ANSI_COLOR_YELLOW "[%s] %s" ANSI_COLOR_RESET "\n", log_level_as_string(nivelDeLogueo), textoJuntado);
			break;

		case LOG_LEVEL_ERROR:
			printf(ANSI_COLOR_RED "[%s] %s" ANSI_COLOR_RESET "\n", log_level_as_string(nivelDeLogueo), textoJuntado);
			break;
	}

	//Llamo al Loguear sin Imprimir Pantalla
	LOG_escribir(archivoLoguear, nombreTareaFuncion, nivelDeLogueo, textoJuntado);

	//Libero la Memoria del va_list
	va_end(listaArgumentosVariables);
}



static void INTERNA_loguearMensaje(const char* archivoLoguear , const char* textoPorLoguear) {
	int numeroError = 0;     //Por Defecto sin Errores

	//Revisamos si el Mutex NO esta Inicializado
	if (INTERNA_mutexInicializado == false) {
		INTERNA_iniciarlizarMutex();
	}


	//Inicio Mutex
	numeroError = pthread_mutex_lock(INTERNA_MutexLogs);
	Macro_Check_And_Handle_Error(numeroError != 0 , "Fallo en pthread_mutex_lock");

	INTERNA_escribirArchivo(archivoLoguear , textoPorLoguear);

	//Fin Mutex
	numeroError = pthread_mutex_unlock(INTERNA_MutexLogs);
	Macro_Check_And_Handle_Error(numeroError != 0 , "Fallo en pthread_mutex_unlock");

	return;


Error_Handler:
	printf(ANSI_COLOR_YELLOW "[ERROR en LOGS] No se pudo Loguear en el Archivo:'%s'. Error con los Mutex" ANSI_COLOR_RESET "\n" , archivoLoguear);
	return;		//Para hacer que pare y no siga

}




static void INTERNA_escribirArchivo(const char* archivoLoguear , const char* textoPorLoguear) {
	int numeroError = 0;     //Por Defecto sin Errores
	FILE* punteroArchivo = NULL;

	//Abro el archivo
	punteroArchivo = fopen(archivoLoguear , "a");
	Macro_Check_And_Handle_Error(punteroArchivo==NULL , "Fallo en fopen");

	//Escribo en el Archivo y Fuerzo su escritura
	numeroError = fprintf(punteroArchivo , "%s\n" , textoPorLoguear);
	Macro_Check_And_Handle_Error(numeroError < 0 , "Fallo en fprintf");
	numeroError = fflush(punteroArchivo);
	Macro_Check_And_Handle_Error(numeroError != 0 , "Fallo en fflush");

	//Cierro el Archivo
	numeroError = fclose(punteroArchivo);
	punteroArchivo = NULL;
	Macro_Check_And_Handle_Error(numeroError != 0 , "Fallo en fclose");
	return;


Error_Handler:
	printf(ANSI_COLOR_YELLOW "[ERROR en LOGS] No se pudo Loguear en el Archivo:'%s'. Error con Escribir en el Archivo" ANSI_COLOR_RESET "\n" , archivoLoguear);

	//Cierro el archivo si esta abierto
	if (punteroArchivo != NULL) {
		numeroError = fclose(punteroArchivo);
		if (numeroError != 0) {
			printf(ANSI_COLOR_RED "No se pudo cerrar el archivo" ANSI_COLOR_RESET "\n");
		}
	}

	return;		//Para hacer que pare y no siga
}

#include "planificador.h"

#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <libgen.h>
#include <unistd.h>


#include "../bibliotecas/logs.h"
#include "../bibliotecas/inicializadores.h"


#include "../lib/commons/string.h"
#include "../lib/commons/collections/list.h"
#include "../lib/commons/config.h"


#include "../lib/readline/config.h"
#include "../lib/readline/readline.h"
#include "../lib/readline/history.h"

extern HIST_ENTRY **history_list();


#define ARCHIVO_LOGS "LogPlanificador.html"
#define ARCHIVO_LOG_CPU "LogEjecucion.html"
#define ARCHIVO_CONFIG "ConfigPlanificador.cfg"
#define INICIO "Inicio"
#define MAXIMA_CANTIDAD_CARACTERES_POR_LEER 1024		//Solo se aceptan Rutas de hasta 1024 Caracteres.
#define INFINITO 1
#define MAXIMA_RUTA_ABSOLUTA 1024		//Bien Largo, asi no tenemos problemas


//#undef Macro_ImprimirParaDebug
//#undef Macro_ImprimirParaDebugConDatos
//#define Macro_ImprimirParaDebug(Mensaje, ...)              rl_save_prompt(); rl_message(Mensaje, ##__VA_ARGS__); rl_restore_prompt();
//#define Macro_ImprimirParaDebugConDatos(Mensaje, ...)      rl_save_prompt(); rl_message("[DEBUG] (%s:%s:%d): " Mensaje "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__); rl_restore_prompt();

//========== Macros para no Ensuciar el Codigo con Locks/Unlocks =============
//IMPORTANTE: Que NO haya un Return entre medio del Lock de los Mutex

#define Macro_Mutex_Lock(Mutex,NombreMutex)						if(  pthread_mutex_lock(&Mutex)!=0 )	{ LOG_escribirDebugPantalla(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_ERROR, "[%s:%s:%s] Fallo al Hacer Lock al Mutex '"   NombreMutex "'. ERRNO:%s", __FILE__, __func__, __LINE__, Macro_Obtener_Errno()) }
#define Macro_Mutex_UnLock(Mutex,NombreMutex)					if(  pthread_mutex_unlock(&Mutex)!=0 )	{ LOG_escribirDebugPantalla(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_ERROR, "[%s:%s:%s] Fallo al Hacer UnLock al Mutex '" NombreMutex "'. ERRNO:%s", __FILE__, __func__, __LINE__, Macro_Obtener_Errno()) }

//#define Macro_Semaforo_Wait(Semaforo,NombreSemaforo)			{ LOG_escribirDebugPantalla(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_TRACE, "Haciendo WAIT del Semaforo: "   NombreSemaforo); if( sem_wait(&Semaforo)!=0 )	{ LOG_escribirDebugPantalla(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_ERROR, "[%s:%s:%s] Fallo al Hacer Wait al Semaforo '" NombreSemaforo "'. ERRNO:%s", __FILE__, __func__, __LINE__, Macro_Obtener_Errno()) } }
//#define Macro_Semaforo_Signal(Semaforo,NombreSemaforo)		{ LOG_escribirDebugPantalla(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_TRACE, "Haciendo SIGNAl del Semaforo: " NombreSemaforo); if( sem_post(&Semaforo)!=0 )	{ LOG_escribirDebugPantalla(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_ERROR, "[%s:%s:%s] Fallo al Hacer Post al Semaforo '" NombreSemaforo "'. ERRNO:%s", __FILE__, __func__, __LINE__, Macro_Obtener_Errno()) } }
#define Macro_Semaforo_Wait(Semaforo,NombreSemaforo)			if( sem_wait(&Semaforo)!=0 )	{ LOG_escribirDebugPantalla(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_ERROR, "[%s:%s:%s] Fallo al Hacer Wait al Semaforo '" NombreSemaforo "'. ERRNO:%s", __FILE__, __func__, __LINE__, Macro_Obtener_Errno()) }
#define Macro_Semaforo_Signal(Semaforo,NombreSemaforo)			if( sem_post(&Semaforo)!=0 )	{ LOG_escribirDebugPantalla(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_ERROR, "[%s:%s:%s] Fallo al Hacer Post al Semaforo '" NombreSemaforo "'. ERRNO:%s", __FILE__, __func__, __LINE__, Macro_Obtener_Errno()) }
//=============================================================================


typedef struct {
	char	puertoDeEscucha[LONGITUD_CHAR_PUERTOS];
	char	algoritmoPlanificacion[10];		//Con 10 deberia Alcanzar
	int		tiempoDeQuantum;
	int		tamanioRafagaFifo;
} tipo_valores_Archivo_Config;




//Recursos Globales
uint32_t					GLOBAL_Numero_ID_mPro;
t_list* 					GLOBAL_lista_mPro = NULL;
t_list* 					GLOBAL_lista_CPUs = NULL;
pthread_mutex_t 			GLOBAL_Mutex_lista_mPro;
pthread_mutex_t 			GLOBAL_Mutex_lista_CPUs;
sem_t 						GLOBAL_Semaforo_mPro_listos;
sem_t 						GLOBAL_Semaforo_CPU_libres;
tipo_valores_Archivo_Config	GLOBAL_valoresArchivoConfig;
tipo_AdminCPU				GLOBAL_adminCPU;
//Para Rutas Absolutas de Archivos Logs:
char*						GLOBAL_ARCHIVO_LOGS;
char*						GLOBAL_ARCHIVO_LOGS_CPU;


int main(){
	int iNumeroError = 0;		//Variable para Controlar Errores, por defecto es 0 que significa que no hay error


	Macro_LimpiarPantalla();
	Comun_Pantalla_Separador_Destacar("Iniciando Planificador");

	//Verifico si existe el archivo de Configuracion
	Macro_ImprimirEstadoInicio("Viendo Si existe El archivo de Configuracion");
	iNumeroError = Comun_existeArchivo(ARCHIVO_CONFIG);
	if (iNumeroError < 0) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf( ANSI_COLOR_RED "El Archivo Config o No existe o No se tienen permisos de Lectura/Escritura/Ejecucion." ANSI_COLOR_RESET "\n");
		return EJECUCION_FALLIDA;
	}
	Macro_ImprimirEstadoFinal(EJECUCION_CORRECTA);


	if (estaValidoArchifoConfig() == false) {
		return EJECUCION_FALLIDA;
	}

	cargarValoresArchivoConfig();

	//Inicializo Ruta Absoluta de Logs

	if( estaEstablecidaRutasAbsoluta()==false ){
		return EJECUCION_FALLIDA;
	}


	Comun_borrarArchivo(GLOBAL_ARCHIVO_LOGS);
	Comun_borrarArchivo(GLOBAL_ARCHIVO_LOGS_CPU);
	LOG_escribir(GLOBAL_ARCHIVO_LOGS, INICIO, LOG_LEVEL_DEBUG, "Iniciando...");


	if (estanInicializadasEstructurasNecesarias() == false) {
		return EJECUCION_FALLIDA;
	}


	if ( estanCreadosModulos() == false){
		return EJECUCION_FALLIDA;
	}


	Comun_Pantalla_Separador_Destacar("Planificador Iniciado");
	LOG_escribir(GLOBAL_ARCHIVO_LOGS, INICIO, LOG_LEVEL_DEBUG, "Inicio Completo. Mostrando consola");
	LOG_escribirSeparador(GLOBAL_ARCHIVO_LOGS);

	//Abro la Consola, Solo sale del Ciclo cuando se llama al Comando Salir
	while ( leerConsola() ) {
		//No hace Falta Hacer nada
	}


	return EJECUCION_CORRECTA;
}





bool estaValidoArchifoConfig(){
	Macro_ImprimirEstadoInicio("Verificando Archivo de Configuracion");

	//Abro Archivo de Configuracion
	t_config* archivoConfig = config_create(ARCHIVO_CONFIG);

	//Verifico que tenga los Campos/Claves que necesitamos
	if (config_has_property(archivoConfig , "PUERTO_ESCUCHA") == false) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf(ANSI_COLOR_RED "El archivo Config No tiene el Campo 'PUERTO_ESCUCHA'" ANSI_COLOR_RESET "\n");
		return false;
	}

	if (config_has_property(archivoConfig , "ALGORITMO_PLANIFICACION") == false) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf(ANSI_COLOR_RED "El archivo Config No tiene el Campo 'ALGORITMO_PLANIFICACION'" ANSI_COLOR_RESET "\n");
		return false;
	}

	if (config_has_property(archivoConfig , "QUANTUM") == false) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf(ANSI_COLOR_RED "El archivo Config No tiene el Campo 'QUANTUM'" ANSI_COLOR_RESET "\n");
		return false;
	}

	if (config_has_property(archivoConfig , "RAFAGA_FIFO") == false) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf(ANSI_COLOR_RED "El archivo Config No tiene el Campo Extra 'RAFAGA_FIFO'" ANSI_COLOR_RESET "\n");
		return false;
	}

	config_destroy(archivoConfig);

	//Si llega aca abajo es que esta OK el archivo Config
	Macro_ImprimirEstadoFinal(EJECUCION_CORRECTA);
	return true;
}



bool estanInicializadasEstructurasNecesarias(){
	int iNumeroError = 0;		//Variable para Controlar Errores, por defecto es 0 que significa que no hay error
	Macro_ImprimirEstadoInicio("Inicializando Estructuras Necesarias");

	GLOBAL_Numero_ID_mPro = 1;		//Empiezan en 1

	GLOBAL_lista_mPro = list_create();
	if( GLOBAL_lista_mPro==NULL ){
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf(ANSI_COLOR_RED "Fallo al Inicializar Lista de mPro. Problema de las Commons o de la Memoria" ANSI_COLOR_RESET "\n");
		return false;
	}

	GLOBAL_lista_CPUs = list_create();
	if (GLOBAL_lista_mPro == NULL) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf(ANSI_COLOR_RED "Fallo al Inicializar Lista de CPUs. Problema de las Commons o de la Memoria" ANSI_COLOR_RESET "\n");
		return false;
	}

	iNumeroError = pthread_mutex_init(&GLOBAL_Mutex_lista_mPro, NULL);	//Inicializado con Atributos Por Defecto
	if (iNumeroError != 0) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf(ANSI_COLOR_RED "Fallo al Inicializar Mutex Lista mPro. Problema de Pthread o de la Memoria" ANSI_COLOR_RESET "\n");
		return false;
	}

	iNumeroError = pthread_mutex_init(&GLOBAL_Mutex_lista_CPUs , NULL);     //Inicializado con Atributos Por Defecto
	if (iNumeroError != 0) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf(ANSI_COLOR_RED "Fallo al Inicializar Mutex Lista CPUs. Problema de Pthread o de la Memoria" ANSI_COLOR_RESET "\n");
		return false;
	}


	iNumeroError = sem_init(&GLOBAL_Semaforo_mPro_listos, 0, 0);			//Inicializado para Compartir entre Threads y Empieza en 0
	if (iNumeroError == -1) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf(ANSI_COLOR_RED "Fallo al Inicializar Semaforo mPro Listos. Problema de Pthread o de la Memoria" ANSI_COLOR_RESET "\n");
		return false;
	}


	iNumeroError = sem_init(&GLOBAL_Semaforo_CPU_libres , 0 , 0);			//Inicializado para Compartir entre Threads y Empieza en 0
	if (iNumeroError == -1) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf(ANSI_COLOR_RED "Fallo al Inicializar Semaforo CPU Libres. Problema de Pthread o de la Memoria" ANSI_COLOR_RESET "\n");
		return false;
	}

	//Si llega aca abajo es que se Inicializaron Bien las Estructuras
	Macro_ImprimirEstadoFinal(EJECUCION_CORRECTA);
	return true;
}


bool estanCreadosModulos(){
	pthread_t NumeroErrorThreads = 0;		//Variable para Controlar Errores, por defecto es 0 que significa que no hay error
	Macro_ImprimirEstadoInicio("Creando Threads de Modulos Independientes");

	NumeroErrorThreads = iniciarThread((void*) &ThreadBloqueos , NULL);
	if (NumeroErrorThreads == -1) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf(ANSI_COLOR_RED "Fallo al Crear Thread de Bloqueos. Problema de Pthread o de la Memoria" ANSI_COLOR_RESET "\n");
		return false;
	}


	//Veo Archivo Config cual Planificador Creo
	if ( string_equals_ignore_case(GLOBAL_valoresArchivoConfig.algoritmoPlanificacion, "Fifo") ){
		NumeroErrorThreads = iniciarThread((void*) &ThreadPlanificador_FIFO , NULL);

	}else if( string_equals_ignore_case(GLOBAL_valoresArchivoConfig.algoritmoPlanificacion, "RR") || string_equals_ignore_case(GLOBAL_valoresArchivoConfig.algoritmoPlanificacion, "RoundRobin") ){
		NumeroErrorThreads = iniciarThread((void*) &ThreadPlanificador_RoundRobin , NULL);
	}else{
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf(ANSI_COLOR_RED "Fallo al Crear Thread del Planificador. Esta Mal Escrito el Algoritmo. Valores Aceptables: Fifo|RR|RoundRobin" ANSI_COLOR_RESET "\n");
		return false;
	}

	//El control de Errores es igual para ambos algoritmos
	if (NumeroErrorThreads == -1) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf(ANSI_COLOR_RED "Fallo al Crear Thread del Planificador. Problema de Pthread o de la Memoria" ANSI_COLOR_RESET "\n");
		return false;
	}

	NumeroErrorThreads = iniciarThread((void*) &ThreadServidor , NULL);
	if (NumeroErrorThreads == -1) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf(ANSI_COLOR_RED "Fallo al Crear Thread del Servidor. Problema de Pthread o de la Memoria" ANSI_COLOR_RESET "\n");
		return false;
	}







	//Si llega aca abajo es que se Crearon bien los Threads
	sleep(1);		//Para que lleguen a Crearse Correctamente los Threads y no salga Feo el Log
	Macro_ImprimirEstadoFinal(EJECUCION_CORRECTA);
	return true;
}


void cargarValoresArchivoConfig(){
	Macro_ImprimirEstadoInicio("Cargando Valores Archivo Config");

	//Abro Archivo de Configuracion
	t_config* archivoConfig = config_create(ARCHIVO_CONFIG);

	//Cargo Valores en Memoria
	strcpy( GLOBAL_valoresArchivoConfig.puertoDeEscucha, config_get_string_value(archivoConfig, "PUERTO_ESCUCHA") );
	strcpy( GLOBAL_valoresArchivoConfig.algoritmoPlanificacion, config_get_string_value(archivoConfig, "ALGORITMO_PLANIFICACION") );
	GLOBAL_valoresArchivoConfig.tiempoDeQuantum = config_get_int_value(archivoConfig, "QUANTUM");
	GLOBAL_valoresArchivoConfig.tamanioRafagaFifo = config_get_int_value(archivoConfig, "RAFAGA_FIFO");

	config_destroy(archivoConfig);

	Macro_ImprimirEstadoFinal(EJECUCION_CORRECTA);
	//Macro_ImprimirParaDebug("Puerto:%s Algoritmo:%s Quantum:%d\n", GLOBAL_valoresArchivoConfig.puertoDeEscucha, GLOBAL_valoresArchivoConfig.algoritmoPlanificacion, GLOBAL_valoresArchivoConfig.tiempoDeQuantum);
	return;
}


bool estaEstablecidaRutasAbsoluta(){
	char* directorioActual = NULL;

	Macro_ImprimirEstadoInicio("Estableciendo Ruta Absoluta para Archivos de Logs");
	directorioActual = getcwd(NULL , MAXIMA_RUTA_ABSOLUTA);

	if (directorioActual == NULL) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf( ANSI_COLOR_RED "Problema al establecer Ruta Absoluta.Error Detectado:%s" ANSI_COLOR_RESET "\n" , Macro_Obtener_Errno());
		return false;
	}
	Macro_ImprimirEstadoFinal(EJECUCION_CORRECTA);

	//Ahora Junto Todo
	GLOBAL_ARCHIVO_LOGS = string_new();
	GLOBAL_ARCHIVO_LOGS_CPU = string_new();

	string_append(&GLOBAL_ARCHIVO_LOGS, directorioActual);
	string_append(&GLOBAL_ARCHIVO_LOGS_CPU, directorioActual);

	string_append(&GLOBAL_ARCHIVO_LOGS, "/");
	string_append(&GLOBAL_ARCHIVO_LOGS_CPU, "/");


	string_append(&GLOBAL_ARCHIVO_LOGS, ARCHIVO_LOGS);
	string_append(&GLOBAL_ARCHIVO_LOGS_CPU, ARCHIVO_LOG_CPU);
	Macro_ImprimirParaDebug("Ruta Absoluta Logs Planificador:'%s'\n", GLOBAL_ARCHIVO_LOGS);
	Macro_ImprimirParaDebug("Ruta Absoluta Logs Ejecucion:'%s'\n", GLOBAL_ARCHIVO_LOGS_CPU);

	return true;
}

long buscarAtrasPuntoComa(FILE* archivoAbierto, long posicionActual){
	int 	iNumeroError = 0;		//Variable para Controlar Errores, por defecto es 0 que significa que no hay error
	int 	charLeido = 0;

	do {
		posicionActual--;

		iNumeroError = fseek(archivoAbierto , posicionActual , SEEK_SET);
		Macro_Check_And_Handle_Error(iNumeroError == -1 , "Fallo fseek");

		charLeido = fgetc(archivoAbierto);
		Macro_Check_And_Handle_Error(charLeido == EOF, "Fallo fgetc");
	} while (charLeido != ';');

	return posicionActual;

Error_Handler:
	printf(ANSI_COLOR_RED "Fallo Fseek o FgetC al aplicarla al Archivo" ANSI_COLOR_RESET "\n");

	return -1;
}


long obtenerPosicioninstruccionFinalizar(const char* rutaLinuxArchivo){
	int 	iNumeroError = 0;		//Variable para Controlar Errores, por defecto es 0 que significa que no hay error
	FILE*	archivoAbierto = NULL;
	long	posicionInstruccionFinalizar = 0;
	int 	charLeido = 0;

	archivoAbierto = fopen( rutaLinuxArchivo, "r");
	Macro_Check_And_Handle_Error(archivoAbierto == NULL, "Fallo Fopen");

	posicionInstruccionFinalizar = Comun_obtener_tamanio_archivo(rutaLinuxArchivo);		//OJO, DownCasting
	Macro_Check_And_Handle_Error(posicionInstruccionFinalizar == -1, "Fallo Comun_obtener_tamanio_archivo");

	posicionInstruccionFinalizar--;
	LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_TRACE, "posicionInstruccionFinalizar: '%d'", posicionInstruccionFinalizar);

	iNumeroError = fseek(archivoAbierto, posicionInstruccionFinalizar, SEEK_SET);
	Macro_Check_And_Handle_Error(iNumeroError == -1, "Fallo fseek");

	charLeido = fgetc(archivoAbierto);
	Macro_Check_And_Handle_Error(charLeido == EOF, "Fallo fgetc");

	//En caso que al final del archivo hallan espacios o un enter se lo acepto y recorro para atras hasta el primer PuntoComa
	if (charLeido != ';') {
		posicionInstruccionFinalizar = buscarAtrasPuntoComa( archivoAbierto, posicionInstruccionFinalizar );
		Macro_Check_And_Handle_Error(posicionInstruccionFinalizar == -1, "Fallo buscarAtrasPuntoComa");
	}

	//Como ya estoy posicionado en el PuntoComa de la Instruccion Finalizar, busco el PuntoComa de Instruccion Anterior
	posicionInstruccionFinalizar = buscarAtrasPuntoComa( archivoAbierto, posicionInstruccionFinalizar );
	Macro_Check_And_Handle_Error(posicionInstruccionFinalizar == -1, "Fallo buscarAtrasPuntoComa");

	iNumeroError =  fclose(archivoAbierto);
	archivoAbierto = NULL;
	Macro_Check_And_Handle_Error(iNumeroError == -1, "Fallo fclose");

	posicionInstruccionFinalizar++;		//Para posicionarme Justo al Inicio de la Instruccion (A pedido de Lucas)
	return posicionInstruccionFinalizar;

Error_Handler:

	if (archivoAbierto != NULL) {
		fclose(archivoAbierto);
	}

	return -1;
}



void loguearResultadoEjecucion(t_list* listaResultadosInstrucciones, char* nombre_mPro){
	int 		cantidadElementos = 0;
	int 		elementoActual = 0;
	char*		resultadoInstruccion = NULL;

	cantidadElementos = list_size(listaResultadosInstrucciones);

	LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_TRACE, "Cantidad de Instrucciones Ejecutadas: '%d'", cantidadElementos);

	for(elementoActual = 0;elementoActual <cantidadElementos; elementoActual++){
		resultadoInstruccion = list_get(listaResultadosInstrucciones, elementoActual);

		LOG_escribir(GLOBAL_ARCHIVO_LOGS_CPU, nombre_mPro, LOG_LEVEL_DEBUG, resultadoInstruccion);
	}
	LOG_escribirSeparador(GLOBAL_ARCHIVO_LOGS_CPU);

	return;
}


tipo_CPU* buscarCPU(uint32_t numeroCPU) {
	tipo_CPU*	CpuPorDevovler = NULL;
	int cantidadElementos = 0;
	int elementoActual = 0;

	cantidadElementos = list_size(GLOBAL_lista_CPUs);

	for (elementoActual = 0; elementoActual < cantidadElementos; elementoActual++) {
		CpuPorDevovler = list_get(GLOBAL_lista_CPUs, elementoActual);

		if(CpuPorDevovler->numeroCPU == numeroCPU){
			break;	//Encontre el que buscaba
		}
	}

	return CpuPorDevovler;
}


tipo_mPro* buscarMPro(uint32_t pid) {
	tipo_mPro* mProPorDevovler = NULL;
	int cantidadElementos = 0;
	int elementoActual = 0;

	cantidadElementos = list_size(GLOBAL_lista_mPro);

	for (elementoActual = 0; elementoActual < cantidadElementos; elementoActual++) {
		mProPorDevovler = list_get(GLOBAL_lista_mPro , elementoActual);

		if (mProPorDevovler->PCB.id == pid) {
			break;     //Encontre el que buscaba
		}
	}

	return mProPorDevovler;
}


char* generarEstadoError(const char* razon) {
	return string_from_format("%s %s", ERROR, (char*) razon);
}

void desconectarCPU(tipo_CPU* cpuPorDesconectar){
	Macro_Mutex_Lock(GLOBAL_Mutex_lista_CPUs , "GLOBAL_Mutex_lista_CPUs");
	cpuPorDesconectar->estado = DESCONECTADO;
	Macro_Mutex_UnLock(GLOBAL_Mutex_lista_CPUs , "GLOBAL_Mutex_lista_CPUs");
	LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_DEBUG, "El CPU Numero '%d' ha sido Desconectado", cpuPorDesconectar->numeroCPU);

	return;
}

char* obtenerNombreEstadoCPU(tipo_CPU_estado estado){
	switch(estado){
		case LIBRE: 		return "Libre";
		case OCUPADO: 		return "Ocupado";
		case DESCONECTADO: 	return "Desconectado";
		default: 			return "MAL. Cualquier Cosa de Estado";
	}
}

int conectarReintentando(struct __t_socket* socketPorConectarse, const char* serverIP, const char* serverPort){
	int iNumeroError = 0;		//Variable para Controlar Errores

	iNumeroError = socketPorConectarse->Conectar(socketPorConectarse , serverIP , serverPort);
	if (iNumeroError <= 0) {
		//Espero un tiempo
		usleep(50000);

		//Reintento
		iNumeroError = socketPorConectarse->Conectar(socketPorConectarse , serverIP , serverPort);
		if (iNumeroError <= 0) {
			//Espero un tiempo mayor al anterior
			sleep(1);
			//Ultimo Reintento, no controlo errores porque directamente devuelvo el resultado
			iNumeroError = socketPorConectarse->Conectar(socketPorConectarse , serverIP , serverPort);
		}
	}

	//Aca devuelve OK (numero mayor a 0) si se termino bien o un Codigo de Error si fallo
	return iNumeroError;
}


void ThreadPlanificador_FIFO(void* valorQueNoUso){
	int 			iNumeroError = 0;		//Variable para Controlar Errores
	int 			cantidadElementosLista = 0;
	int 			elementoActual = 0;
	tipo_mPro* 		mProElegido = NULL;
	tipo_CPU*		CpuElegido = NULL;
	char 			CpuElegido_ip[LONGITUD_CHAR_IP];
	int				CpuElegido_puerto = 0;

	LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_DEBUG,"Inicio del Thread Planificador");

	//Planifica
	while(INFINITO){
		//LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_TRACE,"Antes WAIT CPU Libre");
		Macro_Semaforo_Wait(GLOBAL_Semaforo_CPU_libres, "GLOBAL_Semaforo_CPU_libres");
		//LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_TRACE,"Despues WAIT CPU Libre");
		//LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_TRACE,"Antes WAIT mPro Listo");
		Macro_Semaforo_Wait(GLOBAL_Semaforo_mPro_listos, "GLOBAL_Semaforo_mPro_listos");
		//LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_TRACE,"Despues WAIT mPro Listo");
		LOG_escribirSeparador(GLOBAL_ARCHIVO_LOGS);


		LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_TRACE,"Planificando..");

		//Busco un mPro Listo
		Macro_Mutex_Lock(GLOBAL_Mutex_lista_mPro, "GLOBAL_Mutex_lista_mPro");
		cantidadElementosLista = list_size(GLOBAL_lista_mPro);

		//Busco el mPro en la Lista Global de mPros
		for (elementoActual = 0; elementoActual < cantidadElementosLista; elementoActual++) {
			mProElegido = (tipo_mPro*) list_get(GLOBAL_lista_mPro , elementoActual);
			if ( (string_equals_ignore_case(mProElegido->estado , LISTO) == true) || (string_equals_ignore_case(mProElegido->estado , POR_FINALIZAR) == true) ) {
				break;     //Encontre uno Listo
			}
		}

		mProElegido->PCB.InstruccionesRafaga = GLOBAL_valoresArchivoConfig.tamanioRafagaFifo;
		Macro_Mutex_UnLock(GLOBAL_Mutex_lista_mPro, "GLOBAL_Mutex_lista_mPro");

		if( (string_equals_ignore_case(mProElegido->estado , LISTO) != true) && (string_equals_ignore_case(mProElegido->estado , POR_FINALIZAR) != true) ){
			LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_WARNING, "Se ha elegido a un mPro '%s' para Ejecutar con Estado no Valido. Estado: '%s'", basename(mProElegido->PCB.archivomProc), mProElegido->estado );
		}else{
			LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_TRACE, "Se ha elegido al mPro '%s' (Numero '%d') para Ejecutar. Estado: '%s'", basename(mProElegido->PCB.archivomProc), mProElegido->PCB.id, mProElegido->estado );
		}

		//Busco un CPU Libre para mandar a Ejecutar el mPro
		Macro_Mutex_Lock(GLOBAL_Mutex_lista_CPUs, "GLOBAL_Mutex_lista_CPUs");
		cantidadElementosLista = list_size(GLOBAL_lista_CPUs);

		for (elementoActual = 0; elementoActual < cantidadElementosLista; elementoActual++) {
			CpuElegido = (tipo_CPU*) list_get(GLOBAL_lista_CPUs , elementoActual);
			if (CpuElegido->estado == LIBRE) {
				break;     //Encontre uno Libre
			}
		}

		CpuElegido_puerto = CpuElegido->puerto;

		strcpy(CpuElegido_ip , CpuElegido->ip);
		Macro_Mutex_UnLock(GLOBAL_Mutex_lista_CPUs , "GLOBAL_Mutex_lista_CPUs");


		if( CpuElegido->estado != LIBRE ){
			LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_WARNING, "Se ha elegido al CPU Numero '%d' que tiene Estado Invalido. Estado: '%s'", CpuElegido->numeroCPU, obtenerNombreEstadoCPU(CpuElegido->estado));
		}else{
			LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_TRACE, "Se ha elegido al CPU Numero '%d' para que Ejecute", CpuElegido->numeroCPU);
		}

		SOCKET_crear(socketConectadoCPU);
		iNumeroError = conectarReintentando(socketConectadoCPU , CpuElegido_ip , string_itoa(CpuElegido_puerto));
		if (iNumeroError <= 0) {
			LOG_escribir(GLOBAL_ARCHIVO_LOGS , __func__ , LOG_LEVEL_WARNING , "No me pude Conectar a CPU de IP:'%s' Puerto:'%d'. El mPro '%s' No ha sido Planificado en esta instancia" , CpuElegido_ip , CpuElegido_puerto , basename(mProElegido->PCB.archivomProc));

			desconectarCPU(CpuElegido);
			Macro_Semaforo_Signal(GLOBAL_Semaforo_mPro_listos, "GLOBAL_Semaforo_mPro_listos");
			continue;
		}

		t_order_header headerEnviar;		headerEnviar.sender_id = PLANIFICADOR;		headerEnviar.comando = EJECUTAR_RAFAGA;		//XXX Cambiar por Constructor de Julian si llegara a Crearlo
		iNumeroError = socketConectadoCPU->Enviar(socketConectadoCPU , NULL , headerEnviar , NULL);
		if (iNumeroError <= 0) {
			LOG_escribir(GLOBAL_ARCHIVO_LOGS , __func__ , LOG_LEVEL_WARNING , "No pude Enviar Orden Inicial a CPU de IP:'%s' Puerto:'%d'. El mPro '%s' No ha sido Planificado en esta instancia" , CpuElegido_ip , CpuElegido_puerto , basename(mProElegido->PCB.archivomProc));

			desconectarCPU(CpuElegido);
			Macro_Semaforo_Signal(GLOBAL_Semaforo_mPro_listos, "GLOBAL_Semaforo_mPro_listos");
			continue;
		}


		iNumeroError = socketConectadoCPU->Enviar(socketConectadoCPU , (void*) &mProElegido->PCB , headerEnviar , Serializar_PCB);
		if (iNumeroError <= 0) {
			LOG_escribir(GLOBAL_ARCHIVO_LOGS , __func__ , LOG_LEVEL_WARNING , "No pude Enviar PCB a CPU de IP:'%s' Puerto:'%d'. El mPro '%s' No ha sido Planificado en esta instancia" , CpuElegido_ip , CpuElegido_puerto , basename(mProElegido->PCB.archivomProc));

			desconectarCPU(CpuElegido);
			Macro_Semaforo_Signal(GLOBAL_Semaforo_mPro_listos, "GLOBAL_Semaforo_mPro_listos");
			continue;
		}


		Macro_Mutex_Lock(GLOBAL_Mutex_lista_mPro , "GLOBAL_Mutex_lista_mPro");
		strcpy(mProElegido->estado , EJECUTANDO);
		Metricas_actualizarTiempoEspera(mProElegido);
		Macro_Mutex_UnLock(GLOBAL_Mutex_lista_mPro , "GLOBAL_Mutex_lista_mPro");


		Macro_Mutex_Lock(GLOBAL_Mutex_lista_CPUs , "GLOBAL_Mutex_lista_CPUs");
		CpuElegido->ID_mPro_Ejecutando = mProElegido->PCB.id;
		CpuElegido->estado = OCUPADO;
		Macro_Mutex_UnLock(GLOBAL_Mutex_lista_CPUs , "GLOBAL_Mutex_lista_CPUs");


		LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_INFO, "El mPro '%s' esta Ejecutandose en CPU Numero '%d'", basename(mProElegido->PCB.archivomProc), CpuElegido->numeroCPU );


		iNumeroError = socketConectadoCPU->Desconectar(socketConectadoCPU);
		if (iNumeroError <= 0) {
			LOG_escribir(GLOBAL_ARCHIVO_LOGS , __func__ , LOG_LEVEL_WARNING , "No me pude DesConectar el Socket al CPU de IP:'%s' Puerto:'%d'. Pero el mPro YA HA sido planificado" , CpuElegido_ip , CpuElegido_puerto);
		}

		SOCKET_destruir(socketConectadoCPU);

		LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_TRACE,"Fin de esta instancia de Planificacion");

	}

	LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_WARNING,"WHAT! Esta Llegando al Final del Planificador y No Deberia!!");

	//Nunca Llega Aca
	return;
}


void ThreadPlanificador_RoundRobin(void* valorQueNoUso){
	int 			iNumeroError = 0;		//Variable para Controlar Errores
	int 			cantidadElementosLista = 0;
	int 			elementoActual = 0;
	int 			cpuActual = 0;
	tipo_mPro* 		mProElegido = NULL;
	tipo_CPU*		CpuElegido = NULL;
	char 			CpuElegido_ip[LONGITUD_CHAR_IP];
	int				CpuElegido_puerto = 0;

	LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_DEBUG,"Inicio del Thread Planificador");

	//Planifica
	while(INFINITO){
		//LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_TRACE,"Antes WAIT CPU Libre");
		Macro_Semaforo_Wait(GLOBAL_Semaforo_CPU_libres , "GLOBAL_Semaforo_CPU_libres");
		//LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_TRACE,"Despues WAIT CPU Libre");
		//LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_TRACE,"Antes WAIT mPro Listo");
		Macro_Semaforo_Wait(GLOBAL_Semaforo_mPro_listos , "GLOBAL_Semaforo_mPro_listos");
		//LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_TRACE,"Despues WAIT mPro Listo");
		LOG_escribirSeparador(GLOBAL_ARCHIVO_LOGS);

		LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_TRACE,"Planificando..");

		//Busco un mPro Listo
		Macro_Mutex_Lock(GLOBAL_Mutex_lista_mPro, "GLOBAL_Mutex_lista_mPro");
		cantidadElementosLista = list_size(GLOBAL_lista_mPro);

		//Para que no se pase del tamaño de la lista y vuelva a empezar a llegar al final
		elementoActual = elementoActual % cantidadElementosLista;

		//Busco el mPro en la Lista Global de mPros
		for (; elementoActual < cantidadElementosLista; elementoActual++) {
			mProElegido = (tipo_mPro*) list_get(GLOBAL_lista_mPro , elementoActual);
			if ( (string_equals_ignore_case(mProElegido->estado , LISTO) == true) || (string_equals_ignore_case(mProElegido->estado , POR_FINALIZAR) == true) ) {
				elementoActual++;		//Aumento para Que la proxima Ves Agarre al Proximo (Desde el Actual/Ultimo Disponible)
				break;     //Encontre uno Listo
			}
		}

		mProElegido->PCB.InstruccionesRafaga = GLOBAL_valoresArchivoConfig.tiempoDeQuantum;	//Ojo Upgrade Casting
		Macro_Mutex_UnLock(GLOBAL_Mutex_lista_mPro, "GLOBAL_Mutex_lista_mPro");

		if ((string_equals_ignore_case(mProElegido->estado , LISTO) != true) && (string_equals_ignore_case(mProElegido->estado , POR_FINALIZAR) != true)) {
			LOG_escribir(GLOBAL_ARCHIVO_LOGS , __func__ , LOG_LEVEL_WARNING , "Se ha elegido a un mPro '%s' para Ejecutar con Estado no Valido. Estado: '%s'" , basename(mProElegido->PCB.archivomProc) , mProElegido->estado);
		} else {
			LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_TRACE, "Se ha elegido al mPro '%s' (Numero '%d') para Ejecutar. Estado: '%s'", basename(mProElegido->PCB.archivomProc), mProElegido->PCB.id, mProElegido->estado );
		}

		//Busco un CPU Libre para mandar a Ejecutar el mPro
		Macro_Mutex_Lock(GLOBAL_Mutex_lista_CPUs, "GLOBAL_Mutex_lista_CPUs");
		cantidadElementosLista = list_size(GLOBAL_lista_CPUs);

		for (cpuActual = 0; cpuActual < cantidadElementosLista; cpuActual++) {
			CpuElegido = (tipo_CPU*) list_get(GLOBAL_lista_CPUs , cpuActual);
			if (CpuElegido->estado == LIBRE) {
				break;     //Encontre uno Libre
			}
		}

		CpuElegido_puerto = CpuElegido->puerto;

		strcpy(CpuElegido_ip , CpuElegido->ip);
		Macro_Mutex_UnLock(GLOBAL_Mutex_lista_CPUs , "GLOBAL_Mutex_lista_CPUs");

		if (CpuElegido->estado != LIBRE) {
			LOG_escribir(GLOBAL_ARCHIVO_LOGS , __func__ , LOG_LEVEL_WARNING , "Se ha elegido al CPU Numero '%d' que tiene Estado Invalido. Estado: '%s'" , CpuElegido->numeroCPU , obtenerNombreEstadoCPU(CpuElegido->estado));
		} else {
			LOG_escribir(GLOBAL_ARCHIVO_LOGS , __func__ , LOG_LEVEL_TRACE , "Se ha elegido al CPU Numero '%d' para que Ejecute" , CpuElegido->numeroCPU);
		}

		SOCKET_crear(socketConectadoCPU);
		iNumeroError = conectarReintentando(socketConectadoCPU , CpuElegido_ip , string_itoa(CpuElegido_puerto));
		if (iNumeroError <= 0) {
			LOG_escribir(GLOBAL_ARCHIVO_LOGS , __func__ , LOG_LEVEL_WARNING , "No me pude Conectar a CPU de IP:'%s' Puerto:'%d'. El mPro '%s' No ha sido Planificado en esta instancia" , CpuElegido_ip , CpuElegido_puerto , basename(mProElegido->PCB.archivomProc));

			desconectarCPU(CpuElegido);
			Macro_Semaforo_Signal(GLOBAL_Semaforo_mPro_listos, "GLOBAL_Semaforo_mPro_listos");
			continue;
		}

		t_order_header headerEnviar;		headerEnviar.sender_id = PLANIFICADOR;		headerEnviar.comando = EJECUTAR_RAFAGA;		//XXX Cambiar por Constructor de Julian si llegara a Crearlo
		iNumeroError = socketConectadoCPU->Enviar(socketConectadoCPU , NULL , headerEnviar , NULL);
		if (iNumeroError <= 0) {
			LOG_escribir(GLOBAL_ARCHIVO_LOGS , __func__ , LOG_LEVEL_WARNING , "No pude Enviar Orden Inicial a CPU de IP:'%s' Puerto:'%d'. El mPro '%s' No ha sido Planificado en esta instancia" , CpuElegido_ip , CpuElegido_puerto , basename(mProElegido->PCB.archivomProc));

			desconectarCPU(CpuElegido);
			Macro_Semaforo_Signal(GLOBAL_Semaforo_mPro_listos, "GLOBAL_Semaforo_mPro_listos");
			continue;
		}


		iNumeroError = socketConectadoCPU->Enviar(socketConectadoCPU , (void*) &mProElegido->PCB , headerEnviar , Serializar_PCB);
		if (iNumeroError <= 0) {
			LOG_escribir(GLOBAL_ARCHIVO_LOGS , __func__ , LOG_LEVEL_WARNING , "No pude Enviar PCB a CPU de IP:'%s' Puerto:'%d'. El mPro '%s' No ha sido Planificado en esta instancia" , CpuElegido_ip , CpuElegido_puerto , basename(mProElegido->PCB.archivomProc));

			desconectarCPU(CpuElegido);
			Macro_Semaforo_Signal(GLOBAL_Semaforo_mPro_listos , "GLOBAL_Semaforo_mPro_listos");
			continue;
		}


		Macro_Mutex_Lock(GLOBAL_Mutex_lista_mPro , "GLOBAL_Mutex_lista_mPro");
		strcpy(mProElegido->estado , EJECUTANDO);
		Metricas_actualizarTiempoEspera(mProElegido);
		Macro_Mutex_UnLock(GLOBAL_Mutex_lista_mPro , "GLOBAL_Mutex_lista_mPro");


		Macro_Mutex_Lock(GLOBAL_Mutex_lista_CPUs , "GLOBAL_Mutex_lista_CPUs");
		CpuElegido->ID_mPro_Ejecutando = mProElegido->PCB.id;
		CpuElegido->estado = OCUPADO;
		Macro_Mutex_UnLock(GLOBAL_Mutex_lista_CPUs , "GLOBAL_Mutex_lista_CPUs");


		LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_INFO, "El mPro '%s' esta Ejecutandose en CPU Numero '%d' con Quantum=%d", basename(mProElegido->PCB.archivomProc), CpuElegido->numeroCPU, mProElegido->PCB.InstruccionesRafaga );


		iNumeroError = socketConectadoCPU->Desconectar(socketConectadoCPU);
		if (iNumeroError <= 0) {
			LOG_escribir(GLOBAL_ARCHIVO_LOGS , __func__ , LOG_LEVEL_WARNING , "No me pude DesConectar el Socket al CPU de IP:'%s' Puerto:'%d'. Pero el mPro YA HA sido planificado" , CpuElegido_ip , CpuElegido_puerto);
		}

		SOCKET_destruir(socketConectadoCPU);

		LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_TRACE,"Fin de esta instancia de Planificacion");

	}

	LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_WARNING,"WHAT! Esta Llegando al Final del Planificador y No Deberia!!");

	//Nunca Llega Aca
	return;
}


void ThreadBloqueos(void* valorQueNoUso){
	int 			cantidadElementosLista = 0;
	int 			elementoActual = 0;
	tipo_mPro* 		mProActual = NULL;
	bool			deboHacerSignal = false;

	LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_DEBUG,"Inicio del Thread de Bloqueos");

	while(INFINITO){

		//LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_TRACE,"Ejecutando una Instancia del Thread de Bloqueos");

		//Reviso Lista mPro para ir Desbloqueando (segun corresponda)
		Macro_Mutex_Lock(GLOBAL_Mutex_lista_mPro , "GLOBAL_Mutex_lista_mPro");
		cantidadElementosLista = list_size(GLOBAL_lista_mPro);
		Macro_Mutex_UnLock(GLOBAL_Mutex_lista_mPro, "GLOBAL_Mutex_lista_mPro");

		for (elementoActual = 0; elementoActual < cantidadElementosLista; elementoActual++) {

			deboHacerSignal = false;

			//LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_TRACE,"Viendo que desbloquear...");

			Macro_Mutex_Lock(GLOBAL_Mutex_lista_mPro , "GLOBAL_Mutex_lista_mPro");
			mProActual = (tipo_mPro*) list_get(GLOBAL_lista_mPro , elementoActual);

			if( (mProActual->segundosBloqueo > 0)  && string_equals_ignore_case(mProActual->estado, BLOQUEADO)){
				mProActual->segundosBloqueo--;

			}else if( (mProActual->segundosBloqueo==0) && string_equals_ignore_case(mProActual->estado, BLOQUEADO) ){
				//Desbloqueo
				Metricas_fixBloqueos(mProActual);
				strcpy(mProActual->estado, LISTO);
				deboHacerSignal = true;
				LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_INFO,"mPro: '%s' Desbloqueado", basename(mProActual->PCB.archivomProc) );
			}
			Macro_Mutex_UnLock(GLOBAL_Mutex_lista_mPro, "GLOBAL_Mutex_lista_mPro");

			if(deboHacerSignal == true){
				Macro_Semaforo_Signal(GLOBAL_Semaforo_mPro_listos, "GLOBAL_Semaforo_mPro_listos");
			}
		}

		//Bloqueo por 1 segundo, asi me aseguro que realmente se cumpla el tiempo establecido de bloqueo de cada mPro
		sleep(1);
	}


	return;
}



void ThreadServidor(void* valorQueNoUso){
	int iNumeroError = 0;		//Variable para Controlar Errores
	LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_DEBUG,"Inicio del Thread Servidor");

	SOCKET_crear(socketDeEscucha);		//NOTA: Cambiar a dentro del While si decido Que se creen threads independientes por cada orden
	SOCKET_crear(socketCliente);
	iNumeroError = socketDeEscucha->Escuchar(socketDeEscucha, GLOBAL_valoresArchivoConfig.puertoDeEscucha);
	if( iNumeroError<=0 ){
		LOG_escribirDebugPantalla(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_ERROR, "Error al Ponerme en Escucha. Me Cierro");
		return;
	}

	//Acepto nuevos clientes infinitamente, bloqueandose hasta que se conecte un nuevo cliente
	while ( INFINITO) {
		iNumeroError = 0;		//Limpio para Evitar Problemas

		Macro_ImprimirParaDebug("Esperando Nuevo Cliente...\n");
		LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_TRACE,"Servidor esperando Nuevo Cliente");

		iNumeroError = socketDeEscucha->Aceptar_Cliente(socketDeEscucha , socketCliente);
		if (iNumeroError <= 0) {
			LOG_escribirDebugPantalla(GLOBAL_ARCHIVO_LOGS , __func__ , LOG_LEVEL_WARNING , "Error al Aceptar un Cliente");
			continue;
		}

		//Variable para el header
		t_order_header headerRecibido;


		//Aca vamos a esperar a que me llegue una peticion
		iNumeroError = socketCliente->Recibir(socketCliente , &headerRecibido , NULL , NULL);
		if (iNumeroError <= 0) {
			LOG_escribirDebugPantalla(GLOBAL_ARCHIVO_LOGS , __func__ , LOG_LEVEL_WARNING , "Error al Recibir Header de un Cliente");
			continue;
		}


		//Veo quien se me conecto y en base a eso voy a ver que orden/peticion me mandaron para atenderla
		switch (headerRecibido.sender_id) {
			//Veo si se me conecto algun CPU
			case CPU:
				Servidor_diferenciarOrden(headerRecibido, socketCliente);
				break;
			default:
				LOG_escribirDebugPantalla(GLOBAL_ARCHIVO_LOGS , __func__ , LOG_LEVEL_WARNING , "Llego una Orden de alguien que no reconosco, quien mando: %d" , headerRecibido.sender_id);
				break;
		}

		iNumeroError = socketCliente->Desconectar(socketCliente);
		if (iNumeroError <= 0) {
			LOG_escribirDebugPantalla(GLOBAL_ARCHIVO_LOGS , __func__ , LOG_LEVEL_WARNING , "Error al Desconectar a un Cliente");
			continue;
		}
	}

	//En teoria nunca ejecutaria aca abajo ni en necesario
	return;
}




void Servidor_diferenciarOrden(t_order_header headerRecibido, t_socket* socketCliente){
	//Veo que orden me enviaron
	switch (headerRecibido.comando) {
		case CONECTAR_CPU_ADMIN:
			Servidor_Orden_ConectarAdminCPU(socketCliente);
			break;

		case CONECTAR_CPU:
			Servidor_Orden_ConectarCPU(socketCliente);
			break;

		case FIN_CON_ERROR:
			Servidor_Orden_FinPorError(socketCliente);
			break;

		case FIN_RAFAGA:
			Servidor_Orden_FinRafaga(socketCliente);
			break;

		case BLOQUEO_ES:
			Servidor_Orden_BloqueoES(socketCliente);
			break;

		case FIN_NORMAL:
			Servidor_Orden_FinNormal(socketCliente);
			break;

		default:
			LOG_escribirDebugPantalla(GLOBAL_ARCHIVO_LOGS , __func__ , LOG_LEVEL_WARNING , "Llego una orden de un CPU que no se la reconoce, la orden decia: %d" , headerRecibido.comando);
			break;
	}

	return;
}



void Servidor_Orden_ConectarAdminCPU(t_socket* socketCliente){
	t_order_header headerEnviar;
	LOG_escribirDebugPantalla(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_DEBUG, "Llego una nueva Orden de Conectar Admin CPU");

	strcpy( GLOBAL_adminCPU.ip, socketCliente->ipCliente);
	GLOBAL_adminCPU.puerto = socketCliente->Puerto_Cliente;

	//XXX Devolver el Puerto si Lucas lo necesita
	headerEnviar.comando = OK;			headerEnviar.sender_id = PLANIFICADOR;
	if ( socketCliente->Enviar(socketCliente, NULL, headerEnviar, NULL)<=0  ){
		LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_WARNING, "No pude devovler OK al Admin CPU");
	}

	LOG_escribirDebugPantalla(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_DEBUG, "Administrador de CPUs Conectado al Planificador. IP:'%s' Puerto:'%d'",GLOBAL_adminCPU.ip, GLOBAL_adminCPU.puerto);
	return;
}


void Servidor_Orden_ConectarCPU(t_socket* socketCliente){
	int 			iNumeroError = 0;		//Variable para Controlar Errores
	tipo_CPU* 		nuevoCPU = NULL;
	char*			numeroCPU_temporal;
	uint32_t 		numeroCPUConvertido = 0;
	t_order_header 	headerQueNoUso;
	t_order_header 	headerEnviar;


	LOG_escribirDebugPantalla(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_DEBUG, "Llego una nueva Orden de Conectar CPU");

	iNumeroError = socketCliente->Recibir(socketCliente, &headerQueNoUso, (void**)&numeroCPU_temporal, DeSerializarString);
	if( iNumeroError<=0 ){
		LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_ERROR, "Fallo Orden porque fallo al recibir");
		return;
	}

	//Convierto de Char a Int y de Int a Uint32
	numeroCPUConvertido = atoi(numeroCPU_temporal);		//Ojo, Upgrade Casting

	nuevoCPU = malloc( sizeof(tipo_CPU) );
	nuevoCPU->numeroCPU = numeroCPUConvertido;
	nuevoCPU->estado = LIBRE;
	nuevoCPU->ID_mPro_Ejecutando = 0;	//Debo Inicializar a Algun Valor
	strcpy( nuevoCPU->ip, socketCliente->ipCliente);
	nuevoCPU->puerto = socketCliente->Puerto_Cliente;

	//XXX Devolver el Puerto si Lucas lo necesita
	headerEnviar.comando = OK;			headerEnviar.sender_id = PLANIFICADOR;
	if ( socketCliente->Enviar(socketCliente, NULL, headerEnviar, NULL)<=0  ){
		LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_WARNING, "No pude devovler OK al CPU que se conecto");
	}

	Macro_Mutex_Lock(GLOBAL_Mutex_lista_CPUs, "GLOBAL_Mutex_lista_CPUs");
	list_add(GLOBAL_lista_CPUs, nuevoCPU);
	Macro_Mutex_UnLock(GLOBAL_Mutex_lista_CPUs, "GLOBAL_Mutex_lista_CPUs");


	Macro_Semaforo_Signal(GLOBAL_Semaforo_CPU_libres , "GLOBAL_Semaforo_CPU_libres");

	LOG_escribirDebugPantalla(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_INFO, "CPU '%d' Agregado Exitosamente al Planificador", numeroCPUConvertido);
	return;
}


void Servidor_Orden_FinPorError(t_socket* socketCliente){
	int 			iNumeroError = 0;		//Variable para Controlar Errores
	t_order_header 	headerQueNoUso;
	t_rafaga*		rafagaTerminada;
	t_list*			listaResultadoInstrucciones = NULL;
	tipo_mPro*		mProPorActualizar = NULL;
	tipo_CPU*		CpuPorActualizar = NULL;
	char*			razonError = NULL;

	LOG_escribirDebugPantalla(GLOBAL_ARCHIVO_LOGS , __func__ , LOG_LEVEL_DEBUG , "Llego una nueva Orden de Fin de mPro por Error");

	iNumeroError = socketCliente->Recibir(socketCliente , &headerQueNoUso , (void**) &rafagaTerminada , DeSerializar_Resultado_Rafaga);
	Macro_Check_And_Handle_Error(iNumeroError<=0, "Fallo al recibir PCB de mPro + Numero CPU");


	iNumeroError = socketCliente->Recibir(socketCliente , &headerQueNoUso , (void**)&listaResultadoInstrucciones , DeSerializar_Lista_Resultados_Rafaga);
	Macro_Check_And_Handle_Error(iNumeroError<=0, "Fallo al recibir Lista de Resultados Instrucciones");



	iNumeroError = socketCliente->Recibir(socketCliente , &headerQueNoUso , (void**)&razonError , DeSerializarString);
	Macro_Check_And_Handle_Error(iNumeroError<=0, "Fallo al recibir Razon de Error");

	loguearResultadoEjecucion(listaResultadoInstrucciones, basename(rafagaTerminada->pcb.archivomProc));

	//Busco y Actualizo mPro
	Macro_Mutex_Lock(GLOBAL_Mutex_lista_mPro, "GLOBAL_Mutex_lista_mPro");
	mProPorActualizar = buscarMPro(rafagaTerminada->pcb.id);
	Metricas_actualizarTiempoEjecucion(mProPorActualizar);

	mProPorActualizar->PCB.PC = rafagaTerminada->pcb.PC;
	strcpy( mProPorActualizar->estado, generarEstadoError(razonError) );
	Macro_Mutex_UnLock(GLOBAL_Mutex_lista_mPro, "GLOBAL_Mutex_lista_mPro");


	//Busco y Actualizo CPU
	Macro_Mutex_Lock(GLOBAL_Mutex_lista_CPUs, "GLOBAL_Mutex_lista_CPUs");
	CpuPorActualizar = buscarCPU(rafagaTerminada->numeroCPU);

	CpuPorActualizar->estado = LIBRE;
	CpuPorActualizar->ID_mPro_Ejecutando = 0;
	Macro_Mutex_UnLock(GLOBAL_Mutex_lista_CPUs, "GLOBAL_Mutex_lista_CPUs");


	Macro_Semaforo_Signal(GLOBAL_Semaforo_CPU_libres , "GLOBAL_Semaforo_CPU_libres");

	LOG_escribirSeparador(GLOBAL_ARCHIVO_LOGS);
	LOG_escribirDebugPantalla(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_WARNING, "El mPro: '%s' ha terminado con Error. El CPU '%d' esta Libre ahora.", basename(rafagaTerminada->pcb.archivomProc), rafagaTerminada->numeroCPU);
	Metricas_loguear(mProPorActualizar);
	return;

Error_Handler:
	LOG_escribirDebugPantalla(GLOBAL_ARCHIVO_LOGS , __func__ , LOG_LEVEL_ERROR , "Fallo la Orden porque no pude Recibir Correctamente por la Red.");

	return;
}


void Servidor_Orden_FinRafaga(t_socket* socketCliente){
	int 			iNumeroError = 0;		//Variable para Controlar Errores
	t_order_header 	headerQueNoUso;
	t_rafaga* 		rafagaTerminada;
	t_list* 		listaResultadoInstrucciones = NULL;
	tipo_mPro* 		mProPorActualizar = NULL;
	tipo_CPU* 		CpuPorActualizar = NULL;

	LOG_escribirDebugPantalla(GLOBAL_ARCHIVO_LOGS , __func__ , LOG_LEVEL_DEBUG , "Llego una nueva Orden de Fin de Rafaga de un mPro");

	iNumeroError = socketCliente->Recibir(socketCliente , &headerQueNoUso , (void**) &rafagaTerminada ,DeSerializar_Resultado_Rafaga);
	Macro_Check_And_Handle_Error(iNumeroError <= 0 , "Fallo al recibir PCB de mPro + Numero CPU");

	iNumeroError = socketCliente->Recibir(socketCliente , &headerQueNoUso , (void**) &listaResultadoInstrucciones , DeSerializar_Lista_Resultados_Rafaga);
	Macro_Check_And_Handle_Error(iNumeroError <= 0 , "Fallo al recibir Lista de Resultados Instrucciones");

	loguearResultadoEjecucion(listaResultadoInstrucciones , basename(rafagaTerminada->pcb.archivomProc));

	//Busco y Actualizo mPro
	Macro_Mutex_Lock(GLOBAL_Mutex_lista_mPro , "GLOBAL_Mutex_lista_mPro");
	mProPorActualizar = buscarMPro(rafagaTerminada->pcb.id);
	Metricas_actualizarTiempoEjecucion(mProPorActualizar);

	if ( string_equals_ignore_case(mProPorActualizar->estado, EJECUTANDO_FINALIZAR)==false  ) {
		mProPorActualizar->PCB.PC = rafagaTerminada->pcb.PC;
		strcpy(mProPorActualizar->estado , LISTO);
	}else{
		strcpy(mProPorActualizar->estado , POR_FINALIZAR);
	}
	Macro_Mutex_UnLock(GLOBAL_Mutex_lista_mPro , "GLOBAL_Mutex_lista_mPro");

	Macro_Semaforo_Signal(GLOBAL_Semaforo_mPro_listos , "GLOBAL_Semaforo_mPro_listos");

	//Busco y Actualizo CPU
	Macro_Mutex_Lock(GLOBAL_Mutex_lista_CPUs , "GLOBAL_Mutex_lista_CPUs");
	CpuPorActualizar = buscarCPU(rafagaTerminada->numeroCPU);

	CpuPorActualizar->estado = LIBRE;
	CpuPorActualizar->ID_mPro_Ejecutando = 0;
	Macro_Mutex_UnLock(GLOBAL_Mutex_lista_CPUs , "GLOBAL_Mutex_lista_CPUs");

	Macro_Semaforo_Signal(GLOBAL_Semaforo_CPU_libres , "GLOBAL_Semaforo_CPU_libres");

	LOG_escribirDebugPantalla(GLOBAL_ARCHIVO_LOGS , __func__ , LOG_LEVEL_INFO , "El mPro: '%s' ha terminado correctamente una Rafaga. El CPU '%d' esta Libre ahora." , basename(rafagaTerminada->pcb.archivomProc) , rafagaTerminada->numeroCPU);
	return;


Error_Handler:
	LOG_escribirDebugPantalla(GLOBAL_ARCHIVO_LOGS , __func__ , LOG_LEVEL_ERROR , "Fallo la Orden porque no pude Recibir Correctamente por la Red.");

	return;
}


void Servidor_Orden_BloqueoES(t_socket* socketCliente){
	int 			iNumeroError = 0;		//Variable para Controlar Errores
	t_order_header	headerQueNoUso;
	t_rafaga* 		rafagaTerminada;
	t_list* 		listaResultadoInstrucciones = NULL;
	tipo_mPro* 		mProPorActualizar = NULL;
	tipo_CPU* 		CpuPorActualizar = NULL;
	char*			segundosBloqueo = NULL;
	bool			deboHacerSignal = false;

	LOG_escribirDebugPantalla(GLOBAL_ARCHIVO_LOGS , __func__ , LOG_LEVEL_DEBUG , "Llego una nueva Orden de Bloqueo de mPro");

	iNumeroError = socketCliente->Recibir(socketCliente , &headerQueNoUso , (void**) &rafagaTerminada , DeSerializar_Resultado_Rafaga);
	Macro_Check_And_Handle_Error(iNumeroError <= 0 , "Fallo al recibir PCB de mPro + Numero CPU");

	iNumeroError = socketCliente->Recibir(socketCliente , &headerQueNoUso , (void**) &listaResultadoInstrucciones , DeSerializar_Lista_Resultados_Rafaga);
	Macro_Check_And_Handle_Error(iNumeroError <= 0 , "Fallo al recibir Lista de Resultados Instrucciones");

	//TODO Decirle a Lucas que me envie el Numero de Segundos de Bloqueo al final.

	iNumeroError = socketCliente->Recibir(socketCliente , &headerQueNoUso , (void**) &segundosBloqueo , DeSerializarString);
	Macro_Check_And_Handle_Error(iNumeroError<=0, "Fallo al recibir Segundos de Bloqueo");


	loguearResultadoEjecucion(listaResultadoInstrucciones , basename(rafagaTerminada->pcb.archivomProc));

	//Busco y Actualizo mPro
	Macro_Mutex_Lock(GLOBAL_Mutex_lista_mPro , "GLOBAL_Mutex_lista_mPro");
	mProPorActualizar = buscarMPro(rafagaTerminada->pcb.id);
	Metricas_actualizarTiempoEjecucion(mProPorActualizar);

	if ( string_equals_ignore_case(mProPorActualizar->estado, EJECUTANDO_FINALIZAR)==false  ) {
		mProPorActualizar->PCB.PC = rafagaTerminada->pcb.PC;
		strcpy(mProPorActualizar->estado , BLOQUEADO);
		mProPorActualizar->segundosBloqueo = atoi(segundosBloqueo);		//Ojo, Upgrade Casting
	} else {
		strcpy(mProPorActualizar->estado , POR_FINALIZAR);
		deboHacerSignal = true;
	}
	Macro_Mutex_UnLock(GLOBAL_Mutex_lista_mPro , "GLOBAL_Mutex_lista_mPro");

	if (deboHacerSignal == true) {
		Macro_Semaforo_Signal(GLOBAL_Semaforo_mPro_listos , "GLOBAL_Semaforo_mPro_listos");
		//Nota: Lo Hice asi de raro y rebuscado para evitar hacer el Signal dentro del Mutex, para achicar la zona critica y evitar problemas de Condicion de Carrera
	}

	//Busco y Actualizo CPU
	Macro_Mutex_Lock(GLOBAL_Mutex_lista_CPUs , "GLOBAL_Mutex_lista_CPUs");
	CpuPorActualizar = buscarCPU(rafagaTerminada->numeroCPU);

	CpuPorActualizar->estado = LIBRE;
	CpuPorActualizar->ID_mPro_Ejecutando = 0;
	Macro_Mutex_UnLock(GLOBAL_Mutex_lista_CPUs , "GLOBAL_Mutex_lista_CPUs");

	Macro_Semaforo_Signal(GLOBAL_Semaforo_CPU_libres , "GLOBAL_Semaforo_CPU_libres");

	LOG_escribirDebugPantalla(GLOBAL_ARCHIVO_LOGS , __func__ , LOG_LEVEL_INFO , "El mPro: '%s' se Bloqueaa por '%d' Segundos. El CPU '%d' esta Libre ahora." , basename(rafagaTerminada->pcb.archivomProc) ,  atoi(segundosBloqueo), rafagaTerminada->numeroCPU);
	return;


Error_Handler:
	LOG_escribirDebugPantalla(GLOBAL_ARCHIVO_LOGS , __func__ , LOG_LEVEL_ERROR , "Fallo la Orden porque no pude Recibir Correctamente por la Red.");

	return;
}


void Servidor_Orden_FinNormal(t_socket* socketCliente){
	int 			iNumeroError = 0;		//Variable para Controlar Errores
	t_order_header 	headerQueNoUso;
	t_rafaga*		rafagaTerminada;
	t_list*			listaResultadoInstrucciones = NULL;
	tipo_mPro*		mProPorActualizar = NULL;
	tipo_CPU*		CpuPorActualizar = NULL;

	LOG_escribirDebugPantalla(GLOBAL_ARCHIVO_LOGS , __func__ , LOG_LEVEL_DEBUG , "Llego una nueva Orden de Fin Normal de mPro");

	iNumeroError = socketCliente->Recibir(socketCliente , &headerQueNoUso , (void**) &rafagaTerminada , DeSerializar_Resultado_Rafaga);
	Macro_Check_And_Handle_Error(iNumeroError<=0, "Fallo al recibir PCB de mPro + Numero CPU");


	iNumeroError = socketCliente->Recibir(socketCliente , &headerQueNoUso , (void**)&listaResultadoInstrucciones , DeSerializar_Lista_Resultados_Rafaga);
	Macro_Check_And_Handle_Error(iNumeroError<=0, "Fallo al recibir Lista de Resultados Instrucciones");


	loguearResultadoEjecucion(listaResultadoInstrucciones, basename(rafagaTerminada->pcb.archivomProc));

	//Busco y Actualizo mPro
	Macro_Mutex_Lock(GLOBAL_Mutex_lista_mPro, "GLOBAL_Mutex_lista_mPro");
	mProPorActualizar = buscarMPro(rafagaTerminada->pcb.id);
	Metricas_actualizarTiempoEjecucion(mProPorActualizar);

	mProPorActualizar->PCB.PC = rafagaTerminada->pcb.PC;
	strcpy( mProPorActualizar->estado, TERMINADO );
	Macro_Mutex_UnLock(GLOBAL_Mutex_lista_mPro, "GLOBAL_Mutex_lista_mPro");


	//Busco y Actualizo CPU
	Macro_Mutex_Lock(GLOBAL_Mutex_lista_CPUs, "GLOBAL_Mutex_lista_CPUs");
	CpuPorActualizar = buscarCPU(rafagaTerminada->numeroCPU);

	CpuPorActualizar->estado = LIBRE;
	CpuPorActualizar->ID_mPro_Ejecutando = 0;
	Macro_Mutex_UnLock(GLOBAL_Mutex_lista_CPUs, "GLOBAL_Mutex_lista_CPUs");


	Macro_Semaforo_Signal(GLOBAL_Semaforo_CPU_libres , "GLOBAL_Semaforo_CPU_libres");

	LOG_escribirSeparador(GLOBAL_ARCHIVO_LOGS);
	LOG_escribirDebugPantalla(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_INFO, "El mPro: '%s' ha terminado Correctamente. El CPU '%d' esta Libre ahora.", basename(rafagaTerminada->pcb.archivomProc), rafagaTerminada->numeroCPU);
	Metricas_loguear(mProPorActualizar);
	return;

Error_Handler:
	LOG_escribirDebugPantalla(GLOBAL_ARCHIVO_LOGS , __func__ , LOG_LEVEL_ERROR , "Fallo la Orden porque no pude Recibir Correctamente por la Red.");

	return;
}



void Consola_Comando_Correr(const char* rutaLinuxArchivo){
	int iNumeroError = 0;			//Variable para Controlar Errores, en 0 significa no error
	tipo_mPro* nuevoMPro = NULL;
	int cantidadElementosAntes = 0;
	int cantidadElementosDespues = 0;

	//Valido que no este vacio
	if (strlen(rutaLinuxArchivo) <= 0) {
		printf(ANSI_COLOR_RED "Ingrese PATH Archivo mPro" ANSI_COLOR_RESET "\n");
		return;
	}

	iNumeroError = Comun_existeArchivo(rutaLinuxArchivo);
	if (iNumeroError == -1) {
		printf(ANSI_COLOR_RED "No Existe Archivo mPro o No se tienen permisos de Lectura, Escritura y Ejecucion" ANSI_COLOR_RESET "\n");
		return;
	}

	nuevoMPro = malloc( sizeof(tipo_mPro) );
	nuevoMPro->PCB.id = GLOBAL_Numero_ID_mPro;

	GLOBAL_Numero_ID_mPro++;	//Lo Aumento

	strcpy(nuevoMPro->PCB.archivomProc , rutaLinuxArchivo);
	nuevoMPro->PCB.PC = 0;
	nuevoMPro->PCB.InstruccionesRafaga = 0;						//Debo Inicializar a Algo
	strcpy(nuevoMPro->estado , LISTO);
	nuevoMPro->segundosBloqueo = 0;								//Debo Inicializar a Algo
	Metricas_inicializar(nuevoMPro);

	Macro_Mutex_Lock(GLOBAL_Mutex_lista_mPro , "GLOBAL_Mutex_lista_mPro");
	cantidadElementosAntes = list_size(GLOBAL_lista_mPro);
	list_add(GLOBAL_lista_mPro, nuevoMPro);
	cantidadElementosDespues = list_size(GLOBAL_lista_mPro);
	Macro_Mutex_UnLock(GLOBAL_Mutex_lista_mPro, "GLOBAL_Mutex_lista_mPro");

	//Chequeo Commons
	if( cantidadElementosAntes>=cantidadElementosDespues ){
		LOG_escribirPantalla(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_ERROR, "Andan mal las Commons, funcion 'list_size' o 'list_add'");
		return;
	}


	Macro_Semaforo_Signal(GLOBAL_Semaforo_mPro_listos, "GLOBAL_Semaforo_mPro_listos");

	LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_INFO, "mPro '%s' Agregado al Planificador", basename( (char*)rutaLinuxArchivo));
	printf(ANSI_COLOR_GREEN "mPro '%s' Agregado al Planificador, sera Ejecutado en Breve" ANSI_COLOR_RESET "\n", basename( (char*)rutaLinuxArchivo) );
	return;
}

void Consola_Comando_PS(){
	int 			cantidadElementosLista = 0;
	int 			elementoActual = 0;
	tipo_mPro* 		mProActual = NULL;

	Macro_Mutex_Lock(GLOBAL_Mutex_lista_mPro , "GLOBAL_Mutex_lista_mPro");
	cantidadElementosLista = list_size(GLOBAL_lista_mPro);
	Macro_ImprimirParaDebug("Cantidad Elementos Lista: %d\n", cantidadElementosLista);

	for (elementoActual = 0; elementoActual < cantidadElementosLista; elementoActual++) {
		mProActual = (tipo_mPro*) list_get(GLOBAL_lista_mPro , elementoActual);
		printf("mProc %d: %s  -> %s\n", mProActual->PCB.id, basename(mProActual->PCB.archivomProc), mProActual->estado );
	}
	Macro_Mutex_UnLock(GLOBAL_Mutex_lista_mPro, "GLOBAL_Mutex_lista_mPro");

	return;
}

void Consola_Comando_Finalizar(const char* mProID){
	int				cantidadElementosLista = 0;
	int 			elementoActual = 0;
	tipo_mPro* 		mProActual = NULL;
	int				mProID_Temporal = 0;
	uint32_t		mProID_Convertida = 0;
	long			posicionInstruccionFinalizar = 0;

	//Primera Conversion
	mProID_Temporal = atoi(mProID);
	LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_TRACE, "mProID_Temporal: '%d'", mProID_Temporal);

	//Segunda Conversion
	//TODO Revisar Upgrade Casting en C para ver que no estoy haciendo Cagada
	mProID_Convertida = mProID_Temporal;
	LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_TRACE, "mProID_Convertida: '%d'", mProID_Convertida);

	Macro_Mutex_Lock(GLOBAL_Mutex_lista_mPro , "GLOBAL_Mutex_lista_mPro");
	cantidadElementosLista = list_size(GLOBAL_lista_mPro);
	Macro_ImprimirParaDebug("Cantidad Elementos Lista: %d\n" , cantidadElementosLista);

	for (elementoActual = 0; elementoActual < cantidadElementosLista; elementoActual++) {
		mProActual = (tipo_mPro*) list_get(GLOBAL_lista_mPro , elementoActual);

		if( mProActual->PCB.id == mProID_Convertida ){
			break;	//Ya encontre el mPro Buscado
		}
	}
	Macro_Mutex_UnLock(GLOBAL_Mutex_lista_mPro , "GLOBAL_Mutex_lista_mPro");

	if (mProActual->PCB.id != mProID_Convertida) {
		printf(ANSI_COLOR_RED "No Existe el mPro o Nunca fue Puesto a Correr" ANSI_COLOR_RESET "\n");
		return;		//Lo puse aca para Return Fuera del Mutex
	}

	if (string_equals_ignore_case(mProActual->estado , POR_FINALIZAR)  || string_equals_ignore_case(mProActual->estado , EJECUTANDO_FINALIZAR)) {
		printf(ANSI_COLOR_YELLOW "El mPro Ya esta por Finalizarse" ANSI_COLOR_RESET "\n");
		return;
	}

	if ( string_equals_ignore_case(mProActual->estado,TERMINADO) || string_starts_with(mProActual->estado, "ERROR") ) {
		printf(ANSI_COLOR_YELLOW "El mPro Ya esta Finalizado" ANSI_COLOR_RESET "\n");
		return;
	}


	posicionInstruccionFinalizar = obtenerPosicioninstruccionFinalizar(mProActual->PCB.archivomProc);
	if( posicionInstruccionFinalizar==-1 ){
		printf(ANSI_COLOR_RED "No pude encontrar la Instruccion Finalizar dentro del Archivo" ANSI_COLOR_RESET "\n");
		return;
	}



	Macro_Mutex_Lock(GLOBAL_Mutex_lista_mPro , "GLOBAL_Mutex_lista_mPro");
	//TODO Revisar Upgrade Casting en C para ver que no estoy haciendo Cagada
	mProActual->PCB.PC = posicionInstruccionFinalizar;
	if(string_equals_ignore_case(mProActual->estado, EJECUTANDO)){
		strcpy(mProActual->estado, EJECUTANDO_FINALIZAR);
	}else{
		strcpy(mProActual->estado, POR_FINALIZAR);
	}
	Macro_Mutex_UnLock(GLOBAL_Mutex_lista_mPro , "GLOBAL_Mutex_lista_mPro");

	LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_INFO, "Se mando a Finalizar al mPro '%s'", basename( (char*)mProActual->PCB.archivomProc));
	printf(ANSI_COLOR_GREEN "El mPro '%s' Finalizara en Breve" ANSI_COLOR_RESET "\n", basename( (char*)mProActual->PCB.archivomProc) );
	return;
}

void Consola_Comando_CPU(){
	int 			numeroError = 1;	//Para controlar errores
	int 			cantidadElementosLista = 0;
	int 			elementoActual = 0;
	t_order_header 	headerEnviar;
	t_order_header 	headerQueNoUso;
	t_list*			listaUsoCPU = NULL;
	t_uso_CPU*		usoCPUActual = NULL;


	SOCKET_crear(socketAdminCPU);

	numeroError = socketAdminCPU->Conectar(socketAdminCPU, GLOBAL_adminCPU.ip, string_itoa(GLOBAL_adminCPU.puerto) );
	Macro_Check_And_Handle_Error(numeroError <= 0, "No pude conectarme al Admin CPU. Puede ser que nunca se haya conectado el Admin CPU o un Problema de Red");

	headerEnviar.comando=USO_CPU;			headerEnviar.sender_id=PLANIFICADOR;
	numeroError = socketAdminCPU->Enviar(socketAdminCPU, NULL, headerEnviar, NULL);
	Macro_Check_And_Handle_Error(numeroError <= 0, "No pude pedirle Orden al Admin CPU. Puede ser un Problema de Red");

	numeroError = socketAdminCPU->Recibir(socketAdminCPU, &headerQueNoUso, (void**) &listaUsoCPU, DeSerializar_USOCPU);
	Macro_Check_And_Handle_Error(numeroError <= 0, "No pude Recibir Datos del Admin CPU. Puede ser un Problema de Red");


	cantidadElementosLista = list_size(listaUsoCPU);
	Macro_ImprimirParaDebug("Cantidad Elementos Lista: %d\n", cantidadElementosLista);

	for (elementoActual = 0; elementoActual < cantidadElementosLista; elementoActual++) {
		usoCPUActual = (t_uso_CPU*) list_get(listaUsoCPU , elementoActual);
		printf("CPU Numero %u -> %u%%\n", usoCPUActual->id_cpu, usoCPUActual->tiempo_uso );
	}

	return;

Error_Handler:
	printf(ANSI_COLOR_RED "Fallo el Comando. Puede ser que nunca se haya conectado el Admin CPU o un Problema de Red" ANSI_COLOR_RESET "\n");

	return;
}


void Consola_Comando_DeadLockChecker(){

	Macro_ImprimirEstadoInicio("Viendo lista mPro..");
	Macro_Mutex_Lock(GLOBAL_Mutex_lista_mPro , "GLOBAL_Mutex_lista_mPro");
	Macro_Mutex_UnLock(GLOBAL_Mutex_lista_mPro , "GLOBAL_Mutex_lista_mPro");
	Macro_ImprimirEstadoFinal(EJECUCION_CORRECTA);

	Macro_ImprimirEstadoInicio("Viendo lista CPUs..");
	Macro_Mutex_Lock(GLOBAL_Mutex_lista_CPUs , "GLOBAL_Mutex_lista_CPUs");
	Macro_Mutex_UnLock(GLOBAL_Mutex_lista_CPUs , "GLOBAL_Mutex_lista_CPUs");
	Macro_ImprimirEstadoFinal(EJECUCION_CORRECTA);

	printf(ANSI_COLOR_GREEN "No hay DeadLocks" ANSI_COLOR_RESET "\n");
	return;
}

void Consola_Comando_StatusCpu(){
	int 		cantidadElementosLista = 0;
	int 		elementoActual = 0;
	tipo_CPU* 	CpuActual = NULL;

	Macro_Mutex_Lock(GLOBAL_Mutex_lista_CPUs , "GLOBAL_Mutex_lista_CPUs");
	cantidadElementosLista = list_size(GLOBAL_lista_CPUs);
	Macro_ImprimirParaDebug("Cantidad Elementos Lista: %d\n" , cantidadElementosLista);

	printf("%3s  %5s  %3s  %7s  %15s\n", "Num", "Estado", "mPro", "IP", "Puerto");

	for (elementoActual = 0; elementoActual < cantidadElementosLista; elementoActual++) {
		CpuActual = (tipo_CPU*) list_get(GLOBAL_lista_CPUs , elementoActual);
		printf("%3d  %5s  %3d  %14s  %8d\n\n", CpuActual->numeroCPU, obtenerNombreEstadoCPU(CpuActual->estado), CpuActual->ID_mPro_Ejecutando, CpuActual->ip, CpuActual->puerto);
	}
	Macro_Mutex_UnLock(GLOBAL_Mutex_lista_CPUs , "GLOBAL_Mutex_lista_CPUs");

	return;
}

void Consola_Comando_StatusMPro(){
	int 		cantidadElementosLista = 0;
	int 		elementoActual = 0;
	tipo_mPro* 	mProActual = NULL;

	Macro_Mutex_Lock(GLOBAL_Mutex_lista_mPro , "GLOBAL_Mutex_lista_mPro");
	cantidadElementosLista = list_size(GLOBAL_lista_mPro);
	Macro_ImprimirParaDebug("Cantidad Elementos Lista: %d\n" , cantidadElementosLista);

	printf("%2s  %15s  %8s  %4s  %5s  %15s  %8s  %3s  %3s  %5s\n" , "ID" , "Nombre" , "PC", "Inst", "Bloq", "Estado", "Res", "Esp", "Eje", "Stamp");

	for (elementoActual = 0; elementoActual < cantidadElementosLista; elementoActual++) {
		mProActual = (tipo_mPro*) list_get(GLOBAL_lista_mPro , elementoActual);

		printf("%1d  %20s  %5d  %4d  %4d  %20s  %3.1f  %3.1f  %3.1f  %5ld\n" , mProActual->PCB.id , mProActual->PCB.archivomProc , mProActual->PCB.PC, mProActual->PCB.InstruccionesRafaga, mProActual->segundosBloqueo, mProActual->estado, mProActual->tiempoRespuesta, mProActual->tiempoEspera, mProActual->tiempoEjecucion, (long int)mProActual->ultimoTimeStamp);
	}
	Macro_Mutex_UnLock(GLOBAL_Mutex_lista_mPro , "GLOBAL_Mutex_lista_mPro");


	return;
}



tipo_TimeStamp	Tiempo_obtenerActual(){
	tipo_TimeStamp tiempoActual = time(NULL);
	return tiempoActual;
}


double Tiempo_calcularDiferencia( tipo_TimeStamp tiempoViejo ){
	double			diferenciaEnSegundos = 0;
	tipo_TimeStamp 	tiempoActual;

	tiempoActual = Tiempo_obtenerActual();
	Macro_Check_And_Handle_Error(tiempoActual==ERROR_TimeStamp, "Error con 'obtenerTiempoActual'");

	diferenciaEnSegundos = difftime( (time_t)tiempoActual, (time_t)tiempoViejo);
	Macro_Check_And_Handle_Error(diferenciaEnSegundos < 0 , "Error con 'difftime'. Dio tiempo Negativo.");

	if ( diferenciaEnSegundos == 0 ){
		LOG_escribir(GLOBAL_ARCHIVO_LOGS, __func__, LOG_LEVEL_WARNING, "Se calculo una diferencia de tiempo y dio 0 segundos. No deberia ser tan corta la diferencia.. Puede que este bien..");
	}

	return diferenciaEnSegundos;

Error_Handler:
	return -1;
}


void Metricas_inicializar(tipo_mPro* mPro){
	tipo_TimeStamp tiempoActual;

	mPro->tiempoRespuesta = 0;
	mPro->tiempoEjecucion = 0;
	mPro->tiempoEspera = 0;

	tiempoActual = Tiempo_obtenerActual();
	Macro_Check_And_Handle_Error(tiempoActual == ERROR_TimeStamp, "Fallo 'Tiempo_obtenerActual'");

	mPro->ultimoTimeStamp = tiempoActual;
	return;

Error_Handler:
	LOG_escribir(GLOBAL_ARCHIVO_LOGS,__func__,LOG_LEVEL_ERROR, "Fallo Inicializacion. Todas las Metricas del mPro '%s' seguramente daran Mal", basename(mPro->PCB.archivomProc));
	return;
}



void Metricas_actualizarTiempoEspera(tipo_mPro* mPro){
	double	diferenciaEnSegundos = 0;
	tipo_TimeStamp tiempoActual;

	diferenciaEnSegundos = Tiempo_calcularDiferencia(mPro->ultimoTimeStamp);
	Macro_Check_And_Handle_Error(diferenciaEnSegundos == -1, "Fallo 'Tiempo_calcularDiferencia'");

	tiempoActual = Tiempo_obtenerActual();
	Macro_Check_And_Handle_Error(tiempoActual == ERROR_TimeStamp , "Fallo 'Tiempo_obtenerActual'");

	mPro->ultimoTimeStamp = tiempoActual;
	mPro->tiempoEspera += diferenciaEnSegundos;
	//Solo se actualiza la 1era vez que se manda a ejecutar, que esta en 0
	if(mPro->tiempoRespuesta == 0){
		mPro->tiempoRespuesta += diferenciaEnSegundos;
	}
	return;

Error_Handler:
	LOG_escribir(GLOBAL_ARCHIVO_LOGS,__func__,LOG_LEVEL_WARNING, "Fallo 'Tiempo_calcularDiferencia' o 'Tiempo_obtenerActual'. No se pudo Actualizar el Tiempo de Espera y/o de Respuesta del mPro '%s'.", basename(mPro->PCB.archivomProc));
	return;
}



void Metricas_actualizarTiempoEjecucion(tipo_mPro* mPro){
	double	diferenciaEnSegundos = 0;
	tipo_TimeStamp tiempoActual;

	diferenciaEnSegundos = Tiempo_calcularDiferencia(mPro->ultimoTimeStamp);
	Macro_Check_And_Handle_Error(diferenciaEnSegundos == -1, "Fallo 'Tiempo_calcularDiferencia'");

	tiempoActual = Tiempo_obtenerActual();
	Macro_Check_And_Handle_Error(tiempoActual == ERROR_TimeStamp , "Fallo 'Tiempo_obtenerActual'");

	mPro->tiempoEjecucion += diferenciaEnSegundos;
	mPro->ultimoTimeStamp = tiempoActual;
	return;

Error_Handler:
	LOG_escribir(GLOBAL_ARCHIVO_LOGS,__func__,LOG_LEVEL_WARNING, "Fallo 'Tiempo_calcularDiferencia' o 'Tiempo_obtenerActual'. No se pudo Actualizar el Tiempo de Ejecucion del mPro '%s'.", basename(mPro->PCB.archivomProc));
	return;
}


void Metricas_fixBloqueos(tipo_mPro* mPro){
	//Se resetea el TimeStamp para que no se sume el tiempo de bloque al tiempo de Espera
	tipo_TimeStamp tiempoActual;

	tiempoActual = Tiempo_obtenerActual();
	Macro_Check_And_Handle_Error(tiempoActual == ERROR_TimeStamp , "Fallo 'Tiempo_obtenerActual'");

	mPro->ultimoTimeStamp = tiempoActual;
	return;

Error_Handler:
	LOG_escribir(GLOBAL_ARCHIVO_LOGS,__func__,LOG_LEVEL_WARNING, "Fallo 'Tiempo_obtenerActual'. No pudo evitarse sumar el tiempo de Bloqueo al de Espera del mPro '%s'.", basename(mPro->PCB.archivomProc));
	return;
}


void Metricas_loguear(tipo_mPro* mPro){
	LOG_escribir(GLOBAL_ARCHIVO_LOGS,__func__,LOG_LEVEL_DEBUG, "ID:'%d' Ruta Completa:'%s'", mPro->PCB.id, mPro->PCB.archivomProc);
	LOG_escribir(GLOBAL_ARCHIVO_LOGS,__func__,LOG_LEVEL_DEBUG, "Tiempo de Respuesta: '%f'", mPro->tiempoRespuesta);
	LOG_escribir(GLOBAL_ARCHIVO_LOGS,__func__,LOG_LEVEL_DEBUG, "Tiempo de Espera: '%f'", mPro->tiempoEspera);
	LOG_escribir(GLOBAL_ARCHIVO_LOGS,__func__,LOG_LEVEL_DEBUG, "Tiempo de Ejecucion: '%f'", mPro->tiempoEjecucion);
	return;
}

int leerConsola() {
	// Funcion para iniciar algunos parametros especiales de lectura
  	rl_attempted_completion_function = completar_comandos;
	
	char *lineaLeida = NULL;
	int resultado = 0;

	// Establezco el Prompt que se va a mostrar
	char* prompttext = string_from_format("%c" ANSI_COLOR_YELLOW "%c>Ingrese un comando: %c" ANSI_COLOR_RESET "%c", RL_PROMPT_START_IGNORE,RL_PROMPT_END_IGNORE, RL_PROMPT_START_IGNORE,RL_PROMPT_END_IGNORE);
	
	// Leo de la consola
	lineaLeida = readline(prompttext);
	if (!lineaLeida) {
		//Ocurrio un error
	}
	string_trim(&lineaLeida);

	if (strlen(lineaLeida) > 0) {
		if (*lineaLeida) {
			add_history(lineaLeida);
			resultado = ejecutarLinea(lineaLeida);
			if (lineaLeida != NULL) {
				free(lineaLeida);
			}
		}
	} else {
		// No se ingreso ningun texto en la consola
		printf("Escriba 'ayuda' para tener una lista de los comandos disponibles.\n");
		resultado = 1;
	}
	return resultado;
}

int ejecutarLinea(char* lineaLeida) {
	COMMAND *comandoEjecutar;
	char *parametros = NULL;
	char* comando = malloc(sizeof(char)*20);
	int ipos = 0, iposparam = 0;
	while((lineaLeida[ipos] != ' ') && (lineaLeida[ipos] != '\0') &&  (lineaLeida[ipos] != '\n' ) ) {
		comando[ipos] = lineaLeida[ipos];
		ipos++;
	}
	comando[ipos] = '\0'; // establece el ultimo caracter para que sea una cadena

	int largoparams = strlen(lineaLeida) - ipos;

	if (largoparams > 0) {
		while (lineaLeida[ipos] == ' ')
			ipos++; // incremento la posicion porque lo anterior es un espacio
		parametros = malloc(sizeof(char)*largoparams); // no sumo 1 porque ya me queda un espacio mas libre
		
		while((lineaLeida[ipos] != '\n') && (lineaLeida[ipos] != '\0'))  {
			parametros[iposparam] = lineaLeida[ipos];
			ipos++;
			iposparam++;
		}
		parametros[iposparam] = '\0';
	} 

	comandoEjecutar = buscar_comando(comando);
	free(comando);
	if (!comandoEjecutar) {
		printf("Comando no reconocido. Escribi 'ayuda' para tener una lista de los comandos.\n");
		// ERROR No se encontro el comando
		free(parametros);
		return -1;
	}
	return ((*(comandoEjecutar->func))(parametros));	
}

COMMAND* buscar_comando(char* nombrecmd) {
	int i;
	for (i = 0; commands[i].name; i++) {
		if (strcasecmp(nombrecmd,commands[i].name) == 0){
			return (&commands[i]);
		}
	}
	return ((COMMAND*)NULL);
}

char **completar_comandos(const char *text, int start, int end) {
  char **matches;

  matches = (char **)NULL;

  if (start == 0)
    matches = rl_completion_matches (text, command_generator);

  return (matches);
}

char *command_generator (const char *text, int state) {
  static int list_index, len;
  char *name;

  if (!state)
    {
      list_index = 0;
      len = strlen (text);
    }

  while ((name = commands[list_index].name)) {
      list_index++;

      if (strncasecmp(name, text, len) == 0)
        return (strdup(name));
    }

  return ((char *)NULL);
}

// Devuelve NULL y un error si se pasaron menos parametros de los esperados por la funcion
char** valid_argument (char* caller, char* arg)
{
  char **returnString = NULL;
  COMMAND *comandoEjecutar;
  comandoEjecutar = buscar_comando(caller);
  int nParams = 0;

  if (arg != NULL) {
    if (strlen(arg) != 0) {
	returnString = split_arg(arg);
/*      returnString = string_split(arg, " ");
        while (returnString[nParams] != NULL) {
	if (returnString[nParams][0] == '"') {
		int iPos = 1;
		while ((returnString[nParams][iPos] != '"') && (returnString[nParams][iPos] != '\0')) {
			returnString[nParams][iPos-1] = returnString[nParams][iPos];
			iPos++;
		}
		if (returnString[nParams][iPos] == '"') {
			returnString[nParams][iPos-1] = '\0';
		} else {
			returnString[nParams][iPos-1] = returnString[nParams][iPos];
		}
	}
	nParams++;
      }
*/
	while (returnString[nParams] != NULL)
		nParams++;
    }
  }

  if (nParams < comandoEjecutar->nParametros) {
	printf ("%s: No se pasaron parametros.\n", caller);
	if (returnString != NULL) {
		nParams = 0;
		while (returnString[nParams] != NULL) {
			free(returnString[nParams]);
			nParams++;
		}
		free(returnString);
	}
        returnString = NULL;
  } 

  return returnString;
}

char **split_arg(const char* arg) {
	char **returnString = malloc(sizeof(char*));
	int largoString = strlen(arg);
	int iPos = 0;
	int iPosArray = 0;
	int iLargoArray = 0;
	int offset = 0;
	bool quoteOpen = false;
	for (iPos = 0; iPos < largoString; iPos++) {
		if (arg[iPos] == '"') { 
			if (quoteOpen == false) {
				quoteOpen = true;
				offset++;
			} else {
				quoteOpen = false;
				returnString[iPosArray] = malloc(sizeof(char) * (iLargoArray+1));
				memcpy(returnString[iPosArray], arg + offset, iLargoArray);
				returnString[iPosArray][iLargoArray] = '\0';
				offset += iLargoArray + 1;
				iLargoArray = 0;
				iPosArray++;
				returnString = realloc(returnString, sizeof(char*) * (iPosArray+1));
			}
		} else {
			if ((arg[iPos] == ' ') && (quoteOpen == false)) {
				returnString[iPosArray] = malloc(sizeof(char) * (iLargoArray+1));
				memcpy(returnString[iPosArray], arg + offset, iLargoArray);
				returnString[iPosArray][iLargoArray] = '\0';
				offset += iLargoArray + 1;
				iLargoArray = 0;
				iPosArray++;
				returnString = realloc(returnString, sizeof(char*)*(iPosArray+1));
			} else {
				iLargoArray++;
			}
		}
	}
	if (iLargoArray != 0) {
		returnString[iPosArray] = malloc(sizeof(char) * (iLargoArray+1));
		memcpy(returnString[iPosArray], arg + offset, iLargoArray);
		returnString[iPosArray][iLargoArray] = '\0';
		iPosArray++;
		returnString = realloc(returnString, sizeof(char*)*(iPosArray+1));
	}
	returnString[iPosArray] = NULL;
	return returnString;
}

int com_list (char *arg) {
  char *syscom = malloc(100);
  if (!arg)
    arg = "";

  sprintf (syscom, "ls -FClg %s", arg);
  system (syscom);
  free(syscom);
  return 1;
}

int com_help (char *arg) {
  register int i;
  int printed = 0;

  char** args = valid_argument ("help", arg);
  if (args != NULL) {
 	for (i = 0; commands[i].name; i++)
	{
    		if (strcmp(args[0], commands[i].name) == 0)
      		{
      			if (strlen(commands[i].name) > 7) { 
				printf ("%s\t%s.\n", commands[i].name, commands[i].doc);
			} else {
				printf ("%s\t\t%s.\n", commands[i].name, commands[i].doc);
       			}
			printed++;
      		}
     	}
	printed = 0;
	while (args[printed] != NULL) {
		free(args[printed]);
		printed++;
	}
	free(args);
	return 1;
  } 

  if (!printed)
    {
      printf ("No se especifico un comando.  Las posibilidades son:\n");

      for (i = 0; commands[i].name; i++)
        {
          /* Immprime en varias columnas */
          if (printed == 4)
            {
              printed = 0;
              printf ("\n");
            }
	  if (strlen(commands[i].name) > 7) {
          	printf ("%s\t", commands[i].name);
          } else {
		printf ("%s\t\t", commands[i].name);
          }
	  printed++;
        }

      if (printed)
        printf ("\n");
    }
  free(arg);
  return 1;
}

int com_cd (char *arg) {
  if (chdir (arg) == -1)
    {
      return -1;
    }

  com_pwd (arg);
  return 1;
}

int com_pwd (char *arg) {
  char dir[1024], *s;

  s = getcwd (dir, sizeof(dir) - 1);
  if (s == 0)
    {
      printf ("Error al obtener el directorio actual: %s\n", dir);
      return -1;
    }

  printf ("El directorio actual es: %s\n", dir);
  free(arg);
  return 1;
}

int com_quit (char *arg) {
  return 0;
}

int com_run (char *arg) {
  char** args = valid_argument ("correr", arg);
  if (args == NULL)
	return -1;

  Consola_Comando_Correr(args[0]);
  int iPos = 0;
  while (args[iPos] != NULL) {
	free(args[iPos]);
	iPos++;
  }
  free(args);
  free(arg);
  return 1;
}

int com_kill (char *arg) {
  char** args = valid_argument ("finalizar", arg);
  if (args == NULL)
	return -1;

  Consola_Comando_Finalizar(args[0]);
  // Liberar args
  int iPos = 0;
  while (args[iPos] != NULL) {
	free(args[iPos]);
	iPos++;
  }
  free(args);
  free(arg);
  return 1;
}

int com_ps (char *arg) {
  Consola_Comando_PS();
  free(arg);
  return 1;
}

int com_cpu (char *arg) {
  Consola_Comando_CPU();
  free(arg);
  return 1;
}

int com_deadlock (char *arg) {
  Consola_Comando_DeadLockChecker();
  free(arg);
  return 1;
}

int com_statcpu (char *arg) {
  Consola_Comando_StatusCpu();
  free(arg); 
  return 1;
}

int com_statmproc (char *arg) {
  Consola_Comando_StatusMPro();
  free(arg);  
  return 1;
}
/*int leerConsola() {

	//Coloreado Para que sirva de Separador Entre comandos, asi es mas facil de distinguir Visualmente
	//printf(ANSI_COLOR_YELLOW ">Ingrese un comando [Correr / Finalizar / PS / CPU]: " ANSI_COLOR_RESET);

	int estaReconocidoComando = -1; //Los valores que va a tener son 1/0/-1 Siendo 1 y 0 Que el comando SI esta reconocido y -1 Que no lo esta. Por defecto se toma que no esta reconocido el comando

	char lineaLeida[MAXIMA_CANTIDAD_CARACTERES_POR_LEER];
	char *input[10];

	lineaLeida[0] = '\0'; // se asegura que si hacen input vacio no lea cualquier cosa 
	lineaLeida[sizeof(lineaLeida) - 1] = ~'\0'; // se asegura que el ultimo caracter marque que es un string (le pongo caracter de Fin de Cadena) 

	//Lee una Linea de la Consola (Terminal de Linux)
//	if(   fgets(lineaLeida, sizeof(lineaLeida), stdin) == NULL  ){
//		printf(ANSI_COLOR_RED "¡FFFUUUCK! Fallo el Fgets al Leer desde Consola. WTF!!!!" ANSI_COLOR_RESET "\n");
//		return estaReconocidoComando;
//	}
	char* tmpline = (char*) NULL;
	char* prompttext = string_from_format("%c" ANSI_COLOR_YELLOW "%c>Ingrese un comando [Correr / Finalizar / PS / CPU]: %c" ANSI_COLOR_RESET "%c", RL_PROMPT_START_IGNORE,RL_PROMPT_END_IGNORE, RL_PROMPT_START_IGNORE,RL_PROMPT_END_IGNORE);
	//tmpline = readline(ANSI_COLOR_YELLOW ">Ingrese un comando [Correr / Finalizar / PS / CPU]: " ANSI_COLOR_RESET );
	tmpline = readline(prompttext);
	if (*tmpline) {
		add_history(tmpline);
	}
	memcpy(lineaLeida, tmpline, strlen(tmpline));
	lineaLeida[strlen(tmpline)] = '\0';
	free(tmpline);

	int caracterActual = 0; //read counter
	int cantidadArgumentos = 0; //args counter
	int k = 0; //write counter
	bool estoyDentroArgumento = false; //

	//Cada Argumento debe tener un tamaño igual a toda la cantidad de caracteres por leer, ya que puede ser que haya 1 solo argumento gigante
	input[cantidadArgumentos] = malloc(sizeof(char) * MAXIMA_CANTIDAD_CARACTERES_POR_LEER); //Pido memoria para guardar la instruccion por separado de los argumentos
	char* currentPointer = input[cantidadArgumentos];

	for (caracterActual = 0; caracterActual < MAXIMA_CANTIDAD_CARACTERES_POR_LEER; caracterActual++) {
		if (lineaLeida[caracterActual] == '\n') {
			// cierro el currentPointer porque el usuario ya ingreso enter, despues del \n es dato basura, por eso hago break
			currentPointer[k] = '\0';
			break;
		}

		// Busco comillas, si tenia un argumento abierto, cierro el argumento, si estaba cerrado, cambio j,
		// abriendo un nuevo argumento, y reseteo k para buffear el nuevo argumento
		if (lineaLeida[caracterActual] == '\"') {
			if (!estoyDentroArgumento) {
				currentPointer[k] = '\0';
				cantidadArgumentos++;
				input[cantidadArgumentos] = malloc(sizeof(char) * MAXIMA_CANTIDAD_CARACTERES_POR_LEER);
				currentPointer = input[cantidadArgumentos];
				estoyDentroArgumento = true;
				k = 0;
			} else {
				estoyDentroArgumento = false;
				currentPointer[k] = '\0';
				k++;
			}
		} else {
			// Cuando el char es un espacio, si estoy adentro de un argumento, no lo ignoro, si estoy fuera, lo ignoro
			// (ejemplo espacio entre argumentos, ignoro, espacio adentro de una ruta, no lo ignoro)
			if (lineaLeida[caracterActual] == ' ') {
				if (estoyDentroArgumento) {
					currentPointer[k] = lineaLeida[caracterActual];
					k++;
				}
			} else {
				currentPointer[k] = lineaLeida[caracterActual];
				k++;
			}
		}
	};


	if (string_equals_ignore_case(input[0], "Correr")) {
		estaReconocidoComando = 1;
		if (cantidadArgumentos < 1) {
			printf(ANSI_COLOR_RED "Ingresar PATH de mPro a Correr" ANSI_COLOR_RESET "\n");
		} else {
			Consola_Comando_Correr(input[1]);
		}
	}

	if (string_equals_ignore_case(input[0], "Finalizar")) {
		estaReconocidoComando = 1;
		if (cantidadArgumentos < 1) {
			printf(ANSI_COLOR_RED "Ingresar PID de mPro a Finalizar" ANSI_COLOR_RESET "\n");
		} else {
			Consola_Comando_Finalizar(input[1]);
		}
	}

	if (string_equals_ignore_case(input[0] , "PS")) {
		estaReconocidoComando = 1;
		Consola_Comando_PS();

	}

	if (string_equals_ignore_case(input[0] , "CPU")) {
		estaReconocidoComando = 1;
		Consola_Comando_CPU();

	}

	if (string_equals_ignore_case(input[0] , "Salir")) {
		estaReconocidoComando = 0;
		Comun_Pantalla_Separador_Destacar("Cerrando Planificador");

	}

	if (string_equals_ignore_case(input[0] , "DeadLockChecker") || string_equals_ignore_case(input[0] , "DeadLock")) {
		estaReconocidoComando = 1;
		Consola_Comando_DeadLockChecker();
	}

	if (string_equals_ignore_case(input[0] , "StatusCpu") || string_equals_ignore_case(input[0] , "StatusCPUs")) {
		estaReconocidoComando = 1;
		Consola_Comando_StatusCpu();
	}

	if (string_equals_ignore_case(input[0] , "StatusMPro") || string_equals_ignore_case(input[0] , "StatusMPros")) {
		estaReconocidoComando = 1;
		Consola_Comando_StatusMPro();
	}


	//Pequeña Mejora para que los "Enter" No los Tome como Error, sino que haga varios \n para Separar. Muy Util, es como un "Clear" o "Ctrl + L"
	if (string_is_empty(input[0])) {
		estaReconocidoComando = 1;
		printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
	}

	// Libero la memoria del comando y los argumentos
	for (caracterActual = 0; caracterActual <= cantidadArgumentos; caracterActual++) {
		free(input[caracterActual]);
	};

	// Si no se reconocio el comando, se envia error
	if (estaReconocidoComando == -1) {
		printf(ANSI_COLOR_RED"El Comando Ingresado no es conocido" ANSI_COLOR_RESET "\n");
		Macro_ImprimirParaDebug("Comando Ingresado: %s", lineaLeida);
	}

	printf("\n");		//Para separar entre comandos

	return estaReconocidoComando;
}
*/

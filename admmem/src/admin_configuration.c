#include "../headers/admin_configuration.h"

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "../../bibliotecas/comun.h"
#include "../../bibliotecas/logs.h"
#include "../../lib/commons/config.h"
#include "../../lib/commons/string.h"

void cleanLogFile() {
	Comun_borrarArchivo(ARCHIVO_LOG);
}

bool validarConfiguracion() {
	int errorCode = Comun_existeArchivo(ADMIN_CONFIG_PATH);

	if (errorCode == -1) {
		Macro_Imprimir_Error("%s - No existe el archivo de configuracion - %s", __func__, ADMIN_CONFIG_PATH);
		return false;
	}

	t_config* archivoConfig = config_create(ADMIN_CONFIG_PATH);

	if (!config_has_property(archivoConfig, "PUERTO_ESCUCHA")) {
		Macro_Imprimir_Error("La configuracion no tiene PUERTO_ESCUCHA");
		errorCode = -1;
	}

	if (!config_has_property(archivoConfig, "IP_SWAP")) {
		Macro_Imprimir_Error("La configuracion no tiene IP_SWAP");
		errorCode = -1;
	}

	if (!config_has_property(archivoConfig, "PUERTO_SWAP")) {
		Macro_Imprimir_Error("La configuracion no tiene PUERTO_SWAP");
		errorCode = -1;
	}

	if (!config_has_property(archivoConfig, "MAXIMO_MARCOS_POR_PROCESO")) {
		Macro_Imprimir_Error("La configuracion no tiene MAXIMO_MARCOS_POR_PROCESO");
		errorCode = -1;
	}

	if (!config_has_property(archivoConfig, "CANTIDAD_MARCOS")) {
		Macro_Imprimir_Error("La configuracion no tiene CANTIDAD_MARCOS");
		errorCode = -1;
	}

	if (!config_has_property(archivoConfig, "TAMANIO_MARCO")) {
		Macro_Imprimir_Error("La configuracion no tiene TAMANIO_MARCO");
		errorCode = -1;
	}

	if (!config_has_property(archivoConfig, "ENTRADAS_TLB")) {
		Macro_Imprimir_Error("La configuracion no tiene ENTRADAS_TLB");
		errorCode = -1;
	}

	if (!config_has_property(archivoConfig, "TLB_HABILITADA")) {
		Macro_Imprimir_Error("La configuracion no tiene TLB_HABILITADA");
		errorCode = -1;
	}

	if (!config_has_property(archivoConfig, "RETARDO_MEMORIA")) {
		Macro_ImprimirParaDebug("La configuracion no tiene RETARDO_MEMORIA");
		errorCode = -1;
	}

	if (!config_has_property(archivoConfig, "ALGORITMO_REEMPLAZO")) {
		Macro_Imprimir_Error("La configuracion no tiene ALGORITMO_REEMPLAZO");
		errorCode = -1;
	}

	config_destroy(archivoConfig);
	if (errorCode == -1) {
		return false;
	}
	return true;
}

void conseguirPuertoAdmin(char portString[]) {
	t_config* archivoConfig = config_create(ADMIN_CONFIG_PATH);

	char *portAdmin;
	if (config_has_property(archivoConfig, "PUERTO_ESCUCHA")) {
		portAdmin = config_get_string_value(archivoConfig, "PUERTO_ESCUCHA");
	} else {
		//Control de Errores para que Explote al Usarlo con Funciones Sockets
		portAdmin = string_duplicate("-1");
	}

	Macro_ImprimirParaDebugConDatos("Puerto Escucha Puerto:%s\n", portAdmin);

	strcpy(portString, portAdmin);

	config_destroy(archivoConfig);
}

void conseguirIpPuertoSwap(char ip[], char port[]) {
	t_config* archivoConfig = config_create(ADMIN_CONFIG_PATH);

	char *ipString;
	if (config_has_property(archivoConfig, "IP_SWAP")) {
		ipString = config_get_string_value(archivoConfig, "IP_SWAP");
	} else {
		//Control de Errores para que Explote al Usarlo con Funciones Sockets
		ipString = string_duplicate("-1");
	}

	char *portString;
	if (config_has_property(archivoConfig, "PUERTO_SWAP")) {
		portString = config_get_string_value(archivoConfig, "PUERTO_SWAP");
	} else {
		//Control de Errores para que Explote al Usarlo con Funciones Sockets
		portString = string_duplicate("-1");
	}

	Macro_ImprimirParaDebugConDatos("SWAP  - IP:%s   Puerto:%s\n", ipString, portString);

	strcpy(ip, ipString);
	strcpy(port, portString);

	config_destroy(archivoConfig);
}

int traerTotalMarcos() {
	int cantidad = 0;

	t_config* archivoConfig = config_create(ADMIN_CONFIG_PATH);

	cantidad = config_get_int_value(archivoConfig, "CANTIDAD_MARCOS");

	config_destroy(archivoConfig);

	return cantidad;
}

int traerMaximoMarcosPorProceso() {
	int cantidad = 0;

	t_config* archivoConfig = config_create(ADMIN_CONFIG_PATH);

	cantidad = config_get_int_value(archivoConfig, "MAXIMO_MARCOS_POR_PROCESO");

	config_destroy(archivoConfig);

	return cantidad;
}

int traerTamanioMarcos() {
	int tamanio = 0;

	t_config* archivoConfig = config_create(ADMIN_CONFIG_PATH);

	tamanio = config_get_int_value(archivoConfig, "TAMANIO_MARCO");

	config_destroy(archivoConfig);

	return tamanio;
}

int traerTamanioTLB() {
	int tamanio = 3;

	t_config* archivoConfig = config_create(ADMIN_CONFIG_PATH);

	tamanio = config_get_int_value(archivoConfig, "ENTRADAS_TLB");

	config_destroy(archivoConfig);

	return tamanio;
}

bool isTLBon() {
	bool isOn = true;

	t_config* archivoConfig = config_create(ADMIN_CONFIG_PATH);

	char* string_isOn = config_get_string_value(archivoConfig, "TLB_HABILITADA");

	isOn = string_equals_ignore_case("TRUE", string_isOn);

	config_destroy(archivoConfig);

	return isOn;
}

double traerRetardoMemoria() {
	double retardo = 1;

	t_config* archivoConfig = config_create(ADMIN_CONFIG_PATH);

	char *value = config_get_string_value(archivoConfig, "RETARDO_MEMORIA");

	retardo = strtod(value, NULL);

	config_destroy(archivoConfig);

	return retardo;
}

void conseguirAlgoritmoReemplazo(char algoritmo[]) {
	t_config* archivoConfig = config_create(ADMIN_CONFIG_PATH);

	char *algoritmoReemplazo;
	if (config_has_property(archivoConfig, "ALGORITMO_REEMPLAZO")) {
		algoritmoReemplazo = config_get_string_value(archivoConfig, "ALGORITMO_REEMPLAZO");
	} else {
		algoritmoReemplazo = string_duplicate("FIFO");
	}

	strcpy(algoritmo, algoritmoReemplazo);

	config_destroy(archivoConfig);
}

int initRegularMutex(pthread_mutex_t** pointerToMutex) {
	*pointerToMutex = malloc(sizeof(pthread_mutex_t));
	if (*pointerToMutex == NULL) {
		return -1;
	}

	//pthread_mutexattr_t attr;
	//pthread_mutexattr_init(&attr);
	//pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

	//int numeroError = pthread_mutex_init(*pointerToMutex, &attr);
	int numeroError = pthread_mutex_init(*pointerToMutex, NULL);
	if (numeroError != 0) {
		return -1;
	}

	return 1;
}

int initSharedMutex(pthread_mutex_t** pointerToMutex) {

	/* mutex */
	/*    mutex_id = shm_open(MYMUTEX, O_CREAT | O_RDWR | O_TRUNC, S_IRWXU | S_IRWXG);
	 if (mutex_id < 0) {
	 perror("shm_open failed with " MYMUTEX);
	 return -1;
	 }
	 if (ftruncate(mutex_id, sizeof(pthread_mutex_t)) == -1) {
	 perror("ftruncate failed with " MYMUTEX);
	 return -1;
	 }
	 mutex = (pthread_mutex_t *)mmap(NULL, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED, mutex_id, 0);
	 if (mutex == MAP_FAILED) {
	 perror("mmap failed with " MYMUTEX);
	 return -1;
	 }
	 shm_unlink(MYMUTEX);
	 */

	*pointerToMutex = mmap(NULL, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if ((*pointerToMutex == NULL) || (*pointerToMutex == MAP_FAILED)) {
		return -1;
	}

	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	int numeroError = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
	if (numeroError != 0) {
		return -1;
	}

	numeroError = pthread_mutex_init(*pointerToMutex, &attr);
	if (numeroError != 0) {
		return -1;
	}

	return 1;
}
long long getCurrentLongTime() {
	struct timeval tv;

	gettimeofday(&tv, NULL);

	unsigned long long millisecondsSinceEpoch = (unsigned long long) (tv.tv_sec) * 1000
			+ (unsigned long long) (tv.tv_usec) / 1000;

	return millisecondsSinceEpoch;
}

void trace(const char* funcion, char* mensaje) {

	LOG_escribirPantalla(ARCHIVO_LOG, funcion, LOG_LEVEL_TRACE, mensaje);
}

void debug(const char* funcion, char* mensaje) {

	LOG_escribirPantalla(ARCHIVO_LOG, funcion, LOG_LEVEL_DEBUG, mensaje);
}

void info(const char* funcion, char* mensaje) {

	LOG_escribirPantalla(ARCHIVO_LOG, funcion, LOG_LEVEL_INFO, mensaje);
}

void error(const char* funcion, char* mensaje) {

	char* str = string_new();

	string_append(&str, mensaje);
	string_append(&str, " \n");
	string_append(&str, Macro_Obtener_Errno());

	LOG_escribirPantalla(ARCHIVO_LOG, funcion, LOG_LEVEL_ERROR, str);

	free(str);
}

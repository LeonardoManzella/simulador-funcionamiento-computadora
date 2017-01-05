#include "cpu_utiles.h"
#include "cpu_parametros_config.h"
#include "../bibliotecas/comun.h"
#include "../bibliotecas/logs.h"
#include "../lib/commons/string.h"
#include "../lib/commons/collections/list.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

int validar_config(t_config* config, t_cpu_config* cpuConfig);

t_cpu_config abrirArchivoConfig(char* path) {

	t_cpu_config cpuConfig;

	cpuConfig.esValida = true;

	//veo si existe el archivo de config
	if(-1 == Comun_existeArchivo(path)) {
		//error
		error(__func__, "El archivo de config no existe");
		cpuConfig.esValida = false;
	}

	//genero la config y la valido
	t_config* config = config_create(ARCHIVO_CONFIG);

	//valido la config
	if(-1 == validar_config(config, &cpuConfig)) {

		cpuConfig.esValida = false;
	}

	//destruyo config
	config_destroy(config);

	//devuelvo la config de cpu
	return cpuConfig;
}

/**
 * Valida los parámetros de configuración, en caso de error lo informo en log y devuelve error en caso de ser necesario
 */
int validar_config(t_config* config, t_cpu_config* cpuConfig) {

	if (!config_has_property(config, "PUERTO_ESCUCHA_CPU")) {

		error(__func__, "Error al obtener propiedad PUERTO_ESCUCHA_CPU del archivo de config");
		return -1;
	}

	strcpy(cpuConfig->puerto_escucha_cpu, config_get_string_value(config, "PUERTO_ESCUCHA_CPU"));


	if (!config_has_property(config, "IP_PLANIFICADOR")) {

		error(__func__, "Error al obtener propiedad IP_PLANIFICADOR del archivo de config");
		return -1;
	}

	strcpy(cpuConfig->ip_planificador, config_get_string_value(config, "IP_PLANIFICADOR"));

	if (!config_has_property(config, "PUERTO_PLANIFICADOR")) {

		error(__func__, "Error al obtener propiedad PUERTO_PLANIFICADOR del archivo de config");
		return -1;
	}

	strcpy(cpuConfig->puerto_planificador, config_get_string_value(config, "PUERTO_PLANIFICADOR"));

	if (!config_has_property(config, "IP_MEMORIA")) {

		error(__func__, "Error al obtener propiedad IP_MEMORIA del archivo de config");
		return -1;
	}

	strcpy(cpuConfig->ip_memoria, config_get_string_value(config, "IP_MEMORIA"));

	if (!config_has_property(config, "PUERTO_MEMORIA")) {

		error(__func__, "Error al obtener propiedad PUERTO_MEMORIA del archivo de config");
		return -1;
	}

	strcpy(cpuConfig->puerto_memoria, config_get_string_value(config, "PUERTO_MEMORIA"));

	if (!config_has_property(config, "CANTIDAD_HILOS")) {

		error(__func__, "Error al obtener propiedad CANTIDAD_HILOS del archivo de config");
		return -1;
	}

	cpuConfig->cantidad_hilos = config_get_int_value(config, "CANTIDAD_HILOS");

	if (!config_has_property(config, "RETARDO")) {

		error(__func__, "Error al obtener propiedad RETARDO del archivo de config");
		return -1;
	}

	cpuConfig->retardo = config_get_int_value(config, "RETARDO");

	//impresión por consola
	cpuConfig->imprimirPorConsola = true;

	if (config_has_property(config, "IMPRIMIR_POR_CONSOLA")) {

		if(string_equals_ignore_case(config_get_string_value(config, "IMPRIMIR_POR_CONSOLA"), "false")) {
			cpuConfig->imprimirPorConsola = false;
		}
	}

	return 0;
}

char* leer_mcod(char* archivo) {

	int result = -1;
	struct stat buf;
	int fileSize = -1;
	char* mCod = NULL;

	FILE* file = fopen(archivo, "r");

	if(NULL == file) {
		//error al abrir archivo
		error(__func__, string_from_format("error al abrir archivo: %s", archivo));
		return NULL;
	}

	//busco el tamaño del archivo
	result = fstat(fileno(file), &buf);

	if(-1 == result) {
		//error fstat
		error(__func__, string_from_format("error al realizar fstat para archivo: %s", archivo));
		return NULL;
	}

	fileSize = buf.st_size;

	mCod = malloc(fileSize + 1);

	if(NULL == mCod) {
		//error en malloc
		error(__func__, "error al realizar malloc");
		return NULL;
	}

	result = fread(mCod, 1, fileSize, file);

	if((result <= 0) || (ferror(file)) || (feof(file))) {
		//error al leer archivo
		error(__func__, string_from_format("error al intentar leer del archivo: %s", archivo));
		return NULL;
	}

	//cierro mcod
	result = fclose(file);

	if(result != 0) {
		//error al cerrar
		error(__func__, string_from_format("error al cerrar el archivo mCod: %s. Sigo...", archivo));
	}

	mCod[fileSize] = '\0';

	trace(__func__, string_from_format("se leyo el siguiente mCod: [%s]", mCod));

	return mCod;
}

void freeArgsArray(char** argsArray, int argsCount) {

	int i = 0;

	for(i = 0; i <= argsCount; i++) {

		freeResource(argsArray[i]);
	}
}

void freeResource(void* resource) {

	if(NULL != resource) {
		free(resource);
		resource = NULL;
	}
}

void trimLeftWhitespaces(char** text) {

	char *string_without_whitespaces = *text;

	while (isspace(*string_without_whitespaces)) {

		string_without_whitespaces++;
	}

	char *new_string = string_duplicate(string_without_whitespaces);

	freeResource(*text);
	*text = new_string;
}

void printListContent(t_list* list) {

	int i = 0;

	for(i=0; i < list_size(list); i++) {

		trace(__func__, string_from_format("elemento %d : [%s]", i, list_get(list, i)));
	}
}

char* formatWriteText(char* text) {

	if(NULL == text) {
		//nop
		warn(__func__, "se recibio un texto nulo");
		return NULL;
	}

	//aloco un temporal para reconstruir el string, probablemente venga sin caracter de fin por lo que uso buffer size
	char* tempStr = malloc(STRING_BUFFER_SIZE);

	strcpy(tempStr, text);

	char* result = malloc(strlen(tempStr) + 1);

	if(NULL == result) {
		//error en malloc
		error(__func__, "error al reservar memoria.");
		return NULL;
	}

	int size = 0;

	if(string_starts_with(tempStr, "\"")) {
		//empieza con comillas
		if(string_ends_with(tempStr, "\"")) {
			//termina con comillas
			size = strlen(tempStr + 1) - 1;
			strncpy(result, tempStr + 1, size);
			result[size] = '\0';
		}else{
			//no termina con comillas
			size = strlen(tempStr + 1);
			strcpy(result, tempStr + 1);
			result[size] = '\0';
		}
	}else{
		//no había comillas, devuelvo el texto original
		freeResource(result);
		return tempStr;
	}

	return result;
}

t_list* list_take_last(t_list* self, int count) {

	if(list_size(self) < count) {
		//faltan registros
		warn(__func__, "La lista tenía menos elementos que los pedidos, devuelvo la lista directamente.");

		return self;
	}

	int i = 0;
	t_list* sublist = list_create();

	for (i = count; i > 0; i--) {
		void* element = list_get(self, i);
		list_add(sublist, element);
	}

	return sublist;
}

t_list* process_cpu_usage_array(int array[][CPU_USAGE_TIME_CALC_PARAM], int cantCPUs) {

	int i, j = 0;
	int totalUsage = 0;
	int valuesCounter = 0;
	t_uso_CPU* element;
	t_list* result = list_create();

	for(i = 0; i < cantCPUs; i++) {

		totalUsage = 0;
		element = malloc(sizeof(t_uso_CPU));

		if(NULL == element) {

			warn(__func__, "error en malloc para calcular estadistica.. ignoro este cpu");
		}else{

			for(j = 0; j < CPU_USAGE_TIME_CALC_PARAM;  j++) {

				if(-1 == array[i][j]) {
					//valor no utilizado aún... lo ignoro
				}else{
					valuesCounter++;
					totalUsage = array[i][j] + totalUsage;
				}


			}

			element->id_cpu = i + 1;

			if(0 != totalUsage && 0 != valuesCounter) {

				element->tiempo_uso = totalUsage * 100 / valuesCounter;
			}else{

				element->tiempo_uso = 0;
			}

			list_add(result, (void*) element);
			//warn(__func__, string_from_format("cpu %d, uso %d\%", element->id_cpu, element->tiempo_uso));
		}
	}

	return result;
}

void destroy_statistic(t_uso_CPU* statistic) {

	freeResource(statistic);
}

void trace(const char* funcion, char* mensaje) {

	LOG_escribirDebugPantalla(ARCHIVO_LOG, funcion, LOG_LEVEL_TRACE, mensaje);
}

void debug(const char* funcion, char* mensaje) {

	LOG_escribirDebugPantalla(ARCHIVO_LOG, funcion, LOG_LEVEL_DEBUG, mensaje);
}

//void info(const char* funcion, char* mensaje) {
//
//	LOG_escribirDebugPantalla(ARCHIVO_LOG, funcion, LOG_LEVEL_INFO, mensaje);
//}

void info(const char* funcion, char* mensaje, bool imprimirPorConsola) {

	if(imprimirPorConsola) {

		fprintf(stdout, ANSI_COLOR_GREEN "[%s] %s" ANSI_COLOR_RESET "\n", funcion, mensaje);
		LOG_escribir(ARCHIVO_LOG, funcion, LOG_LEVEL_INFO, mensaje);
	}else {

		LOG_escribirDebugPantalla(ARCHIVO_LOG, funcion, LOG_LEVEL_INFO, mensaje);
	}
}

void warn(const char* funcion, char* mensaje) {

	LOG_escribirDebugPantalla(ARCHIVO_LOG, funcion, LOG_LEVEL_WARNING, mensaje);
}

void error(const char* funcion, char* mensaje) {

	char* str = string_new();

	string_append(&str, mensaje);
	string_append(&str, " \n");
	string_append(&str, Macro_Obtener_Errno());

	LOG_escribirDebugPantalla(ARCHIVO_LOG, funcion, LOG_LEVEL_ERROR, str);

	freeResource(str);
}

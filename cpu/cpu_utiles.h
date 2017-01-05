/**
 * Funciones generales para el módulo cpu
 */
#ifndef CPU_UTILS_H__
#define CPU_UTILS_H__

#include "../lib/commons/config.h"
#include "cpu_parametros_config.h"
#include "../lib/commons/collections/list.h"
#include "../bibliotecas/structs.h"

/*
 * Función que abre el archivo de config, lo valida y llena el struct
 */
t_cpu_config abrirArchivoConfig(char* config_path);

/*
 * Función que lee el código mCod del path indicado y lo devuelve como un char*
 */
char* leer_mcod(char* archivo);

/*
 * Función que libera los recursos utilizados al generar un array de args (lo uso para parseo de instrucciones)
 */
void freeArgsArray(char** argsArray, int argsCount);

/*
 * Función que libera un recurso y lo setea en NULL para evitar double frees
 */
void freeResource(void* resource);

/*
 * Función que remueve todos los white-space chars al principio de un string (incluye caracteres de enter, etc)
 */
void trimLeftWhitespaces(char** str);

/*
 * Función que imprime el contenido de una lista de chars hasta encontrar un null
 */
void printListContent(t_list* list);

/*
 * Función que agarra los últimos elementos de una lista y devuelve otra con los mismos.
 * En caso de que la lista tenga menos elementos que los pedidos, se devuelve la lista.
 */
t_list* list_take_last(t_list* self, int count);

/*
 * Función que recibe un array de consumo de cpu y genera un listado de t_uso_CPU
 */
t_list* process_cpu_usage_array(int array[][CPU_USAGE_TIME_CALC_PARAM], int cantCPUs);

/*
 * Función que busca '"' al principio y al final del string recibido y devuelve una copia sin los mismos
 */
char* formatWriteText(char* text);

/**
 * Función que elimina un registro de estadísticas t_uso_CPU
 */
void destroy_statistic(t_uso_CPU* statistic);

/*
 * Funciones para simplificar logueo
 */
void trace(const char* funcion, char* mensaje);

void debug(const char* funcion, char* mensaje);

//void info(const char* funcion, char* mensaje);

//info alternativo para imprimir por consola en release
void info(const char* funcion, char* mensaje, bool imprimirPorConsola);

void warn(const char* funcion, char* mensaje);

void error(const char* funcion, char* mensaje);

#endif

/*
 * swap_log.c
 *
 *  Created on: 24/10/2015
 *      Author: utnso
 */

#include <stdlib.h>
#include "swap_log.h"
#include "../bibliotecas/logs.h"
#include "../bibliotecas/comun.h"
#include "../lib/commons/string.h"


void trace(const char* funcion, char* mensaje) {

	LOG_escribirDebugPantalla(ARCHIVO_LOG, funcion, LOG_LEVEL_TRACE, mensaje);
}

void debug(const char* funcion, char* mensaje) {

	LOG_escribirDebugPantalla(ARCHIVO_LOG, funcion, LOG_LEVEL_DEBUG, mensaje);
}

void info(const char* funcion, char* mensaje) {

	LOG_escribirPantalla(ARCHIVO_LOG, funcion, LOG_LEVEL_INFO, mensaje);
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

	if(str != NULL) {
		free(str);
	}
}

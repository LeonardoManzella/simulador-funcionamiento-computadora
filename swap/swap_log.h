/*
 * swap_log.h
 *
 *  Created on: 24/10/2015
 *      Author: utnso
 */

#ifndef SWAP_SWAP_LOG_H_
#define SWAP_SWAP_LOG_H_

#define ARCHIVO_LOG "logSwap.html"

/*
 * Funciones para simplificar logueo
 */
void trace(const char* funcion, char* mensaje);

void debug(const char* funcion, char* mensaje);

void info(const char* funcion, char* mensaje);

void warn(const char* funcion, char* mensaje);

void error(const char* funcion, char* mensaje);

#endif /* SWAP_SWAP_LOG_H_ */

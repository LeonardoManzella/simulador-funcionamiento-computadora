/*
 * admin_configuration.h
 *
 *  Created on: 3/9/2015
 *      Author: utnso
 */

#ifndef HEADERS_ADMIN_CONFIGURATION_H_
#define HEADERS_ADMIN_CONFIGURATION_H_

#include <pthread.h>
#include <stdbool.h>

#define ARCHIVO_LOG 			"logAdmin.html"
#define ADMIN_CONFIG_PATH 		"config_admin.cfg"

//Limpia el archivo de logs
void cleanLogFile();

//Se encarga de validar que exista un archivo de configuracion y su contenido
bool validarConfiguracion();

//trae el puerto de escucha del administrador de memoria("PUERTO_ESCUCHA") de la configuracion
void conseguirPuertoAdmin(char portString[]);

//trae el puerto y la ip de swap desde la configuracion ("PUERTO_SWAP" & "IP_SWAP")
void conseguirIpPuertoSwap(char ip[], char port[]);

//trae el total de marcos desde la configuracion("CANTIDAD_MARCOS")
int traerTotalMarcos();

//trae el tamanio maximo de marcos que puede tener asignado un proceso("MAXIMO_MARCOS_POR_PROCESO")
int traerMaximoMarcosPorProceso();

//trae el tamanio de cada marco("TAMANIO_MARCO")
int traerTamanioMarcos();

//trae la cantidad de entradas de la TLB("ENTRADAS_TLB")
int traerTamanioTLB();

//trae si la tlb esta activada o no("TLB_HABILITADA")
bool isTLBon();

//trae el delay que tiene el acceso a memoria cuando no accede por la TLB ("RETARDO_MEMORIA")
double traerRetardoMemoria();

//inicializa un mutex. Devuelve 1 si se inicio, -1 si no se inicio
int initRegularMutex(pthread_mutex_t** pointerToMutex);

int initSharedMutex(pthread_mutex_t** pointerToMutex);

long long getCurrentLongTime();

void debug(const char* funcion, char* mensaje);

void info(const char* funcion, char* mensaje);

void error(const char* funcion, char* mensaje);

void conseguirAlgoritmoReemplazo(char algoritmo[]);

#endif /* HEADERS_ADMIN_CONFIGURATION_H_ */

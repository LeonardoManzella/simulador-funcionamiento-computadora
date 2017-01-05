/**
 * Header que define todos los parámetros de configuración para cpu
 */
#ifndef CPU_PARAMS_H__
#define CPU_PARAMS_H__

#include <stdint.h>
#include <stdbool.h>
#include "../bibliotecas/comun.h"

#define ARCHIVO_LOG "logCPU.html"
#define ARCHIVO_CONFIG "cpu.cfg"
#define INSTRUCTION_SEPARATOR " "
#define CPU_USAGE_TIME_CALC_PARAM 60 //tiempo para calcular las estadísticas
#define CPU_USAGE_SUPER_LIST_CLEANING_TIME 120 //tiempo a esperar para limpiar la lista de estadísticas de uso de cpu
/*
 * defino cantidad de argumentos para parsear en cada instrucción
 */
#define ARGUMENTOS_INST_INICIAR 1
#define ARGUMENTOS_INST_LEER 1
#define ARGUMENTOS_INST_ESCRIBIR 2
#define ARGUMENTOS_INST_ES 1
/*
 * defino códigos de error para ejecución de instrucciones
 */
#define CPU_FINALIZAR_PROCESO -100
#define CPU_BLOQUEO_ES -101
#define CPU_ERROR_EJECUCION -102
#define CPU_ERROR_INST_NO_ENCONTRADA -103

/*
 * Configuración de cpu - a llenar en base a un archivo .cfg
 */
typedef struct {

	char		puerto_escucha_cpu[LONGITUD_CHAR_PUERTOS];
	char		ip_planificador[LONGITUD_CHAR_IP];
	char		puerto_planificador[LONGITUD_CHAR_PUERTOS];
	char		ip_memoria[LONGITUD_CHAR_IP];
	char		puerto_memoria[LONGITUD_CHAR_PUERTOS];
	uint32_t	cantidad_hilos;
	uint32_t	retardo;									//tiempo a esperar al terminar de ejecutar cada instrucción
	bool		esValida;									//boolean para indicar si la validación de la config fue correcta o no
	bool		imprimirPorConsola;							//boolean para indicar si se imprimen mensajes de información por consola o no

} t_cpu_config;

typedef struct {

	uint32_t	nroCPU;
	bool		disponible;

} t_cpu_thread;

#endif

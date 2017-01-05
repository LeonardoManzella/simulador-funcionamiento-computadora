/*
 * Biblioteca_Serializacion.h
 *
 *  Created on: 06/08/2015
 *      Author: utnso
 */

#ifndef BIBLIOTECA_SERIALIZACION_H_
#define BIBLIOTECA_SERIALIZACION_H_

#include <stdbool.h>
#include <stdint.h>
#include "structs.h"

//#define TAMANIO_ORDEN 35
#define TAMANIO_ORDER_HEADER sizeof(t_order_header)

/**
*	Enum que define el emisor del mensaje
*/
typedef enum {
	PLANIFICADOR 	= 1001,
	CPU 			= 1002,
	ADMMEM 		= 1003,
	SWAP 			= 1004
} t_sender_id;

typedef enum {
	CONECTAR_CPU,
	CONECTAR_CPU_ADMIN,
	EJECUTAR_RAFAGA,
	FIN_CON_ERROR,
	FIN_NORMAL,
	FIN_RAFAGA,
	BLOQUEO_ES,
	USO_CPU,
	INICIAR_PROCESO,
	LEER_FRAME,
	ESCRIBIR_FRAME,
	CONTENIDO_FRAME,
	FRAMES_OCUPADOS,
	FINALIZAR_PROCESO,
	OK,
	ERROR,
	SEG_FAULT
} t_comando;
/**
*	Estructura que define el header del paquete
*/
typedef struct __attribute__ ((__packed__)) __t_order_header {
	t_sender_id sender_id;  //identificador del emisor del mensaje
//	char order[TAMANIO_ORDEN]; //string que define la orden o acción a realizar. La debe interpretar el receptor del paquete.
	t_comando comando;
} t_order_header;


uint32_t SerializarString(char** resultado, uint32_t* tamanioResultado, void* string);
uint32_t DeSerializarString(const char* datosSerializados, uint32_t sizeDatosSerializados, void** resultado);
uint32_t Serializar_USOCPU(char** resultado, uint32_t* tamanioResultado, void* datos);
uint32_t DeSerializar_USOCPU(const char* datosSerializados, uint32_t sizeDatosSerializados, void** resultado);
uint32_t Serializar_PCB(char** resultado, uint32_t* tamanioResultado, void* datos);
uint32_t DeSerializar_PCB(const char* datosSerializados, uint32_t sizeDatosSerializados, void** resultado);
uint32_t Serializar_MemSWAP_IniciarProceso(char** resultado, uint32_t* tamanioResultado, void* datosProceso);
uint32_t DeSerializar_MemSWAP_IniciarProceso(const char* datosSerializados, uint32_t sizeDatosSerializados, void** resultado);
uint32_t Serializar_MemSWAP_ContenidoFrame(char** resultado, uint32_t* tamanioResultado, void* string);
uint32_t DeSerializar_MemSWAP_ContenidoFrame(const char* datosSerializados, uint32_t sizeDatosSerializados, void** resultado);
uint32_t Serializar_MemSWAP_Frame(char** resultado, uint32_t* tamanioResultado, void* datosProceso);
uint32_t DeSerializar_MemSWAP_Frame(const char* datosSerializados, uint32_t sizeDatosSerializados, void** resultado);
uint32_t Serializar_MemSWAP_FinalizarProceso(char** resultado, uint32_t* tamanioResultado, void* datos);
uint32_t DeSerializar_MemSWAP_FinalizarProceso (const char* datosSerializados, uint32_t sizeDatosSerializados, void** resultado);

//Des/Serializador de una Lista de las commons que continen elementos char* (los char* pueden no ser todos del mismo tamaño)
uint32_t Serializar_Lista_Resultados_Rafaga(char** resultado, uint32_t* tamanioResultado, void* datos);
uint32_t DeSerializar_Lista_Resultados_Rafaga(const char* datosSerializados, uint32_t sizeDatosSerializados, void** resultado);

//Des/Serializador del struct "t_rafaga" (en structs.h).
uint32_t Serializar_Resultado_Rafaga(char** resultado, uint32_t* tamanioResultado, void* datos);
uint32_t DeSerializar_Resultado_Rafaga(const char* datosSerializados, uint32_t sizeDatosSerializados, void** resultado);

#endif /* BIBLIOTECA_SERIALIZACION_H_ */

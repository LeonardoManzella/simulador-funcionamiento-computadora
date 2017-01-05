#ifndef BIBLIOTECA_STRUCTS_H_
#define BIBLIOTECA_STRUCTS_H_

#include "../lib/commons/collections/list.h"
#include <stdbool.h>
#include <pthread.h>
#include <stdint.h>

// Variable que define el largo predetermiando de un campo de texto
#define STRING_BUFFER_SIZE 256

typedef struct __attribute__ ((__packed__)) __t_uso_CPU {
	uint32_t id_cpu;
	uint32_t tiempo_uso;
} t_uso_CPU;

typedef struct __attribute__ ((__packed__)) __t_PCB {
	uint32_t 	id; 					// Id del Proceso
	char 		archivomProc[1024]; 	// Ruta y nombre del archivo a ejecutar
	uint32_t 	PC; 					// Program Counter o Instruction Pointer
	uint32_t 	InstruccionesRafaga; 	// Cantidad de instrucciones que ejecuta una rafaga
} t_PCB;


typedef struct __attribute__ ((__packed__)) __t_rafaga {
	t_PCB		pcb;			//PCB del mPro que finaliza su rafaga (sea por la razon que sea)
	uint32_t	numeroCPU;		//Numero del CPU que acaba de terminar de ejecutar esa rafaga del mPro (por ende ahora estara libre)
} t_rafaga;


//pagina en memoria y swap
typedef struct __attribute__ ((__packed__)) {
	int idProceso;
	int idPagina;
	int marco;
	bool modificada;
	bool prescencia;
	bool usado; // para algoritmo Clock-M
} t_pagina;

//struct para el manejo de procesos en admin de memoria y swap
typedef struct __attribute__ ((__packed__)) {
	int idProceso;
	int cantidadPaginasTotales;//Lo uso solo en admin de memoria
	int accesosASwap;
	int cantidadFallosDePagina;
	t_list* paginas;
	t_list* puntero_frames; // Para FIFO y Clock-M
} t_proceso_memoria;

#endif /* BIBLIOTECA_SERIALIZACION_H_ */

#include "serializacion.h"
#include <string.h>
#include <stdlib.h>
#include "../lib/commons/collections/list.h"
#include "structs.h"

uint32_t SerializarString(char** resultado, uint32_t* tamanioResultado, void* string) {
	*tamanioResultado = strlen(string) + 1;
	*resultado = malloc(sizeof(char) * (*tamanioResultado));
	memcpy(*resultado, string, *tamanioResultado);
	return 1;
}

uint32_t DeSerializarString(const char* datosSerializados, uint32_t sizeDatosSerializados, void** resultado) {
	*resultado = malloc(sizeof(char) * sizeDatosSerializados);
	memcpy(*resultado, datosSerializados, sizeDatosSerializados);
	return 1;
}

uint32_t Serializar_USOCPU(char** resultado, uint32_t* tamanioResultado, void* datos) {
	// Variables para despues devolver los resultados
	int tamanioStringFinal = 0;
	char *Buffer = NULL;
	int offset = 0;
	void *PunteroLista;
	t_uso_CPU *punteroDatos;

	// casteo el tipo void a una t_list para evitar errores
	t_list *listaDatos;
	listaDatos = (t_list *) datos;

	//Verifico que la lista no esta vacia
	//list_is_empty(listaDatos);

	// Calculo la cantidad total de elementos de la lista y asigno los espacios de memoria y variables
	int iCantElementos = list_size(listaDatos);
	tamanioStringFinal = iCantElementos * sizeof(t_uso_CPU);
	Buffer = malloc(sizeof(char) * tamanioStringFinal);
	int iElementoActual = 0;

	// Voy completando el string que voy a enviar
	for (iElementoActual = 0; iElementoActual < iCantElementos; iElementoActual++) {
		PunteroLista = list_get(listaDatos, iElementoActual);
		punteroDatos = (t_uso_CPU*) PunteroLista;
		memcpy(Buffer + offset, &punteroDatos->id_cpu, sizeof(punteroDatos->id_cpu));
		offset += sizeof(punteroDatos->id_cpu);
		memcpy(Buffer + offset, &punteroDatos->tiempo_uso, sizeof(punteroDatos->tiempo_uso));
		offset += sizeof(punteroDatos->tiempo_uso);
	}

	*tamanioResultado = tamanioStringFinal;
	*resultado = Buffer;
	return 1;
}

uint32_t DeSerializar_USOCPU(const char* datosSerializados, uint32_t sizeDatosSerializados, void** resultado) {
	t_list* listaResultado = list_create();

	// TODO: Verificar que al menos exista un dato;
	int cantElementos = sizeDatosSerializados / sizeof(t_uso_CPU);
	int iElementoActual;
	int offset = 0;
	t_uso_CPU *punteroDatos;

	for (iElementoActual = 0; iElementoActual < cantElementos; iElementoActual++) {
		punteroDatos = malloc(sizeof(t_uso_CPU));
		memcpy(punteroDatos, datosSerializados + offset, sizeof(t_uso_CPU));
		offset += sizeof(t_uso_CPU);
		list_add(listaResultado, punteroDatos);
	}
	*resultado = listaResultado;
	return 1;
}

uint32_t Serializar_PCB(char** resultado, uint32_t* tamanioResultado, void* datos) {
	// Variables para despues devolver los resultados
	int tamanioStringFinal = 0;
	char *Buffer = NULL;
	int offset = 0;
	t_PCB *punteroDatos = (t_PCB*) datos;

	tamanioStringFinal = sizeof(t_PCB);
	Buffer = malloc(sizeof(char) * tamanioStringFinal);

	// Voy completando el string que voy a enviar
	memcpy(Buffer + offset, &punteroDatos->id, sizeof(punteroDatos->id));
	offset += sizeof(punteroDatos->id);
	memcpy(Buffer + offset, punteroDatos->archivomProc, sizeof(punteroDatos->archivomProc));
	offset += sizeof(punteroDatos->archivomProc);
	memcpy(Buffer + offset, &punteroDatos->PC, sizeof(punteroDatos->PC));
	offset += sizeof(punteroDatos->PC);
	memcpy(Buffer + offset, &punteroDatos->InstruccionesRafaga, sizeof(punteroDatos->InstruccionesRafaga));
	offset += sizeof(punteroDatos->InstruccionesRafaga);

	*tamanioResultado = tamanioStringFinal;
	*resultado = Buffer;
	return 1;
}

uint32_t DeSerializar_PCB(const char* datosSerializados, uint32_t sizeDatosSerializados, void** resultado) {

	// TODO: Verificar que los tamanos correspondan;
	t_PCB *punteroDatos;

	punteroDatos = malloc(sizeof(t_PCB));
	memcpy(punteroDatos, datosSerializados, sizeof(t_PCB));

	*resultado = punteroDatos;
	return 1;
}

uint32_t Serializar_MemSWAP_IniciarProceso(char** resultado, uint32_t* tamanioResultado, void* datos) {

	int tamanioStringFinal = 0;
	char *buffer = NULL;
	int offset = 0;

	t_proceso_memoria *datosProceso = (t_proceso_memoria*) datos;

	tamanioStringFinal = sizeof(datosProceso->cantidadPaginasTotales) + sizeof(datosProceso->idProceso);
	buffer = malloc(sizeof(char) * tamanioStringFinal);

	memcpy(buffer + offset, &datosProceso->idProceso, sizeof(datosProceso->idProceso));
	offset += sizeof(datosProceso->idProceso);
	memcpy(buffer + offset, &datosProceso->cantidadPaginasTotales, sizeof(datosProceso->cantidadPaginasTotales));
	offset += sizeof(datosProceso->cantidadPaginasTotales);

	*tamanioResultado = tamanioStringFinal;
	*resultado = buffer;
	return 1;
}

uint32_t DeSerializar_MemSWAP_IniciarProceso(const char* datosSerializados, uint32_t sizeDatosSerializados,
		void** resultado) {

	int offset = 0;

	// Inicializo en NULL la lista de paginas ya que no se utiliza en este caso
	t_proceso_memoria *datosProceso = NULL;

	datosProceso = malloc(sizeof(t_proceso_memoria));

	datosProceso->paginas = NULL;

	memcpy(&datosProceso->idProceso, datosSerializados + offset, sizeof(datosProceso->idProceso));
	offset += sizeof(datosProceso->idProceso);
	memcpy(&datosProceso->cantidadPaginasTotales, datosSerializados + offset, sizeof(datosProceso->cantidadPaginasTotales));
	offset += sizeof(datosProceso->cantidadPaginasTotales);

	*resultado = datosProceso;
	return 1;
}

uint32_t Serializar_MemSWAP_FinalizarProceso(char** resultado, uint32_t* tamanioResultado, void* datos) {

	int tamanioStringFinal = 0;
	char *buffer = NULL;

	t_proceso_memoria *datosProceso = (t_proceso_memoria*) datos;

	tamanioStringFinal = sizeof(datosProceso->idProceso);
	buffer = malloc(sizeof(char) * tamanioStringFinal);

	memcpy(buffer, &datosProceso->idProceso, sizeof(datosProceso->idProceso));

	*tamanioResultado = tamanioStringFinal;
	*resultado = buffer;
	return 1;
}

uint32_t DeSerializar_MemSWAP_FinalizarProceso (const char* datosSerializados, uint32_t sizeDatosSerializados,
		void** resultado) {

	// Inicializo en NULL la lista de paginas ya que no se utiliza en este caso
	t_proceso_memoria *datosProceso = NULL;

	datosProceso = malloc(sizeof(t_proceso_memoria));

	datosProceso->paginas = NULL;
	datosProceso->cantidadPaginasTotales = 0;

	memcpy(&datosProceso->idProceso, datosSerializados, sizeof(datosProceso->idProceso));

	*resultado = datosProceso;
	return 1;
}

uint32_t Serializar_MemSWAP_ContenidoFrame(char** resultado, uint32_t* tamanioResultado, void* string) {
	*tamanioResultado = strlen(string) + 1;
	*resultado = malloc(sizeof(char) * (*tamanioResultado));
	memcpy(*resultado, string, *tamanioResultado);
	return 1;
}

uint32_t DeSerializar_MemSWAP_ContenidoFrame(const char* datosSerializados, uint32_t sizeDatosSerializados,
		void** resultado) {
	*resultado = malloc(sizeof(char) * sizeDatosSerializados);
	memcpy(*resultado, datosSerializados, sizeDatosSerializados);
	return 1;
}

uint32_t Serializar_MemSWAP_Frame(char** resultado, uint32_t* tamanioResultado, void* datos) {

	int tamanioStringFinal = 0;
	char *buffer = NULL;
	int offset = 0;
	t_pagina *datosPagina = (t_pagina *) datos;

	tamanioStringFinal = sizeof(t_pagina);
	buffer = malloc(sizeof(char) * (sizeof(datosPagina->idProceso) + sizeof(datosPagina->idPagina)));

	memcpy(buffer + offset, &datosPagina->idProceso, sizeof(datosPagina->idProceso));
	offset += sizeof(datosPagina->idProceso);
	memcpy(buffer + offset, &datosPagina->idPagina, sizeof(datosPagina->idPagina));
	offset += sizeof(datosPagina->idPagina);

	*tamanioResultado = tamanioStringFinal;
	*resultado = buffer;
	return 1;
}

uint32_t DeSerializar_MemSWAP_Frame(const char* datosSerializados, uint32_t sizeDatosSerializados, void** resultado) {

	int offset = 0;

	t_pagina *datosPagina = NULL;

	datosPagina = malloc(sizeof(t_pagina));

	memcpy(&datosPagina->idProceso, datosSerializados + offset, sizeof(datosPagina->idProceso));
	offset += sizeof(datosPagina->idProceso);
	memcpy(&datosPagina->idPagina, datosSerializados + offset, sizeof(datosPagina->idPagina));
	offset += sizeof(datosPagina->idPagina);

	*resultado = datosPagina;
	return 1;
}

uint32_t Serializar_Lista_Resultados_Rafaga(char** resultado, uint32_t* tamanioResultado, void* datos) {
	// Variables para despues devolver los resultados
	int tamanioStringFinal = 0;
	char *Buffer = NULL;
	int offset = 0;
	void *PunteroLista;
	char *punteroDatos;

	// casteo el tipo void a una t_list para evitar errores
	t_list *listaDatos;
	listaDatos = (t_list *) datos;

	//Verifico que la lista no esta vacia
	if (list_is_empty(listaDatos) == 1) {
		Buffer = malloc(sizeof(char));
		tamanioStringFinal = 1;
		memset(Buffer, '\0', 1);
		*tamanioResultado = tamanioStringFinal;
		*resultado = Buffer;
		return 1;
	}

	// Calculo la cantidad total de elementos de la lista y asigno los espacios de memoria y variables
	int iCantElementos = list_size(listaDatos);
	tamanioStringFinal = iCantElementos * STRING_BUFFER_SIZE;
	Buffer = malloc(sizeof(char) * tamanioStringFinal);
	int iElementoActual = 0;

	// Voy completando el string que voy a enviar
	for (iElementoActual = 0; iElementoActual < iCantElementos; iElementoActual++) {
		PunteroLista = list_get(listaDatos, iElementoActual);
		punteroDatos = (char*) PunteroLista;
		memcpy(Buffer + offset, punteroDatos, STRING_BUFFER_SIZE);
		offset += STRING_BUFFER_SIZE;
	}

	*tamanioResultado = tamanioStringFinal;
	*resultado = Buffer;
	return 1;
}

uint32_t DeSerializar_Lista_Resultados_Rafaga(const char* datosSerializados, uint32_t sizeDatosSerializados, void** resultado) {
	t_list* listaResultado = list_create();

	// Verifico que al menos exista un dato;
	if (sizeDatosSerializados >= STRING_BUFFER_SIZE) {
		int cantElementos = sizeDatosSerializados / STRING_BUFFER_SIZE;
		int iElementoActual;
		int offset = 0;
		char *punteroDatos;

		for (iElementoActual = 0; iElementoActual < cantElementos; iElementoActual++) {
			punteroDatos = malloc(STRING_BUFFER_SIZE);
			memcpy(punteroDatos, datosSerializados + offset, STRING_BUFFER_SIZE);
			offset += STRING_BUFFER_SIZE;
			list_add(listaResultado, punteroDatos);
		}
	} else {
		//Verifico que la lista este en 0
		if (sizeDatosSerializados < 1) {
			list_destroy(listaResultado);
			return 0;
		}
	}
	*resultado = listaResultado;
	return 1;
}

uint32_t Serializar_Resultado_Rafaga(char** resultado, uint32_t* tamanioResultado, void* datos) {

	int tamanioStringFinal = 0;
	char *buffer = NULL;
	int offset = 0;
	t_rafaga *datosRafaga = (t_rafaga *) datos;

	tamanioStringFinal = sizeof(t_rafaga);
	buffer = malloc(sizeof(char) * (sizeof(t_rafaga)));

	memcpy(buffer + offset, &datosRafaga->numeroCPU, sizeof(datosRafaga->numeroCPU));
	offset += sizeof(datosRafaga->numeroCPU);
	memcpy(buffer + offset, &datosRafaga->pcb, sizeof(datosRafaga->pcb));
	offset += sizeof(datosRafaga->pcb);
	
	*tamanioResultado = tamanioStringFinal;
	*resultado = buffer;
	return 1;
}

uint32_t DeSerializar_Resultado_Rafaga(const char* datosSerializados, uint32_t sizeDatosSerializados, void** resultado) {

	int offset = 0;

	t_rafaga *datosRafaga = NULL;

	datosRafaga = malloc(sizeof(t_rafaga));

	memcpy(&datosRafaga->numeroCPU, datosSerializados + offset, sizeof(datosRafaga->numeroCPU));
	offset += sizeof(datosRafaga->numeroCPU);
	memcpy(&datosRafaga->pcb, datosSerializados + offset, sizeof(datosRafaga->pcb));
	offset += sizeof(datosRafaga->pcb);

	*resultado = datosRafaga;
	return 1;
}

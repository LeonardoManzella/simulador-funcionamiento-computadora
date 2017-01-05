#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "../../bibliotecas/comun.h"
#include "../../bibliotecas/structs.h"
#include "../../bibliotecas/logs.h"
#include "../../lib/commons/bitarray.h"
#include "../../lib/commons/collections/list.h"
#include "../../lib/commons/string.h"

#include "../admin.h"
#include "../headers/admin_helper.h"
#include "../headers/admin_communication.h"
#include "../headers/admin_configuration.h"

void vaciar_y_liberar_lista_de_paginas(t_list** listaDePaginas, t_bitarray** array_posiciones) {
	//OJO No usa el mutex de memoria, depende de quien lo use ponga el mutex antes y despues de esto
	int indiceLista = 0;
	int tamanioLista = list_size(*listaDePaginas);

	for (indiceLista = tamanioLista; indiceLista > 0; indiceLista--) {

		//t_pagina* pagina = list_remove(*listaDePaginas, indiceLista - 1);
		// Siempre remueve la pagina 0, ya que la lista se actualiza en orden
		t_pagina* pagina = list_remove(*listaDePaginas, 0);

		if (pagina->marco > -1 && pagina->prescencia) {
			bitarray_set_bit(*array_posiciones, pagina->marco);

			if (isTLBon()) {
				borrar_de_TLB(pagina);
			}
		}

		free(pagina);
	}
}

char* conseguirPunteroAMarco(char** memoria, int position) {
	return (*memoria + position * (traerTamanioMarcos()));
}

int posicionProcesoEnLista(int id_proceso, t_list* lista_procesos) {
	//No olvidar usar mutex de procesos antes de llamar esta funcion, para que la lista no cambie mientras se busca
	int i = 0;
	int size = list_size(lista_procesos);

	for (i = 0; i < size; i++) {
		t_proceso_memoria* mProc = list_get(lista_procesos, i);

		if (mProc->idProceso == id_proceso) {
			return i;
		}
	}

	Macro_ImprimirParaDebugConDatos("No se encontro el proceso %d en la lista de procesos", id_proceso);

	return -1;
}

int conseguirProcesoEnLista(int id_proceso, t_list** lista_procesos, t_proceso_memoria** puntero_a_proceso) {
	//No olvidar usar mutex de procesos antes de llamar esta funcion, para que la lista no cambie mientras se busca
	int i = 0;
	int size = list_size(*lista_procesos);

	//Macro_ImprimirParaDebugConDatos("buscando %d lista tamanio %d", id_proceso, size);

	for (i = 0; i < size; i++) {
		t_proceso_memoria* mProc = list_get(*lista_procesos, i);

		//Macro_ImprimirParaDebugConDatos("en lista %d", mProc->idProceso);

		if (mProc->idProceso == id_proceso) {
			*puntero_a_proceso = mProc;
			return 1;
		}
	}

	Macro_ImprimirParaDebugConDatos("No se encontro el proceso %d en la lista de procesos", id_proceso);

	return -1;
}

int obtenerPaginaEnProceso(int id_pagina, int id_proceso, t_list* lista_pagina, t_pagina** puntero_a_pagina) {
	int i = 0;
	int size = list_size(lista_pagina);

	for (i = 0; i < size; i++) {
		t_pagina* pagina = list_get(lista_pagina, i);

		if (pagina->idProceso == id_proceso && pagina->idPagina == id_pagina) {
			*puntero_a_pagina = pagina;
			return 1;
		}
	}

	Macro_ImprimirParaDebugConDatos("No se encontro la pagina %d en las paginas de la lista", id_pagina);

	return -1;
}

int posicionPaginaEnLista(int id_pagina, int id_proceso, t_list* lista_pagina) {
	int i = 0;
	int size = list_size(lista_pagina);

	for (i = 0; i < size; i++) {
		t_pagina* pagina = list_get(lista_pagina, i);

		if (pagina->idProceso == id_proceso && pagina->idPagina == id_pagina) {
			return i;
		}
	}

	Macro_ImprimirParaDebugConDatos("No se encontro la pagina %d en las paginas de la lista", id_pagina);

	return -1;
}

int crearProceso(t_proceso_memoria** procesoNuevo) {
	t_list* listaDePaginas = list_create();
	(*procesoNuevo)->paginas = listaDePaginas;

	t_list* listaDeFrames = list_create();
	(*procesoNuevo)->puntero_frames = listaDeFrames;

	int i = 0;
	for (i = 0; i < (*procesoNuevo)->cantidadPaginasTotales; i++) {
		t_pagina* nuevaPagina = malloc(sizeof(t_pagina));

		nuevaPagina->idPagina = i;
		nuevaPagina->idProceso = (*procesoNuevo)->idProceso;
		nuevaPagina->marco = -1; //por ahora, -1 es no asignado
		nuevaPagina->modificada = false;
		nuevaPagina->prescencia = false;
		nuevaPagina->usado = false; // para Clock-M

		list_add(listaDePaginas, nuevaPagina);
	}

	int resultadoSwap = iniciar_proceso_en_swap((*procesoNuevo));

	if (resultadoSwap < 0) {
		vaciar_y_liberar_lista_de_paginas(&listaDePaginas, NULL);
		list_destroy((*procesoNuevo)->paginas);
		list_destroy((*procesoNuevo)->puntero_frames);

		return resultadoSwap;
	}

	(*procesoNuevo)->cantidadFallosDePagina = 0;
	(*procesoNuevo)->accesosASwap = 0;

	return 1;
}

int finalizarProceso(t_proceso_memoria** procesoVictima, t_bitarray** array_posiciones, pthread_mutex_t** mutexMemoria) {
	pthread_mutex_lock(*mutexMemoria);
	vaciar_y_liberar_lista_de_paginas(&((*procesoVictima)->paginas), array_posiciones);
	pthread_mutex_unlock(*mutexMemoria);

	list_destroy((*procesoVictima)->paginas);

	int ipagActual;
	int iFramesEnMemoria = list_size((*procesoVictima)->puntero_frames);
	for (ipagActual = 0; ipagActual < iFramesEnMemoria; ipagActual++) {
		list_remove((*procesoVictima)->puntero_frames, 0);
	}

	list_destroy((*procesoVictima)->puntero_frames);
	return finalizar_proceso_en_swap(*procesoVictima);
}

void delayDeMemoria() {
	if (traerRetardoMemoria() > 0) {
		usleep(traerRetardoMemoria() * 1000000);
	}
}

int getFirstFreePosition(t_bitarray** array_posiciones) {
	int i = 0;

	size_t sizeBitArray = traerTotalMarcos();

	//Macro_ImprimirParaDebugConDatos("size bit array %d", sizeBitArray);

	for (i = 0; i < sizeBitArray; i++) {
		if (bitarray_test_bit(*array_posiciones, i)) {
			return i;
		}
	}

	Macro_ImprimirParaDebugConDatos("---------No se encontro posicion libre");

	return -1;
}

void limpiarMemoria() {
	Macro_ImprimirParaDebugConDatos("Iniciando limpieza de memoria");
	int iprocActual = 0;
	int nFramesEnMemoria;
	Macro_ImprimirParaDebugConDatos("Bloqueo Procesos");
	pthread_mutex_lock(mutex_procesos);
	int cantProc = list_size(lista_de_procesos);
	for (iprocActual = 0; iprocActual < cantProc; iprocActual++) {
		t_proceso_memoria* procActual = list_get(lista_de_procesos, iprocActual);
		int cantPaginas = list_size(procActual->paginas);
		int ipagActual = 0;
		for (ipagActual = 0; ipagActual < cantPaginas; ipagActual++) {
			t_pagina* pagActual = list_get(procActual->paginas, ipagActual);
			// pongo en blanco
			if (pagActual->marco >= 0) {
				if (pagActual->modificada == true) {
					// guardo el contenido de la pagina en swap
					char *contenidoFrame = malloc(sizeof(char) * traerTamanioMarcos());
					int offset = pagActual->marco * traerTamanioMarcos();
					memcpy(contenidoFrame, memoria + offset, traerTamanioMarcos());
					escribir_pagina_de_swap(pagActual, contenidoFrame);
					free(contenidoFrame);
				}
				pagActual->marco = -1;
				pagActual->modificada = false;
				pagActual->prescencia = false;
				pagActual->usado = false;
			}
		}
		nFramesEnMemoria = list_size(procActual->puntero_frames);
		for (ipagActual = 0; ipagActual < nFramesEnMemoria; ipagActual++) {
			list_remove(procActual->puntero_frames, 0);
		}
	}
	// limpio el bitarray de los marcos

	pthread_mutex_lock(mutex_memoria);
	Macro_ImprimirParaDebugConDatos("Bloqueo Memoria para limpiarla");
	memset(memoria, 0, traerTamanioMarcos() * traerTotalMarcos());
	int ibitactual;
	for (ibitactual = 0; ibitactual < traerTotalMarcos(); ibitactual++) {
		bitarray_set_bit(array_posiciones, ibitactual);
	}
	Macro_ImprimirParaDebugConDatos("DesBloqueo Memoria");
	pthread_mutex_unlock(mutex_memoria);

	// Esto es para limipiar la TLB al limpiar la memoria, si no hay que hacerlo se comenta la linea
	limpiarTLB();

	pthread_mutex_unlock(mutex_procesos);
	Macro_ImprimirParaDebugConDatos("Libero Procesos");

}

void limpiarTLB() {
	if (isTLBon()) {
		Macro_ImprimirParaDebugConDatos("Limpiando TLB.\n Bloqueo TLB.");
		pthread_mutex_lock(mutex_TLB);
		int sizeTLB = list_size(listaTLB);
		int pagActual = 0;
		for (pagActual = 0; pagActual < sizeTLB; pagActual++) {
			list_remove(listaTLB, 0);
		}
		pthread_mutex_unlock(mutex_TLB);
		Macro_ImprimirParaDebugConDatos("DesBloqueo TLB.");
	} else {
		Macro_ImprimirParaDebugConDatos("No esta activa la TLB");
	}
}

void DumpMemory(void *Params) {
//	pid_t childProc;

	Macro_ImprimirParaDebugConDatos("Dump de Memoria.\n Realizo un FORK.");
//	childProc = fork();
//	if (childProc == -1) {
	// Ocurrio un error al hacer fork
//	} else if (childProc == 0) {
//		Macro_ImprimirParaDebugConDatos("Estoy en el Proceso Hijo");
	// Estoy en el proceso hijo
	int fd; // File Descriptor
	char* writeContent; // = malloc(sizeof(char) * 7); // Contenido del texto con el numero de frame
	char* contFrame = malloc(sizeof(char) * (traerTamanioMarcos() + 1)); // Contenido del Frame de Memoria
	contFrame[traerTamanioMarcos()] = '\0';
	int iFrameAct, offset;

	Macro_ImprimirParaDebugConDatos("Bloqueo Procesos");
	pthread_mutex_lock(mutex_procesos);

	fd = open("memory.dmp", O_RDWR | O_CREAT | O_TRUNC, 00600);
	if (fd <= 0) {
		Macro_Imprimir_Error("Error: %s\n", strerror(errno));
	}

	Macro_ImprimirParaDebugConDatos("Total de Marcos: %i\n", traerTotalMarcos());
	for (iFrameAct = 0; iFrameAct < traerTotalMarcos(); iFrameAct++) {
		// texto es numero de marco
		//sprintf(textCont, "%3i | ", iFrameAct);
		Macro_ImprimirParaDebugConDatos("Frame a guardar: '%d'\n", iFrameAct);
		// texto contenido del marco
		offset = traerTamanioMarcos() * iFrameAct;
		mempcpy(contFrame, memoria + offset, traerTamanioMarcos());
		Macro_ImprimirParaDebugConDatos("Contenido Frame: '%s'\n", contFrame);
		// Escribo los datos
		writeContent = string_from_format("%3d | %s\n", iFrameAct, contFrame);
		int res = write(fd, writeContent, string_length(writeContent));
		if (res <= 0) {
			Macro_ImprimirParaDebugConDatos("Error al hacer Write: %s", strerror(errno));
		}
		Macro_ImprimirParaDebugConDatos("Se escribio el archivo");
		free(writeContent);
	}
	// Aca termina el mutex
	// Cierro el archivo y libero la memoria
	close(fd);
	pthread_mutex_unlock(mutex_procesos);
	Macro_ImprimirParaDebugConDatos("DesBloqueo Procesos");
	free(contFrame);
	exit(0);
	return;
}

void updateTimeList(t_list** lista_a_actualizar, t_pagina* frame_a_actualizar) {
	char algoritmo[5];
	algoritmo[4] = '\0';

	conseguirAlgoritmoReemplazo(algoritmo);

	if (!strcmp(algoritmo, "LRU")) {
		int position = posicionPaginaEnLista(frame_a_actualizar->idPagina, frame_a_actualizar->idProceso,
				*lista_a_actualizar);

		if (position > -1) {
			list_remove(*lista_a_actualizar, position);
			list_add(*lista_a_actualizar, frame_a_actualizar);
		}
	}
}

int elegir_victima(t_proceso_memoria* proceso, t_list* lista_de_paginas, bool isTLB) {
	char algoritmo[5];
	algoritmo[4] = '\0';

	conseguirAlgoritmoReemplazo(algoritmo);

	if (!strcmp(algoritmo, "FIFO")) {
		return victima_fifo(proceso, lista_de_paginas, isTLB);
	}

	if (!strcmp(algoritmo, "LRU")) {
		return victima_fifo(proceso, lista_de_paginas, isTLB);
	}

	if (!strcmp(algoritmo, "CLKM")) {
		return victima_clockm(proceso, lista_de_paginas, isTLB);
	}

	return -1;
}

int victima_fifo(t_proceso_memoria* proceso, t_list* lista_de_paginas, bool isTLB) {
	Macro_ImprimirParaDebugConDatos("FIFO algorithm");

	int currentVictim = -1;
	int nextVictim = -1;
	t_pagina* pageNextVictim = NULL;

	t_pagina* pageVictim = (t_pagina *) list_get(proceso->puntero_frames, 0);
	currentVictim = pageVictim->idPagina;

	if (list_size(proceso->puntero_frames) > 1) {
		pageNextVictim = (t_pagina *) list_get(proceso->puntero_frames, 1);
		nextVictim = pageNextVictim->idPagina;
	}

	list_add(proceso->puntero_frames, pageVictim);
	list_remove(proceso->puntero_frames, 0);

	if (currentVictim > -1) {
		LOG_escribirPantalla(ARCHIVO_LOG, __func__, LOG_LEVEL_INFO, "Victima a reemplazar. Pagina: %d del Proceso %d",
				pageVictim->idPagina, pageVictim->idProceso);
	}

	if (nextVictim > -1) {
		LOG_escribirPantalla(ARCHIVO_LOG, __func__, LOG_LEVEL_INFO,
				"Siguiente victima a reemplazar. Pagina: %d del Proceso %d", pageNextVictim->idPagina,
				pageNextVictim->idProceso);
	}

	return currentVictim;
}

int victima_clockm(t_proceso_memoria* proceso, t_list* lista_de_paginas, bool isTLB) {
	Macro_ImprimirParaDebugConDatos("Clock-M algorithm");

	int currentVictim = -1;
	int nextVictim = -1;
	int sizeFramesProceso = list_size(proceso->puntero_frames);

	bool bHabemusVictima = false;
	t_pagina *frameActual;
	bool Ballotage = false;

	t_pagina* frameVictim = (t_pagina*) list_get(proceso->puntero_frames, 0);
	frameActual = frameVictim;

	Macro_ImprimirParaDebugConDatos("Frame Actual [%d] Frame Victima [%d]", frameActual->idPagina,
			frameVictim->idPagina);

	int numeroVuelta = 0;
	if (sizeFramesProceso > 1) {
		Macro_ImprimirParaDebugConDatos("Buscando Victima...");
		while (nextVictim == -1) {
			while (bHabemusVictima == false) {
				if (Ballotage == false) {
					Macro_ImprimirParaDebugConDatos("Buscando (0,0) - Frame Actual [%d] usado [%d] modificado [%d]",
							frameActual->idPagina, frameActual->usado, frameActual->modificada);
					if ((frameActual->usado == false) && (frameActual->modificada == false) && (currentVictim == -1)) {
						Macro_ImprimirParaDebugConDatos("Victima Frame [%d]", frameActual->idPagina);
						bHabemusVictima = true;
						break;
					} else if ((frameActual->usado == false) && (frameActual->modificada == false)
							&& (currentVictim != -1) && (currentVictim != frameActual->idPagina)) {
						Macro_ImprimirParaDebugConDatos("Victima Siguiente Frame [%d]", frameActual->idPagina);
						bHabemusVictima = true;
						break;
					} else {
						list_add(proceso->puntero_frames, frameActual);
						list_remove(proceso->puntero_frames, 0);
						frameActual = (t_pagina*) list_get(proceso->puntero_frames, 0);
						if (frameActual == frameVictim) {
							Macro_ImprimirParaDebugConDatos("No se encontro ningun (0,0)");
							Ballotage = true;
						}
					}
				} else {
					Macro_ImprimirParaDebugConDatos("Buscando (0,1) Frame Actual [%d] usado [%d] modificado [%d]",
							frameActual->idPagina, frameActual->usado, frameActual->modificada);
					if ((frameActual->usado == false) && (frameActual->modificada == true) && (currentVictim == -1)) {
						Macro_ImprimirParaDebugConDatos("Victima Frame [%d]", frameActual->idPagina);
						bHabemusVictima = true;
						break;
					} else if ((frameActual->usado == false) && (frameActual->modificada == true)
							&& (currentVictim != -1) && (currentVictim != frameActual->idPagina)) {
						Macro_ImprimirParaDebugConDatos("Victima Siguiente Frame [%d]", frameActual->idPagina);
						bHabemusVictima = true;
						break;
					} else {
						frameActual->usado = false;
						list_add(proceso->puntero_frames, frameActual);
						list_remove(proceso->puntero_frames, 0);
						frameActual = (t_pagina*) list_get(proceso->puntero_frames, 0);
						if (frameActual == frameVictim) {
							Macro_ImprimirParaDebugConDatos(
									"No se encontro ningun (0,1)\nAhora estan todos los Usados = 0");
							Ballotage = false;
							if (numeroVuelta == 1) {
								break;
							} else {
								numeroVuelta++;
							}
						}
					}
				}
			}
			if ((currentVictim == -1) && (bHabemusVictima == true)) {
				Macro_ImprimirParaDebugConDatos("Se encontro una Victima, ahora a buscar la siguiente victima...");
				currentVictim = frameActual->idPagina;
				bHabemusVictima = false;
				numeroVuelta = 0;
				Ballotage = false;
			} else {
				break;
			}
		}
	} else {
		bHabemusVictima = true;
		currentVictim = frameActual->idPagina;
	}

	if (bHabemusVictima == true) {
		nextVictim = frameActual->idPagina;
		return currentVictim;
	} else {
		return -1;
	}
}

int Reemplazo_de_Marcos(t_proceso_memoria* procesoLocal, int *posicionPagina) {
	int victima = elegir_victima(procesoLocal, procesoLocal->paginas, false);

	if (!(victima > -1)) {
		error(__func__, "error eligiendo victima");
		return -1;
	}

	*posicionPagina = posicionPaginaEnLista(victima, procesoLocal->idProceso, procesoLocal->puntero_frames);

	t_pagina* frameVictima = list_get(procesoLocal->paginas, victima);

	if (frameVictima->modificada) {
		char* contenidoFrame = malloc(sizeof(char) * (traerTamanioMarcos() + 1));

		if (contenidoFrame != NULL) {
			contenidoFrame[traerTamanioMarcos()] = '\0';

			char* frameEnMemoria = conseguirPunteroAMarco(((char**) &memoria), frameVictima->marco);

			memcpy(contenidoFrame, frameEnMemoria, traerTamanioMarcos());

			if (escribir_pagina_de_swap(frameVictima, contenidoFrame) > 0) {
				info(__func__, "pagina guardada en swap correctamente");
			}

			frameVictima->modificada = false;

			free(contenidoFrame);

			procesoLocal->accesosASwap++;
		} else {
			error(__func__, "error reservando memoria para escribir frame en swap");
		}
	}

	if (isTLBon()) {
		borrar_de_TLB(frameVictima);
	}

	frameVictima->prescencia = false;
	frameVictima->usado = false;

	return frameVictima->marco;
}

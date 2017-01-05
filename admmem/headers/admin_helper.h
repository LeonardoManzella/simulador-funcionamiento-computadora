#ifndef HEADERS_ADMIN_HELPER_H_
#define HEADERS_ADMIN_HELPER_H_

#include "../../lib/commons/bitarray.h"
#include "../../lib/commons/collections/list.h"
#include "../../bibliotecas/structs.h"

void vaciar_y_liberar_lista_de_paginas(t_list** listaDePaginas, t_bitarray** array_posiciones);

long long updateTimeReference(long previous);

char* conseguirPunteroAMarco(char** memoria, int position);

int posicionProcesoEnLista(int id_proceso, t_list* lista_procesos);

int conseguirProcesoEnLista(int id_proceso, t_list** lista_procesos, t_proceso_memoria** puntero_a_proceso);

int obtenerPaginaEnProceso(int id_pagina, int id_proceso, t_list* lista_pagina, t_pagina** puntero_a_pagina);

int posicionPaginaEnLista(int id_pagina, int id_proceso, t_list* lista_pagina);

int crearProceso(t_proceso_memoria** procesoNuevo);

int finalizarProceso(t_proceso_memoria** procesoVictima, t_bitarray** array_posiciones, pthread_mutex_t** mutexMemoria);

void delayDeMemoria();

void limpiarMemoria();

void limpiarTLB();

void DumpMemory(void *Params);

int getFirstFreePosition(t_bitarray** array_posiciones);

void updateTimeList(t_list** lista_a_actualizar, t_pagina* frame_a_actualizar);

int elegir_victima(t_proceso_memoria* proceso, t_list* lista_de_paginas, bool isTLB);

int victima_fifo(t_proceso_memoria* proceso, t_list* lista_de_paginas, bool isTLB);

int victima_clockm(t_proceso_memoria* proceso, t_list* lista_de_paginas, bool isTLB);

int Reemplazo_de_Marcos(t_proceso_memoria* procesoLocal, int* posicionPagina);
#endif /* HEADERS_ADMIN_HELPER_H_ */

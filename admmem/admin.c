#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "headers/admin_helper.h"
#include "admin.h"
#include "headers/admin_configuration.h"
#include "headers/admin_communication.h"

#include "../bibliotecas/comun.h"
#include "../bibliotecas/inicializadores.h"
#include "../bibliotecas/serializacion.h"
#include "../bibliotecas/sockets.h"
#include "../bibliotecas/structs.h"
#include "../bibliotecas/logs.h"
#include "../lib/commons/bitarray.h"
#include "../lib/commons/collections/list.h"

//DEJAR ACA
int tlb_hits = 0;
int tlb_miss = 0;

int main() {
	cleanLogFile();

	if (!validarConfiguracion()) {
		error(__func__, "El archivo de configuracion no es valido");
		return EXIT_FAILURE;
	}

	char* freeMemoryArray = NULL;
	int tamanioBitArray = traerTotalMarcos() / 8 + (((traerTotalMarcos() % 8) == 0) ? 0 : 1);

	Macro_ImprimirParaDebugConDatos("tamanio bit array %d", tamanioBitArray);

	if (traerTotalMarcos() != 0) {
		Macro_ImprimirParaDebugConDatos("\n\r\n\r %d marcos\n\r\n\r", traerTotalMarcos());
		memoria = malloc(traerTamanioMarcos() * traerTotalMarcos());
		memset(memoria, 0, (traerTamanioMarcos() * traerTotalMarcos()));
		freeMemoryArray = malloc(sizeof(char) * tamanioBitArray);

		if (memoria == NULL || freeMemoryArray == NULL) {
			error(__func__, "No se reservo la memoria, no es posible iniciar el administrador");
			return EXIT_FAILURE;
		}
	} else {
		Macro_ImprimirParaDebugConDatos("Sin marcos, se maneja directo con swap");
	}

	if (conectar_swap() <= 0) {
		error(__func__,
				"No es posible conectar con el SWAP, asegurese de que la IP y el PUERTO son correctos, y de que el SWAP este funcionando");

		if (memoria != NULL) {
			free(memoria);
		}

		if (freeMemoryArray != NULL) {
			free(freeMemoryArray);
		}

		return EXIT_FAILURE;
	}

	//chequeo que la tlb este habilitada o no, y la creo en caso de que si
	if (isTLBon()) {
		info(__func__, "TLB activa");
		listaTLB = list_create();
	} else {
		info(__func__, "TLB desactivada");
	}

	//Inicio lista de paginas actuales
	lista_de_procesos = list_create();

	if (freeMemoryArray != NULL) {
		array_posiciones = bitarray_create(freeMemoryArray, tamanioBitArray);
		int i = 0;
		for (i = 0; i < traerTotalMarcos(); i++) {
			bitarray_set_bit(array_posiciones, i);
		}
	}

	initMutexes();

	// Este proceso inicia el manejo de señales
	initSignals();

	//Inicio proceso de logueo de tlb hits y miss
	pthread_t idThread;
	pthread_create(&idThread, NULL, (void *) &imprimir_tlb, NULL);

	EsperarCPU();

	return EXIT_SUCCESS;
}

int EsperarCPU() {
	int iReturn = 0;
	char puertoAdmMem[6];
	conseguirPuertoAdmin(puertoAdmMem);

	SOCKET_crear(socket_admmem);

	t_order_header orden;
	socket_admmem->Escuchar(socket_admmem, puertoAdmMem);
	while (1) {
		SOCKET_crear(socket_cliente);
		socket_cliente->Aceptar_Cliente(socket_admmem, socket_cliente);
		while (socket_cliente->descriptorSocket > 0) {
			if (socket_cliente->Recibir(socket_cliente, &orden, NULL, NULL) > 0) {
				Macro_ImprimirParaDebugConDatos("Recibí orden: %d, de: %d\n\r\n\r", orden.comando, orden.sender_id);
				if (orden.sender_id == CPU) {
					switch (orden.comando) {
						case CONECTAR_CPU_ADMIN:
							orden_conectar_CPU(socket_cliente);
						break;
						case INICIAR_PROCESO:
							orden_iniciar_proceso(socket_cliente);
						break;
						case LEER_FRAME:
							orden_CPU_leer_datos(socket_cliente);
						break;
						case ESCRIBIR_FRAME:
							orden_CPU_Escribir_Datos(socket_cliente);
						break;
						case FINALIZAR_PROCESO:
							orden_finalizar_proceso(socket_cliente);
						break;
						default:
							error(__func__,
									"La orden recibida no puede ser atendida por el administrador de memoria, ignorando");

						break;
					}
				} else {
					error(__func__, "Se recibio algo que no viene de CPU, ignorando");
				}
			} else {
				error(__func__, "No se recibio correctamente el mensaje");
			}
			socket_cliente->Desconectar(socket_cliente);
		}
		sleep(1);

		if (socket_cliente->descriptorSocket == 0) {
			SOCKET_destruir(socket_cliente);
		}
	}
	return iReturn;
}

void initMutexes() {
	int numeroError = initRegularMutex(&mutex_memoria);
	if (numeroError < 0) {
		Macro_Imprimir_Error("El Mutex de Memoria no se inicializo bien");
	}

	numeroError = initRegularMutex(&mutex_TLB);
	if (numeroError < 0) {
		Macro_Imprimir_Error("El Mutex de la TLB no se inicializo bien");
	}

	numeroError = initSharedMutex(&mutex_procesos);
	if (numeroError < 0) {
		Macro_Imprimir_Error("El Mutex de la lista de paginas no se inicializo bien");
	}

//  Removido por ahora, al atender solo 1 transaccion a la vez, solo podria hacer una request a la vez al swap
//	numeroError = initRegularMutex(&mutex_swap);
//	if (numeroError < 0) {
//		Macro_Imprimir_Error("El Mutex de SWAP no se inicializo bien");
//	}

}

void initSignals() {
	/*	struct sigaction sa;
	 sa.sa_handler = &handle_signals;
	 sa.sa_flags = SA_RESTART;
	 sigfillset(&sa.sa_mask);
	 if (sigaction(SIGUSR1, &sa, NULL) < 0) {
	 Macro_Imprimir_Error("No se pudo manejar SIGUSR1");
	 }
	 if (sigaction(SIGUSR2, &sa, NULL) < 0) {
	 Macro_Imprimir_Error("No se pudo manejar SIGUSR1");
	 }
	 if (sigaction(SIGPOLL, &sa, NULL) < 0) {
	 Macro_Imprimir_Error("No se pudo manejar SIGUSR1");
	 }
	 */
	if (iniciarHandler(SIGUSR1, handle_signals) < 0) {
		Macro_Imprimir_Error("No se pudo manejar SIGUSR1");
	}
	if (iniciarHandler(SIGUSR2, handle_signals) < 0) {
		Macro_Imprimir_Error("No se pudo manejar SIGUSR1");
	}
	if (iniciarHandler(SIGPOLL, handle_signals) < 0) {
		Macro_Imprimir_Error("No se pudo manejar SIGUSR1");
	}
}

void handle_signals(int signal) {
	switch (signal) {
		case SIGUSR1:
			info(__func__, "Recibida SIGUSR1");
			iniciarThread((void*) &limpiarMemoria, NULL);	// Ejecutar proc. limpiar memoria
		break;
		case SIGUSR2:
			info(__func__, "Recibida SIGUSR2");
			iniciarThread((void*) &limpiarTLB, NULL);
			// Tenqo que ejecutar proce. limpiar TLB
		break;
		case SIGPOLL:
			info(__func__, "Recibida SIGPOLL");
			iniciarHijo(DumpMemory, NULL);
			// Tengo que hacer un clone y volvar los datos
		break;
		default:
			Macro_Imprimir_Error("Se recibio una senal no esperada")
			;
	}
	return;
}

void orden_conectar_CPU(t_socket* socket_cliente) {
	LOG_escribirPantalla(ARCHIVO_LOG, __func__, LOG_LEVEL_DEBUG, "Recibida orden Conectar CPU");

	t_order_header resultado_orden;
	resultado_orden.sender_id = ADMMEM;
	resultado_orden.comando = OK;

	socket_cliente->Enviar(socket_cliente, NULL, resultado_orden, NULL);
}

void orden_iniciar_proceso(t_socket* socket_cliente) {
	t_order_header orden, resultado_orden;
	t_proceso_memoria* nuevoProceso = NULL;
	resultado_orden.sender_id = ADMMEM;

	if (socket_cliente->Recibir(socket_cliente, &orden, (void**) &nuevoProceso, DeSerializar_MemSWAP_IniciarProceso)
			> 0) {

		LOG_escribirPantalla(ARCHIVO_LOG, __func__, LOG_LEVEL_INFO, "Recibida orden Iniciar proceso %d con %d paginas",
				nuevoProceso->idProceso, nuevoProceso->cantidadPaginasTotales);

		int iResultado = crearProceso(&nuevoProceso);
		if (iResultado > 0) {

			pthread_mutex_lock(mutex_procesos);
			int result = list_add(lista_de_procesos, nuevoProceso);
			if (result < 0) {
				LOG_escribirPantalla(ARCHIVO_LOG, __func__, LOG_LEVEL_ERROR, "Error al agregar a la lista");
				resultado_orden.comando = ERROR;
			} else {
				Macro_ImprimirParaDebugConDatos("agregado %d en la posicion %d", nuevoProceso->idProceso, result);
				resultado_orden.comando = OK;
			}

			pthread_mutex_unlock(mutex_procesos);

			LOG_escribirPantalla(ARCHIVO_LOG, __func__, LOG_LEVEL_DEBUG, "Iniciar proceso %d funciono correctamente",
					nuevoProceso->idProceso);
		} else {
			if (iResultado == -2) {
				resultado_orden.comando = FRAMES_OCUPADOS;
				LOG_escribirPantalla(ARCHIVO_LOG, __func__, LOG_LEVEL_ERROR,
						"Error al crear en SWAP: No hay espacio suficiente");
			} else {
				resultado_orden.comando = ERROR;
				LOG_escribirPantalla(ARCHIVO_LOG, __func__, LOG_LEVEL_ERROR, "Error al crear en SWAP, ver log SWAP");
			}
		}

	} else {
		resultado_orden.comando = ERROR;

		LOG_escribirPantalla(ARCHIVO_LOG, __func__, LOG_LEVEL_ERROR,
				"Recibida orden Iniciar, pero los datos nunca llegaron");
	}

	socket_cliente->Enviar(socket_cliente, NULL, resultado_orden, NULL);
}

void orden_finalizar_proceso(t_socket* socket_cliente) {
	t_order_header orden;
	t_proceso_memoria* victima = NULL;

	if (socket_cliente->Recibir(socket_cliente, &orden, (void**) &victima, DeSerializar_MemSWAP_FinalizarProceso) > 0) {

		LOG_escribirPantalla(ARCHIVO_LOG, __func__, LOG_LEVEL_INFO, "Recibida orden Finalizar proceso %d",
				victima->idProceso);

		orden.sender_id = ADMMEM;

		pthread_mutex_lock(mutex_procesos);
		int position = posicionProcesoEnLista(victima->idProceso, lista_de_procesos);
		if (position >= 0) {

			t_proceso_memoria* victimaEnMemoria = list_get(lista_de_procesos, position);

			if (finalizarProceso(&victimaEnMemoria, &array_posiciones, &mutex_memoria) != 1) {

				LOG_escribirPantalla(ARCHIVO_LOG, __func__, LOG_LEVEL_ERROR,
						"No se elimino proceso %d del SWAP correctamente", victima->idProceso);

				orden.comando = ERROR;

			} else {

				orden.comando = OK;

			}

			LOG_escribirPantalla(ARCHIVO_LOG, __func__, LOG_LEVEL_DEBUG,
					"Proceso %d eliminado de Memoria correctamente. Accesos a Swap: %d, Fallos de pagina: %d",
					victimaEnMemoria->idProceso, victimaEnMemoria->accesosASwap,
					victimaEnMemoria->cantidadFallosDePagina);

			list_remove_and_destroy_element(lista_de_procesos, position, free);

		} else {

			LOG_escribirPantalla(ARCHIVO_LOG, __func__, LOG_LEVEL_ERROR,
					"No se encontro proceso %d en la lista de procesos, verifique la informacion", victima->idProceso);

			orden.comando = ERROR;

		}
		pthread_mutex_unlock(mutex_procesos);

		socket_cliente->Enviar(socket_cliente, NULL, orden, NULL);
	} else {
		LOG_escribirPantalla(ARCHIVO_LOG, __func__, LOG_LEVEL_ERROR,
				"Recibida orden Finalizar, pero los datos nunca llegaron");
	}

	if (victima != NULL) {
		free(victima);
	}
}

void orden_CPU_Escribir_Datos(t_socket* socket_cliente) {
	int errorCode = 0;
	t_order_header orden_recibida, orden;
	t_pagina *frameRecibido = NULL;
	t_pagina* frameLocal = NULL;
	char* contenidoFrame = NULL;

	// Recibo los datos del Frame y el Contenido del mismo que tengo que guardar	
	errorCode = socket_cliente->Recibir(socket_cliente, &orden_recibida, (void**) &frameRecibido,
			DeSerializar_MemSWAP_Frame);
	Macro_Check_And_Handle_Error(errorCode <= 0, "No se recibio correctamente los datos de orden de escribir");

	LOG_escribirPantalla(ARCHIVO_LOG, __func__, LOG_LEVEL_INFO, "Recibida orden Escribir pagina %d proceso %d",
			frameRecibido->idPagina, frameRecibido->idProceso);

	errorCode = socket_cliente->Recibir(socket_cliente, &orden_recibida, (void**) &contenidoFrame,
			DeSerializar_MemSWAP_ContenidoFrame);
	Macro_Check_And_Handle_Error(errorCode <= 0, "No se recibio correctamente el contenido de orden de escribir");

	int resultado = -1;

	if (strlen(contenidoFrame) > traerTamanioMarcos()) {
		LOG_escribirPantalla(ARCHIVO_LOG, __func__, LOG_LEVEL_INFO, "Escribir %s tamano %d en %d", contenidoFrame,
				strlen(contenidoFrame), traerTamanioMarcos());
		resultado = -3;
	} else if (traerTotalMarcos() > 0) {
		pthread_mutex_lock(mutex_procesos);
		resultado = traer_o_apuntar_frame_local(frameRecibido, &frameLocal);

		if (resultado > 0) {
			if (contenidoFrame != NULL) {
				//aseguro que el contenido es un string
				char* contenidoMemoria = conseguirPunteroAMarco((char**) &memoria, frameLocal->marco);

				memcpy(contenidoMemoria, contenidoFrame, traerTamanioMarcos());

				frameLocal->modificada = true;
				frameLocal->usado = true;
			}
		}

		pthread_mutex_unlock(mutex_procesos);
	} else {
		// como memoria no esta activa, pasa mensajes a SWAP directo
		resultado = escribir_pagina_de_swap(frameRecibido, contenidoFrame);
	}

	orden.sender_id = ADMMEM;

	if (resultado > 0) {
		orden.comando = OK;

		LOG_escribirPantalla(ARCHIVO_LOG, __func__, LOG_LEVEL_DEBUG, "Completada orden Escribir\n");
	} else {
		if (resultado == -3) {
			LOG_escribirPantalla(ARCHIVO_LOG, __func__, LOG_LEVEL_ERROR,
					"No se pudo completar orden Escribir: Se escribe mas del tamano asignado");

			orden.comando = SEG_FAULT;
		} else if (resultado == -2) {
			LOG_escribirPantalla(ARCHIVO_LOG, __func__, LOG_LEVEL_ERROR,
					"No se pudo completar orden Escribir: No hay espacio en memoria");

			orden.comando = FRAMES_OCUPADOS;
		} else {
			LOG_escribirPantalla(ARCHIVO_LOG, __func__, LOG_LEVEL_ERROR,
					"No se pudo completar orden Escribir: Error durante proceso");

			orden.comando = ERROR;
		}
	}
	errorCode = socket_cliente->Enviar(socket_cliente, NULL, orden, NULL);

	if (frameRecibido != NULL) {
		free(frameRecibido);
	}

	if (contenidoFrame != NULL) {
		free(contenidoFrame);
	}

	return;

	Error_Handler: LOG_escribirPantalla(ARCHIVO_LOG, __func__, LOG_LEVEL_ERROR,
			"No se pudo completar orden Escribir\n");

	orden.comando = ERROR;
	socket_cliente->Enviar(socket_cliente, NULL, orden, NULL);

	if (frameRecibido != NULL) {
		free(frameRecibido);
	}

	if (contenidoFrame != NULL) {
		free(contenidoFrame);
	}
}

void orden_CPU_leer_datos(t_socket* socket_cliente) {
	// Aca esta la comunicacion de un lado, escribir frame va a ser igual pero para el otro lado la serializacion
	uint32_t errorCode = 0;
	t_order_header orden, resultado_orden;
	t_pagina* framePedido = NULL;
	t_pagina* frameLocal = NULL;
	char* contenidoFrame = NULL;
	resultado_orden.sender_id = ADMMEM;

	errorCode = socket_cliente->Recibir(socket_cliente, &orden, (void**) &framePedido, DeSerializar_MemSWAP_Frame);
	Macro_Check_And_Handle_Error(errorCode <= 0, "No se recibio correctamente los datos de orden de leer");

	int resultado = -1;

	LOG_escribirPantalla(ARCHIVO_LOG, __func__, LOG_LEVEL_INFO, "Recibida orden Leer pagina %d proceso %d",
			framePedido->idPagina, framePedido->idProceso);

	if (traerTotalMarcos() > 0) {
		pthread_mutex_lock(mutex_procesos);
		resultado = traer_o_apuntar_frame_local(framePedido, &frameLocal);

		if (resultado > 0) {
			contenidoFrame = malloc(sizeof(char) * (traerTamanioMarcos() + 1));

			if (contenidoFrame != NULL) {
				//aseguro que el contenido es un string
				contenidoFrame[traerTamanioMarcos()] = '\0';

				char* contenidoMemoria = conseguirPunteroAMarco((char**) &memoria, frameLocal->marco);

				memcpy(contenidoFrame, contenidoMemoria, traerTamanioMarcos());
			}
			// Referencia de Usado para Clock-M
			frameLocal->usado = true;
		}

		pthread_mutex_unlock(mutex_procesos);
	} else {
		// Si no hay memoria, paso directo a SWAP
		resultado = leer_pagina_de_swap(framePedido, &contenidoFrame);
	}

	// Cuando se leyo el contenido del Frame correctamente, puede pasar que el Frame no este y tengo que devolver ERROR
	if (resultado > 0) {
		resultado_orden.comando = OK;
		errorCode = socket_cliente->Enviar(socket_cliente, NULL, resultado_orden, NULL);
		Macro_Check_And_Handle_Error(errorCode <= 0, "No se pudo responder correctamente la orden de leer");

		orden.sender_id = ADMMEM;
		orden.comando = CONTENIDO_FRAME;
		errorCode = socket_cliente->Enviar(socket_cliente, contenidoFrame, orden, Serializar_MemSWAP_ContenidoFrame);
		Macro_Check_And_Handle_Error(errorCode <= 0,
				"No se pudo responder correctamente el contenido de la orden de leer");

		LOG_escribirPantalla(ARCHIVO_LOG, __func__, LOG_LEVEL_DEBUG, "Completada orden Leer\n");
	} else {
		if (resultado == -2) {
			LOG_escribirPantalla(ARCHIVO_LOG, __func__, LOG_LEVEL_ERROR,
					"No se pudo completar orden Leer: No hay espacio en memoria");

			orden.comando = FRAMES_OCUPADOS;
		} else {
			LOG_escribirPantalla(ARCHIVO_LOG, __func__, LOG_LEVEL_ERROR,
					"No se pudo completar orden Leer: Error durante proceso");

			orden.comando = ERROR;
		}

		errorCode = socket_cliente->Enviar(socket_cliente, NULL, resultado_orden, NULL);
		Macro_Check_And_Handle_Error(errorCode <= 0, "No se pudo responder correctamente el error de la orden de leer");
	}

	if (framePedido != NULL) {
		free(framePedido);
	}

	if (contenidoFrame != NULL) {
		free(contenidoFrame);
	}

	return;

	Error_Handler: LOG_escribirPantalla(ARCHIVO_LOG, __func__, LOG_LEVEL_ERROR, "No se pudo completar orden Leer\n");
	resultado_orden.comando = ERROR;
	socket_cliente->Enviar(socket_cliente, NULL, resultado_orden, NULL);

	if (framePedido != NULL) {
		free(framePedido);
	}

	if (contenidoFrame != NULL) {
		free(contenidoFrame);
	}
}

int reservarMarcoLibre() {
	int returnValue = -1;

	pthread_mutex_lock(mutex_memoria);
	returnValue = getFirstFreePosition(&array_posiciones);
	//Si hay una posicion libre, ya la reservo
	if (returnValue > -1) {
		bitarray_clean_bit(array_posiciones, returnValue); //se usa clean para que al momento de testear devuelva false
	}
	pthread_mutex_unlock(mutex_memoria);

	return returnValue;
}

//devuelve 1 si encontro (y trajo de ser necesario) el frame, -1 si no lo encontro y -2 si no hay espacio local para traerlo del swap(ni siquiera para reemplazo)
int traer_o_apuntar_frame_local(t_pagina* frameATraer, t_pagina** frameLocalADevolver) {
	Macro_ImprimirParaDebugConDatos("Pagina a Usar %d", frameATraer->idPagina);
	if (isTLBon()) {
		pthread_mutex_lock(mutex_TLB);

		if (obtenerPaginaEnProceso(frameATraer->idPagina, frameATraer->idProceso, listaTLB, frameLocalADevolver) > 0) {
			Macro_ImprimirParaDebugConDatos("TLB Hit");

			t_proceso_memoria* procesoLocal = NULL;
			if (conseguirProcesoEnLista(frameATraer->idProceso, &lista_de_procesos, &procesoLocal) < 0) {
				error(__func__, "No se encontro el proceso en la lista de procesos iniciados");
			} else {
				updateTimeList(&(procesoLocal->puntero_frames), *frameLocalADevolver);
			}

			//delay de acceso a la info
			delayDeMemoria();

			tlb_hits++;

			pthread_mutex_unlock(mutex_TLB);

			return 1;
		} else {
			Macro_ImprimirParaDebugConDatos("TLB Miss");
			tlb_miss++;
		}

		pthread_mutex_unlock(mutex_TLB);
	}

	delayDeMemoria();

	t_proceso_memoria* procesoLocal = NULL;
	if (conseguirProcesoEnLista(frameATraer->idProceso, &lista_de_procesos, &procesoLocal) < 0) {
		error(__func__, "No se encontro el proceso en la lista de procesos iniciados");
		return -1;
	}

	if (obtenerPaginaEnProceso(frameATraer->idPagina, frameATraer->idProceso, procesoLocal->paginas,
			frameLocalADevolver) < 0) {
		error(__func__, "No se encontro la pagina en la lista de paginas del proceso");
		return -1;
	}

	if ((*frameLocalADevolver)->prescencia) {
		//La pagina esta en memoria, hago delay por acceso a memoria y lo guardo en la TLB en caso de que esta exista
		delayDeMemoria();

		updateTimeList(&(procesoLocal->puntero_frames), *frameLocalADevolver);

		// No es necesario ya que Leer y Escribir setean este flag
		//(*frameLocalADevolver)->usado = true;

		if (isTLBon()) {
			agregar_a_TLB(*frameLocalADevolver);
		}

		return 1;
	}

	int marcoAUsar = -1;
	int posicionPaginaAReemplazar;
	// Los primeros frames se agregan en forma secuencial de forma automatica, mientras hay memoria disponible
	int iFramesDelProcesoEnMemoria = list_size(procesoLocal->puntero_frames);
	if (iFramesDelProcesoEnMemoria < traerMaximoMarcosPorProceso()) {
		marcoAUsar = reservarMarcoLibre();
		if ((marcoAUsar == -1) && (iFramesDelProcesoEnMemoria == 0)) {
			return -2;
		} else if (marcoAUsar == -1) {
			marcoAUsar = Reemplazo_de_Marcos(procesoLocal, &posicionPaginaAReemplazar);
			list_replace(procesoLocal->puntero_frames, posicionPaginaAReemplazar, (*frameLocalADevolver));
		} else {
			list_add(procesoLocal->puntero_frames, (*frameLocalADevolver));
		}
	} else {
		// Ya agregue tantos marcos al proceso como podia segun Maximo Marcos por Proceso
		marcoAUsar = Reemplazo_de_Marcos(procesoLocal, &posicionPaginaAReemplazar);
		list_replace(procesoLocal->puntero_frames, posicionPaginaAReemplazar, (*frameLocalADevolver));
	}

	if (marcoAUsar == -1) {
		//error de los algoritmos
		return -1;
	}

	char* marco_puntero = conseguirPunteroAMarco((char**) &memoria, marcoAUsar);

	Macro_ImprimirParaDebugConDatos("Marco a usar %d", marcoAUsar);

	//Inicializo el contenido del frame en 0, para evitar futuros problemas
	memset(marco_puntero, 0, traerTamanioMarcos());

	//SOLO TRAIGO EL CONTENIDO PARA LEER, SI VOY A ESCRIBIR, EL CONTENIDO ES SOBRE ESCRITO, NO ES NECESARIO
	//SI LLEGUE HASTA ACA, LOS DATOS YA LOS TENGO, ENTONCES NO HABRIA ERROR QUE ABORTE LA ESCRITURA
	char* contenidoFrame = NULL;

	if (leer_pagina_de_swap(frameATraer, &contenidoFrame) > 0) {
		int sizeContenido = strlen(contenidoFrame);
		memcpy(marco_puntero, contenidoFrame, sizeContenido);
	}

	procesoLocal->accesosASwap++;
	procesoLocal->cantidadFallosDePagina++;

	delayDeMemoria();

	(*frameLocalADevolver)->marco = marcoAUsar;
	(*frameLocalADevolver)->prescencia = true;
	// Esto ya lo setea Leer y Escribir
	//(*frameLocalADevolver)->usado = true;

	if (isTLBon()) {
		agregar_a_TLB(*frameLocalADevolver);
	}

	return 1;
}

void agregar_a_TLB(t_pagina* pagina_a_agregar) {
	pthread_mutex_lock(mutex_TLB);

	int tlbSize = list_size(listaTLB);

	if (tlbSize == traerTamanioTLB()) {
		t_pagina* pagina = list_get(listaTLB, 0);

		LOG_escribirPantalla(ARCHIVO_LOG, __func__, LOG_LEVEL_INFO,
				"Sacando pagina %d, del proceso %d, de la TLB por FIFO", pagina->idPagina, pagina->idProceso);

		list_remove(listaTLB, 0);
	}

	list_add(listaTLB, pagina_a_agregar);

	pthread_mutex_unlock(mutex_TLB);
}

void borrar_de_TLB(t_pagina* pagina_a_borrar) {
	pthread_mutex_lock(mutex_TLB);

	int position = posicionPaginaEnLista(pagina_a_borrar->idPagina, pagina_a_borrar->idProceso, listaTLB);

	if (position > -1) {
		list_remove(listaTLB, position);
		LOG_escribirPantalla(ARCHIVO_LOG, __func__, LOG_LEVEL_WARNING,
				"Borro de la TLB la pagina %d del proceso %d en la posicion %d por orden (ya no esta mas en memoria)",
				pagina_a_borrar->idPagina, pagina_a_borrar->idProceso, position);
	}

	pthread_mutex_unlock(mutex_TLB);
}

void imprimir_tlb(void* argument) {
	while (true) {
		printf("TLB Hits: %d   -   TLB Misses: %d   -   Tasa de Aciertos: %d\n", tlb_hits, tlb_miss,
				(int) ((tlb_hits * 100) / ((tlb_hits + tlb_miss) > 0 ? (tlb_hits + tlb_miss) : 1)));
		fflush(stdout);

		sleep(60);
	}
}

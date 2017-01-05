#include "swap.h"
#include "swap_log.h"
#include "../bibliotecas/sockets.h"
#include "../bibliotecas/serializacion.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include "../lib/commons/string.h"
#include "../lib/commons/bitarray.h"
#include <stdbool.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <signal.h>

// Configuracion global del SWAP
t_swap_config swap_config;

// Lista de procesos en SWAP
t_list* processList;

// Memoria utilizada;
uint32_t freePages;

// Lista enlazada para administrar la memoria
t_list* pagesState;

int main(int argc, char **argv) {

	uint32_t errorCode = 0;

	// Se borra el archivo de log cada vez que se inicia el SWAP
	Comun_borrarArchivo(ARCHIVO_LOG);

	/************************************************************************/
	info(__func__, "Leyendo el archivo de configuracion");

	errorCode = readSwapConfigFile();
	if(errorCode == -1) {
		error(__func__, "No se pudo leer el archivo de configuracion");
		return -1;
	}

	info(__func__, "Lectura exitosa del archivo de configuraciones");
	/************************************************************************/

	// Inicializo en number_pages la cantidad de las paginas libres
	freePages = swap_config.number_pages;

	/************************************************************************/
	info(__func__, "Creando el archivo SWAP");

	errorCode = createSwapFile();
	if(errorCode == -1) {
		error(__func__, "No se pudo crear el archivo SWAP");
		return -1;
	}

	info(__func__, "Se genero exitosamente la particion SWAP");
	/************************************************************************/


	/************************************************************************/
	info(__func__, "Generando la lista de los procesos");

	processList = list_create();
	if(processList == NULL) {
		error(__func__, "No se pudo leer el archivo de configuracion");
		return -1;
	}

	info(__func__, "Se genero exitosamente la lista de los procesos");
	/************************************************************************/


	/************************************************************************/
	info(__func__, "Generando la lista para la administracion de la memoria SWAP");

	int i;

	pagesState = list_create();
	for(i = 0; i < swap_config.number_pages; i++) {
		t_frame_state* frameState = malloc(sizeof(t_frame_state));
		if(frameState == NULL) {
			error(__func__, "No se pudo crear la lista de los frames libres");
			return -1;
		}
		frameState->ocupado = 0;
		list_add(pagesState, frameState);
	}

	info(__func__, "Se genero exitosamente la lista para la administracion de la memoria SWAP");
	/************************************************************************/


	/************************************************************************/
	info(__func__, "Esperando la conexion del Administrador de Memoria");
	errorCode = esperarAdmMem();
	if(errorCode <= 0) {
		error(__func__, "No se pudo conectar al Administrador de Memoria");
		return -1;
	}

	/************************************************************************/

	// Hasta acá no llega nunca
	return 0;

}

int readSwapConfigFile() {

	debug(__func__, string_from_format("Verificando si existe el archivo de configuracion en la ruta : (%s)", CONFIG_FILE_ROUTE));

	int errorCode = Comun_existeArchivo(CONFIG_FILE_ROUTE);

	if(errorCode == -1) {
		Macro_Imprimir_Error("%s - No existe el archivo de configuracion - %s", __func__, CONFIG_FILE_ROUTE);
		return -1;
	}

	t_config* config = config_create(CONFIG_FILE_ROUTE);

	if(config_has_property(config, "PUERTO_ESCUCHA") == true) {
		Macro_ImprimirParaDebugConDatos("Leo el puerto de escucha");
		strcpy(swap_config.swap_port, config_get_string_value(config, "PUERTO_ESCUCHA"));
	}
	else {
		Macro_Imprimir_Error("El archivo de configuracion del SWAP no tiene el campo PUERTO_ESCUCHA");
		return -1;
	}

	if(config_has_property(config, "CANTIDAD_PAGINAS") == true) {
		Macro_ImprimirParaDebugConDatos("Leo la cantidad de paginas del archivo SWAP");
		swap_config.number_pages = config_get_int_value(config, "CANTIDAD_PAGINAS");
	}
	else {
		Macro_Imprimir_Error("El archivo de configuracion del SWAP no tiene el campo CANTIDAD_PAGINAS");
		return -1;
	}

	if(config_has_property(config, "TAMANIO_PAGINA") == true) {
		Macro_ImprimirParaDebugConDatos("Leo el tamaño de la paginas en el archivo SWAP");
		swap_config.page_size = config_get_int_value(config, "TAMANIO_PAGINA");
	}
	else {
		Macro_Imprimir_Error("El archivo de configuracion del SWAP no tiene el campo TAMANIO_PAGINA");
		return -1;
	}

	if(config_has_property(config, "RETARDO_COMPACTACION") == true) {
		Macro_ImprimirParaDebugConDatos("Leo el retardo de la compactación del archivo SWAP");
		swap_config.compactation_delay = config_get_int_value(config, "RETARDO_COMPACTACION");
	}
	else {
		Macro_Imprimir_Error("El archivo de configuracion del SWAP no tiene el campo RETARDO_COMPACTACION");
		return -1;
	}

	if(config_has_property(config, "RETARDO_SWAP") == true) {
		Macro_ImprimirParaDebugConDatos("Leo el retardo del SWAP");
		swap_config.swap_delay = config_get_int_value(config, "RETARDO_SWAP");
	}
	else {
		Macro_Imprimir_Error("El archivo de configuracion del SWAP no tiene el campo RETARDO_SWAP");
		return -1;
	}

	if(config_has_property(config, "NOMBRE_SWAP") == true) {
		Macro_ImprimirParaDebugConDatos("Leo el nombre del archivo SWAP");
		strcpy(swap_config.name_swap, config_get_string_value(config, "NOMBRE_SWAP"));
	}
	else {
		Macro_Imprimir_Error("El archivo de configuracion del SWAP no tiene el campo NOMBRE_SWAP");
		return -1;
	}

	Macro_ImprimirParaDebugConDatos("El puerto del SWAP:  	 		 %s", swap_config.swap_port);
	Macro_ImprimirParaDebugConDatos("El tamanio de pagina:        	 %d", swap_config.page_size);
	Macro_ImprimirParaDebugConDatos("La cantidad de paginas:      	 %d", swap_config.number_pages);
	Macro_ImprimirParaDebugConDatos("El nombre del archivo SWAP: 	 %s", swap_config.name_swap);
	Macro_ImprimirParaDebugConDatos("EL retardo de compactacion: 	 %d", swap_config.compactation_delay);
	Macro_ImprimirParaDebugConDatos("EL retardo del SWAP: 	 		 %d", swap_config.swap_delay);

	config_destroy(config);

	return 0;
}

int createSwapFile() {

	int errorCode = 0;
	int blockSize = 0;
	struct stat fi;

	uint32_t swapFileSize = swap_config.page_size * swap_config.number_pages;

	info(__func__, string_from_format("El tamanio del archivo SWAP a crear es (%d) bytes", swapFileSize));

	int fd = open(swap_config.name_swap, O_RDWR | O_CREAT | O_TRUNC, 0600);
	if (fd == -1) {
		error(__func__, "No se pudo abrir el archivo SWAP");
		return -1;
	}
	
	errorCode = stat("/", &fi);
	if(errorCode == -1) {
		error(__func__, "No se pudo conseguir la estructura del archivo");
		return -1;
	}

	blockSize = fi.st_blksize;

	info(__func__, string_from_format("El tamanio de los bloques en el Sistema Operativo es de (%d) bytes", blockSize));

	int cantBloques = (swapFileSize / blockSize) + (((swapFileSize % blockSize) == 0) ? 0 : 1);

	info(__func__, string_from_format("La cantidad de bloques en el archivo SWAP: (%d)", cantBloques));

	if (blockSize > swapFileSize) {
		blockSize = swapFileSize;
	}

	char* contBloqueArchivo = malloc(sizeof(char) * blockSize);
	if(contBloqueArchivo == NULL) {
		error(__func__, "No se pudo reservar la memoria");		
		return -1;
	}

	memset(contBloqueArchivo, CHARSWAP, blockSize);
	while (cantBloques > 0) {
		if (write(fd, contBloqueArchivo, blockSize) ==  -1) {
			error(__func__, string_from_format("No se pudo escribir el bloque (%d) en el archivo SWAP", cantBloques));
			return -1;
		}
		cantBloques--;
	}

	info(__func__, "Se completo la escritura del archivo SWAP");

	errorCode = close(fd);
	if(errorCode == -1) {
		error(__func__, "No se pudo cerrar el archivo creado");
		return -1;
	}

	Comun_LiberarMemoria((void**) &contBloqueArchivo);

	Macro_ImprimirParaDebugConDatos("El tamanio del archivo SWAP: %d", swapFileSize);

	return 0;
}

int orderWriteFrame(t_socket* socket_cliente) {
	int errorCode = 0;
	t_order_header orden_recibida, orden;
	t_pagina* frameRecibido = NULL;
	char* contenidoFrame = NULL;
	int timeTotal = 0;
	orden.sender_id = SWAP;

	clock_t timeStart = clock();

	info(__func__, "Se reciben los datos para la orden de la escritura del frame");

	// Recibo los datos del frame y el contenido del mismo que tengo que guardar
	errorCode = socket_cliente->Recibir(socket_cliente, &orden_recibida, (void**) &frameRecibido, DeSerializar_MemSWAP_Frame);
	Macro_Check_And_Handle_Error(errorCode == 0, "No se pudo recibir los datos del frame a escribir");

	errorCode = socket_cliente->Recibir(socket_cliente, &orden_recibida, (void**) &contenidoFrame, DeSerializar_MemSWAP_ContenidoFrame);
	Macro_Check_And_Handle_Error(errorCode == 0, "No se pudo recibir el contenido del frame");

	info(__func__, string_from_format("Datos del frame a escribir, PID del proceso: (%d), el numero del frame: (%d)", frameRecibido->idProceso, frameRecibido->idPagina));

	uint32_t writePosition = 0;
	int partitionSWAP = NULL;

	writePosition = getReadPosition(frameRecibido->idPagina, frameRecibido->idProceso);
	Macro_Check_And_Handle_Error(writePosition == -1, "No se pudo determinar la posicion de lectura de la pagina solicitada");

	debug(__func__, string_from_format("La posicion de la escritura en SWAP : (%d)", writePosition));

	partitionSWAP = open(swap_config.name_swap, O_RDWR, 0600);
	Macro_Check_And_Handle_Error(partitionSWAP == -1, "Problema al abrir la particion SWAP");

	// Verificar que el contenido se menor a page_size
	errorCode = lseek(partitionSWAP, writePosition, SEEK_SET);
	Macro_Check_And_Handle_Error(errorCode == -1, "No se pudo cambiar la posicion en el archivo SWAP");

	errorCode = write(partitionSWAP, contenidoFrame, swap_config.page_size);
	Macro_Check_And_Handle_Error(errorCode == -1, "No se pudo escribir el frame recibido");

	errorCode = close(partitionSWAP);
	Macro_Check_And_Handle_Error(errorCode != 0, "Problema al cerrar la particion SWAP");

	// Devuelvo el resultado de la operacion
	orden.comando = OK;
	
	info(__func__, "Se escribio exitosamente el frame enviado");

	clock_t timeFinish = clock();

	timeTotal = (int)round(((double)(timeFinish - timeStart)) / CLOCKS_PER_SEC);

	debug(__func__, string_from_format("Tiempo total de la escritura : (%d)", timeTotal));

	if(swap_config.swap_delay > timeTotal * 1000000)
		simulateDelay((((int)swap_config.swap_delay - timeTotal)/1000000));

	errorCode = socket_cliente->Enviar(socket_cliente, NULL, orden, NULL);
	Macro_Check_And_Handle_Error(errorCode == 0, "No se pudo enviar la confirmacion de la escritura");

	info(__func__, "Se envio la respuesta exitosa a Administrador de Memoria");

	free(frameRecibido);
	free(contenidoFrame);

	return 0;

  Error_Handler:

  	if(frameRecibido != NULL)
  		free(frameRecibido);

  	if(contenidoFrame != NULL)
  		free(contenidoFrame);

  	orden.comando = ERROR;
  	errorCode = 0;

  	while(errorCode == 0)
  		errorCode = socket_cliente->Enviar(socket_cliente, NULL, orden, NULL);

	return -1;
}

int orderReadFrame(t_socket* socket_cliente) {

	// Aca esta la comunicacion de un lado, escribir frame va a ser igual pero para el otro lado la serializacion
	int errorCode = 0;
	t_order_header orden, resultado_orden;
	t_pagina* framePedido = NULL;
	char* contenidoFrame = NULL;
	resultado_orden.sender_id = SWAP;
	clock_t timeStart = clock();
	
	info(__func__, "Se reciben los datos para la orden de la lectura del frame");

	errorCode = socket_cliente->Recibir(socket_cliente, &orden, (void**) &framePedido, DeSerializar_MemSWAP_Frame);
	Macro_Check_And_Handle_Error(errorCode == 0, "No se pudo recibir los datos del frame a escribir");

	info(__func__, string_from_format("Datos del frame a leer, PID del proceso: (%d), el numero del frame: (%d)", framePedido->idProceso, framePedido->idPagina));

	contenidoFrame = malloc(sizeof(char) * (swap_config.page_size+1));
	Macro_Check_And_Handle_Error(contenidoFrame == NULL, "No se pudo reservar la memoria para la pagina a leer");

	errorCode = getFrameSWAP(framePedido->idPagina, framePedido->idProceso, &contenidoFrame);
	Macro_Check_And_Handle_Error(errorCode == -1, "No se pudo obtener la pagina solicitada");

	// Cuando se leyo el contenido del frame correctamente
	resultado_orden.comando = OK;
	errorCode = socket_cliente->Enviar(socket_cliente, NULL, resultado_orden, NULL);
	Macro_Check_And_Handle_Error(errorCode == 0, "No se pudo enviar la pagina solicitada");
	
	info(__func__, "Se leyo exitosamente el frame solicitado");

	orden.sender_id = SWAP;
	orden.comando = CONTENIDO_FRAME;

	clock_t timeFinish = clock();

	int timeTotal = (int)round(((double)(timeFinish - timeStart)) / CLOCKS_PER_SEC);

	debug(__func__, string_from_format("Tiempo total de la lectura : (%d)", timeTotal));

	if(swap_config.swap_delay > timeTotal * 1000000)
		simulateDelay((((int)swap_config.swap_delay - timeTotal)/1000000));

	errorCode = socket_cliente->Enviar(socket_cliente, (void*) contenidoFrame, orden, Serializar_MemSWAP_ContenidoFrame);
	Macro_Check_And_Handle_Error(errorCode == 0, "No se pudo enviar la pagina solicitada");

	free(contenidoFrame);
	free(framePedido);

	return 0;

  Error_Handler:

  	if(contenidoFrame != NULL)
  		free(contenidoFrame);

  	if(framePedido != NULL)
  		free(framePedido);

  	errorCode = 0;
    resultado_orden.comando = ERROR;
	while(errorCode == 0)
		errorCode = socket_cliente->Enviar(socket_cliente, NULL, resultado_orden, NULL);

	return -1;

}

int orderStartProcess(t_socket* socket_cliente) {

	int errorCode = 0;
	t_order_header orden, resultado_orden;
	resultado_orden.sender_id = SWAP;
	uint32_t pageNumberStart = 0; // FIXME: Ruso aca habia un -1, pero por lo que vi, esta tendria que ser Global como variable
	t_proceso_memoria* newProcess = NULL;
	t_process_swap* newProcessSwap = NULL;
	t_frame_state* frameState = NULL;
	int i;

	info(__func__, "Se reciben los datos para la orden del inicio del proceso nuevo");

	// Se reciben los datos del proceso
	errorCode = socket_cliente->Recibir(socket_cliente, &orden, (void**) &newProcess , DeSerializar_MemSWAP_IniciarProceso);
	Macro_Check_And_Handle_Error(errorCode == 0, "Fallo la recepcion de datos para iniciar un proceso nuevo");

	info(__func__, string_from_format("Datos del frame a leer, PID del proceso: (%d), cantidad de paginas (%d)", newProcess->idProceso, newProcess->cantidadPaginasTotales));

	info(__func__, string_from_format("La cantidad de los frames libres: (%d)", freePages));

	// Verifico que haya espacio libre en SWAP para ubicar el proceso
	Macro_Check_And_Handle_Error(freePages < newProcess->cantidadPaginasTotales, "No hay espacio libre para guardar el proceso");

	// Busco una porcion de memoria para ubicarlo
	errorCode = findFreeSpace(&pageNumberStart, newProcess->cantidadPaginasTotales);
	Macro_Check_And_Handle_Error(errorCode == -1, "No se pudo compactar la memoria SWAP");

	// Preparo el proceso para ser guardado
	newProcessSwap = malloc(sizeof(t_process_swap));
	newProcessSwap->idProceso = newProcess->idProceso;
	newProcessSwap->pageNumberStart = pageNumberStart;
	newProcessSwap->totalPages = newProcess->cantidadPaginasTotales;

	info(__func__, string_from_format("Datos del proceso nuevo: PID - (%d), el frame inicial - (%d), cantidad de paginas - (%d)", newProcessSwap->idProceso, newProcessSwap->pageNumberStart, newProcessSwap->totalPages));

	Comun_LiberarMemoria((void**) &newProcess);

	// Lo agrego a la lista de los procesos ubicados en SWAP
	list_add(processList, newProcessSwap);

	// Resto el espacio libre total en SWAP
	freePages -= newProcessSwap->totalPages;

	debug(__func__, "Se setean los frames ocupados");
	// Seteo las paginas ocupadas en el bitarray
	for(i = 0; i < newProcessSwap->totalPages; i++) {
		frameState = (t_frame_state*)list_get(pagesState, pageNumberStart + i);
		frameState->ocupado = 1;
	}

	resultado_orden.comando = OK;

	errorCode = socket_cliente->Enviar(socket_cliente, NULL, resultado_orden, NULL);
	Macro_Check_And_Handle_Error(errorCode == 0, "No se pudo enviar la confirmacion");

	info(__func__, "Se envio la confirmacion del inicio del proceso nuevo");

	free(newProcess);

	return 0;

  Error_Handler:

    if(freePages < newProcess->cantidadPaginasTotales) {
    	resultado_orden.comando = FRAMES_OCUPADOS;
    } else {
    	resultado_orden.comando = ERROR;
    }

    if(newProcess != NULL)
    	free(newProcess);

    errorCode = 0;
	while(errorCode == 0)
		errorCode = socket_cliente->Enviar(socket_cliente, NULL, resultado_orden, NULL);

	return -1;

}

int orderFinishProcess(t_socket* socket_cliente) {

	#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
	int i, j, errorCode = 0;
	#pragma GCC diagnostic pop
	t_order_header resultado_orden;
	t_order_header orden;
	t_proceso_memoria *processID = NULL;
	t_process_swap* processSwap = NULL;
	t_frame_state* frameState = NULL;

	resultado_orden.sender_id = SWAP;

	info(__func__, "Se reciben los datos para la orden de la finalizacion del proceso");

	errorCode = socket_cliente->Recibir(socket_cliente, &orden, (void**) &processID , DeSerializar_MemSWAP_FinalizarProceso);
	Macro_Check_And_Handle_Error(errorCode == 0, "Fallo la recepcion de datos para finalizar el proceso");

	info(__func__, string_from_format("Datos del proceso a finalizar: PID - (%d)", processID->idProceso));
	
	for(i = 0; i < list_size(processList); i++) {
		processSwap = (t_process_swap*) list_get(processList, i);
		if(processSwap->idProceso == processID->idProceso) {
			info(__func__, "Se encontro el proceso en SWAP");
			for(j = 0; j < processSwap->totalPages; j++) {
				frameState = (t_frame_state*)list_get(pagesState, processSwap->pageNumberStart + i);
				frameState->ocupado = 0;
			}
			debug(__func__, "Se marcaron como libres los frames del proceso finalizado");
			freePages += processSwap->totalPages;
			list_remove(processList,i);
			free(processSwap);
			info(__func__, string_from_format("Cantidad de frames libres despues de la finalizacion del proceso - (%d)", freePages));
			break;
		}
	}

	info(__func__, "Se finalizo exitosamente el proceso solicitado");

	free(processID);
	
	resultado_orden.comando = OK;
	errorCode = socket_cliente->Enviar(socket_cliente, NULL, resultado_orden, NULL);

	info(__func__, "Se envio la confirmacion de la finalizacion del proceso");

	return 0;

  Error_Handler:

	resultado_orden.comando = ERROR;

	if(processID != NULL)
		free(processID);

	errorCode = 0;
	while(errorCode == 0)
		errorCode = socket_cliente->Enviar(socket_cliente, NULL, resultado_orden, NULL);

  	return -1;

}

int getFrameSWAP(uint32_t pageNumber, uint32_t processPID, char** frameData) {

	#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
	int errorCode = 0;
	#pragma GCC diagnostic pop

	uint32_t readPosition = 0;
	FILE* partitionSWAP = NULL;

	debug(__func__, "Se determina la posicion de la lectura");

	readPosition = getReadPosition(pageNumber, processPID);
	Macro_Check_And_Handle_Error(readPosition == -1, "No se pudo determinar la posicion de lectura de la pagina solicitada");

	Macro_ImprimirParaDebugConDatos("Se solicito la pagina '%d' y su ubicacion en swap es '%d'\n", pageNumber, readPosition);
	
	partitionSWAP = fopen(swap_config.name_swap, "r");
	Macro_Check_And_Handle_Error(partitionSWAP == NULL, "Problema al abrir la particion SWAP");

	int fileDescriptor = fileno(partitionSWAP);

	errorCode = lseek(fileDescriptor, readPosition, SEEK_SET);
	Macro_Check_And_Handle_Error(errorCode == -1, "No se pudo reposicionarse en el archivo SWAP");

	errorCode = read(fileDescriptor, *frameData, swap_config.page_size);
	(*frameData)[swap_config.page_size] = '\0';
	Macro_Check_And_Handle_Error(errorCode == -1, "No se pudo leer la pagina solicitada");

	Macro_ImprimirParaDebugConDatos("El contenido de la pagina es '%s'\n", *frameData);

	errorCode = fclose(partitionSWAP);
	Macro_Check_And_Handle_Error(errorCode != 0, "Problema al cerrar la particion SWAP");

	return 0;

  Error_Handler:

	return -1;

}

int getReadPosition(uint32_t pageNumber, uint32_t processPID) {

	t_process_swap* processSwap = NULL;
	int i;

	for(i = 0; i < list_size(processList); i++) {
		processSwap = (t_process_swap*) list_get(processList, i);
		if(processSwap->idProceso == processPID && processSwap->totalPages > pageNumber) {
			return (processSwap->pageNumberStart + pageNumber) * swap_config.page_size;
		}
	}

	return -1;
}

int findFreeSpace(uint32_t* pageNumberStart, uint32_t processSize) {

	int errorCode = 0;
	uint32_t pageNumberStartTemp = 0;
	uint32_t sizeCounter = 0;
	bool startNewSpace = true;
	t_frame_state* frameState = NULL;
	int i;

	info(__func__, "Se realiza la busqueda de un hueco suficientemente grande");

	for(i = 0; i < list_size(pagesState); i++) {
		frameState = (t_frame_state*)list_get(pagesState, i);
		if(!frameState->ocupado) {
			if(startNewSpace) {
				startNewSpace = false;
				pageNumberStartTemp = i;
			}
			sizeCounter++;
			if(sizeCounter == processSize) {
				debug(__func__, string_from_format("Se encontro el hueco, la posicion inicial es (%d)", pageNumberStartTemp));
				i = -1;
				break;
			}
		}
		else {
			startNewSpace = true;
			sizeCounter = 0;
		}
	}

	if(i == swap_config.number_pages) {
		info(__func__, "No se encontro un hueco suficientemente grande, se procede a compactar los datos");
		errorCode = defragmentSWAP();
		if(errorCode == -1)
			return -1;
		*pageNumberStart = swap_config.number_pages - freePages;
	}
	else
		*pageNumberStart = pageNumberStartTemp;

	return 0;

}

int defragmentSWAP() {

	int errorCode = 0;
	uint32_t processPositionStart = 0;
	bool freePositionStartFound = false;
	int i, freePosition;
	t_frame_state* frameState = NULL;
	t_process_swap* processToMove = NULL;
	sigset_t signalSet;
	int timeTotal = 0;

	clock_t timeStart = clock();

	errorCode = sigfillset(&signalSet);
	if(errorCode != 0) {
		error(__func__, "No se pudieron settear los seniales a bloquear");
		return -1;
	}

	errorCode = sigprocmask(SIG_BLOCK, &signalSet, NULL);
	if(errorCode == -1) {
		error(__func__, "No se pudo bloquear los seniales antes de la compactacion");
		return -1;
	}

	for(i = 0; i < list_size(pagesState); i++) {
		if(!freePositionStartFound) {
			frameState = (t_frame_state*)list_get(pagesState, i);
			if(!frameState->ocupado) {
				debug(__func__, string_from_format("Se encontro la posicion libre - (%d)", i));
				freePosition = i;
				freePositionStartFound = true;
			}
		}
		else {
			if(freePositionStartFound) {
				if(frameState->ocupado) {
					processPositionStart = i;

					processToMove = findProcess(processPositionStart);
					if(processToMove == NULL) {
						error(__func__, string_from_format("No se encontro un proceso en SWAP con la posicion inicial (%d)", processPositionStart));
						return -1;
					}

					info(__func__, string_from_format("Los datos del proceso a mover: PID - (%d), cantidad de paginas - (%d)", processToMove->idProceso, processToMove->totalPages));

					errorCode = moveProcess(processToMove, freePosition);
					if(errorCode == -1) {
						error(__func__, "No se pudo mover el proceso");
						return -1;
					}

					freePosition = freePosition + processToMove->totalPages;
					processToMove = NULL;
				}
			}
		}
	}


	errorCode = sigprocmask(SIG_UNBLOCK, &signalSet, NULL);
	if(errorCode == -1) {
		error(__func__, "No se pudo desbloquear los seniales despues de la compactacion");
		return -1;
	}

	clock_t timeFinish = clock();

	timeTotal = (int)round(((double)(timeFinish - timeStart)) / CLOCKS_PER_SEC);

	debug(__func__, string_from_format("Tiempo real de la compactacion : (%d)", timeTotal));

	if(swap_config.compactation_delay > timeTotal * 1000000)
		simulateDelay((((int)swap_config.compactation_delay - timeTotal)/1000000));

	return 0;
}

int moveProcess(t_process_swap* processToMove, int freePositionStart) {

	int i;
	int errorCode = 0;
	char* frameData = NULL;
	t_frame_state* frameState = NULL;
	FILE* partitionSWAP = NULL;

	partitionSWAP = fopen(swap_config.name_swap, "r+");
	if(partitionSWAP == NULL) {
		error(__func__, "No se pudo abrir el archivo SWAP");
		return -1;
	}

	int fileDescriptor = fileno(partitionSWAP);
	if(fileDescriptor == -1) {
		error(__func__, "No se pudo obtener el descriptor del archivo SWAP");
		return -1;
	}

	for(i = 0; i < processToMove->totalPages; i++) {
		errorCode = lseek(fileDescriptor, (processToMove->pageNumberStart + i) * swap_config.page_size, SEEK_SET);
		if(errorCode == -1) {
			error(__func__, string_from_format("No se pudo ubicar el puntero en la posicion (%d)", (processToMove->pageNumberStart + i) * swap_config.page_size));
			return -1;
		}
		errorCode = read(fileDescriptor, frameData, swap_config.page_size);
		if(errorCode == -1) {
			error(__func__, string_from_format("No se pudo leer los datos de la posicion (%d)", (processToMove->pageNumberStart + i) * swap_config.page_size));
			return -1;
		}

		errorCode = lseek(fileDescriptor, (freePositionStart + i) * swap_config.page_size, SEEK_SET);
		if(errorCode == -1) {
			error(__func__, string_from_format("No se pudo ubicar el puntero en la posicion (%d)", (freePositionStart + i) * swap_config.page_size));
			return -1;
		}
		errorCode = write(fileDescriptor, frameData, swap_config.page_size);
		if(errorCode == -1) {
			error(__func__, string_from_format("No se pudo escribir los datos en la posicion (%d)", (freePositionStart + i) * swap_config.page_size));
			return -1;
		}

		frameState = (t_frame_state*)list_get(pagesState, freePositionStart + i);
		frameState->ocupado = 1;
		frameState = (t_frame_state*)list_get(pagesState, processToMove->pageNumberStart + i);
		frameState->ocupado = 0;

		processToMove->pageNumberStart = freePositionStart;
	}

	errorCode = fclose(partitionSWAP);
	if(errorCode != 0) {
		return -1;
	}

	return 0;

}

t_process_swap* findProcess(int processPositionStart) {

	int i;
	t_process_swap* process = NULL;

	for(i = 0; i < list_size(processList); i++) {
		process = (t_process_swap*) list_get(processList, i);
		if(process->pageNumberStart == processPositionStart)
			return process;
	}

	return NULL;

}

int simulateDelay(int timeSleep) {
	int i;

	debug(__func__, string_from_format("I'm going to sleeeeep for a (%d) seconds", timeSleep));

	for (i = 1; i <= timeSleep; i++)
	{
	    usleep(1000000);
	    fflush(stdout);
	}
	return 0;
}


int esperarAdmMem() {
	int iReturn = 0;
	SOCKET_crear(socket_swap);

	t_order_header orden;
	iReturn = socket_swap->Escuchar(socket_swap, swap_config.swap_port);
	if (iReturn <= 0) {
		//Ocurrio un error y lo devuelvo
		iReturn = socket_swap->soc_err;
		SOCKET_destruir(socket_swap);
		return iReturn;
	}
	while (1) {
		SOCKET_crear(socket_cliente);
		iReturn = socket_cliente->Aceptar_Cliente(socket_swap, socket_cliente);
		if (iReturn <= 0) {
			//Ocurrio un error y lo devuelvo
			iReturn = socket_cliente->soc_err;
			SOCKET_destruir(socket_cliente);
			SOCKET_destruir(socket_swap);
			return iReturn;
		}

		while (socket_cliente->descriptorSocket > 0) {
			socket_cliente->Recibir(socket_cliente, &orden, NULL , NULL);

			if (orden.comando == INICIAR_PROCESO) {
				orderStartProcess(socket_cliente);
			}
			if (orden.comando == LEER_FRAME) {
				orderReadFrame(socket_cliente);
			}
			if (orden.comando == ESCRIBIR_FRAME) {
				orderWriteFrame(socket_cliente);
			}
			if (orden.comando == FINALIZAR_PROCESO) {
				orderFinishProcess(socket_cliente);
			}
			// FIXME: Ruso! sea que atras de cada instruccion se desconecta el socket?
			socket_cliente->Desconectar(socket_cliente);
		}
		// FIXME: Ruso! que hace ese sleep(1)? no deberia haber otro adentro del while chiquito?
		sleep(1);
		if (socket_cliente->descriptorSocket == 0) {
			SOCKET_destruir(socket_cliente);
		}
	}

	SOCKET_destruir(socket_swap);
	return iReturn;
}

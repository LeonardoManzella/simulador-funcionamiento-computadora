#include "../headers/admin_communication.h"

#include <stddef.h>
#include <stdint.h>

#include "../../bibliotecas/serializacion.h"
#include "../../bibliotecas/sockets.h"
#include "../../lib/commons/log.h"
#include "../headers/admin_configuration.h"
#include "../../bibliotecas/comun.h"

int conectar_swap() {
	int iReturn = 0;
	char ipSwap[16];
	char puertoSwap[6];
	conseguirIpPuertoSwap(ipSwap, puertoSwap);

	Macro_ImprimirParaDebugConDatos("Conectando con SWAP");
	Macro_ImprimirParaDebugConDatos("Datos de SWAP: %s (%s)", ipSwap, puertoSwap);

	SOCKET_crear(socket_admmem_swap);
	socket_admmem_swap->Conectar(socket_admmem_swap, ipSwap, puertoSwap);
	if (socket_admmem_swap->descriptorSocket > 0) {
		// El SWAP estaba conectado
		Macro_ImprimirParaDebugConDatos("Conexion Exitosa");
		// Envio datos para evitar error en Swap
		t_order_header OrdenAEnviar;
		OrdenAEnviar.sender_id = ADMMEM;
		OrdenAEnviar.comando = OK;
		socket_admmem_swap->Enviar(socket_admmem_swap, NULL, OrdenAEnviar, NULL);
		socket_admmem_swap->Desconectar(socket_admmem_swap);
		iReturn = 1;
	}

	Macro_ImprimirParaDebugConDatos("Cerrando Conexion");
	SOCKET_destruir(socket_admmem_swap);

	return iReturn;
}

int iniciar_proceso_en_swap(t_proceso_memoria* procesoNuevo) {
	int returnValue = 1;

	char ipSwap[16];
	char puertoSwap[6];
	conseguirIpPuertoSwap(ipSwap, puertoSwap);

	SOCKET_crear(socket_con_swap);

	socket_con_swap->Conectar(socket_con_swap, ipSwap, puertoSwap);
	t_order_header orden_a_swap, orden_recibida_swap;
	orden_a_swap.sender_id = ADMMEM;
	orden_a_swap.comando = INICIAR_PROCESO;

	uint32_t resultado;

	resultado = socket_con_swap->Enviar(socket_con_swap, NULL, orden_a_swap, NULL);
	Macro_Check_And_Handle_Error(resultado <= 0, "No se pudo enviar la Orden al swap");

	resultado = socket_con_swap->Enviar(socket_con_swap, (void*) procesoNuevo, orden_a_swap,
			Serializar_MemSWAP_IniciarProceso);
	Macro_Check_And_Handle_Error(resultado <= 0, "No se pudo enviar la datos de orden al swap");

	socket_con_swap->Recibir(socket_con_swap, &orden_recibida_swap, NULL, NULL);
	Macro_Check_And_Handle_Error(resultado <= 0, "No se recibio respuesta del swap");
	if (orden_recibida_swap.sender_id == SWAP) {
		if (orden_recibida_swap.comando == OK) {
			returnValue = 1;

			Macro_ImprimirParaDebug("\n swap iniciar ok \n\r\n\r");
		} else if (orden_recibida_swap.comando == FRAMES_OCUPADOS)  {
			returnValue = -2;

			Macro_ImprimirParaDebug("\n swap sin memoria disponible \n\r\n\r");
		} else {
			returnValue = -1;

			Macro_ImprimirParaDebug("\n swap no pudo iniciar el proceso \n\r\n\r");
		}
	} else {
		// Paso algo muy raro, recibi una orden que no es de SWAP cuando deberia de serlo
		returnValue = -1;
		Macro_ImprimirParaDebug("swap error loco\n\r\n\r");
	}

	socket_con_swap->Desconectar(socket_con_swap);
	SOCKET_destruir(socket_con_swap); // para liberar la conexion y la memoria

	return returnValue;

	Error_Handler:
	//loguear(LOG_LEVEL_ERROR, __func__, "No se pudo enviar Iniciar mProc con SWAP\n");
	socket_con_swap->Desconectar(socket_con_swap);
	SOCKET_destruir(socket_con_swap);
	return -1;
}

int leer_pagina_de_swap(t_pagina* framePedido, char** paginaARetornar) {
	// Inicializo todas las variables
	char ipSwap[16];
	char puertoSwap[6];
	t_order_header orden, orden_recibida;

	// Obtengo el puerto e ip de Swap
	conseguirIpPuertoSwap(ipSwap, puertoSwap);

	uint32_t resultado;

	// Creo el socket con SWAP
	SOCKET_crear(socket_con_swap);
	socket_con_swap->Conectar(socket_con_swap, ipSwap, puertoSwap);

	// Especifico la orden a enviar
	orden.sender_id = ADMMEM;
	orden.comando = LEER_FRAME;

	// Envio la orden, y la orden con los datos, se envia dos veces
	resultado = socket_con_swap->Enviar(socket_con_swap, NULL, orden, NULL);
	Macro_Check_And_Handle_Error(resultado <= 0, "No se pudo enviar la orden leer al swap");

	socket_con_swap->Enviar(socket_con_swap, (void*) framePedido, orden, Serializar_MemSWAP_Frame);
	Macro_Check_And_Handle_Error(resultado <= 0, "No se pudo enviar datos de la orden leer al swap");

	// Espero que SWAP haga lo que tiene que hacer y...
	// Recibo el Ok o error de SWAP que proceso la orden
	socket_con_swap->Recibir(socket_con_swap, &orden_recibida, NULL, NULL);
	if (orden_recibida.comando == ERROR) {
		resultado = -1;
		Macro_Check_And_Handle_Error(resultado <= 0, "El swap devolvio error");
	}

	char *frameLocal = NULL;
	resultado = socket_con_swap->Recibir(socket_con_swap, &orden_recibida, (void**) &frameLocal,
			DeSerializar_MemSWAP_ContenidoFrame);
	Macro_Check_And_Handle_Error(resultado <= 0, "No se pudo recibir datos de la orden leer del swap");
	*paginaARetornar = frameLocal;

	// Destruyo la conexion con SWAP, si falla en este punto, es problema del Adm. de Memoria y al Ruso le chupa un huevo
	socket_con_swap->Desconectar(socket_con_swap);
	SOCKET_destruir(socket_con_swap); // para liberar la conexion y la memoria

	if (!(orden.sender_id == SWAP) && (orden.comando = CONTENIDO_FRAME)) {
		resultado = -1;
		Macro_Check_And_Handle_Error(resultado <= 0, "El swap devolvio un resultado inesperado");
	}

	return 1;

	Error_Handler: error(__func__, "No se pudo enviar Iniciar mProc con SWAP\n");
	socket_con_swap->Desconectar(socket_con_swap);
	SOCKET_destruir(socket_con_swap);
	return -1;
}

int finalizar_proceso_en_swap(t_proceso_memoria* proceso) {
	int returnValue = 1;

	char ipSwap[16];
	char puertoSwap[6];
	conseguirIpPuertoSwap(ipSwap, puertoSwap);

	SOCKET_crear(socket_con_swap);

	socket_con_swap->Conectar(socket_con_swap, ipSwap, puertoSwap);
	t_order_header orden_a_swap, orden_recibida_swap;
	orden_a_swap.sender_id = ADMMEM;
	orden_a_swap.comando = FINALIZAR_PROCESO;

	uint32_t resultado;

	resultado = socket_con_swap->Enviar(socket_con_swap, NULL, orden_a_swap, NULL);
	Macro_Check_And_Handle_Error(resultado <= 0, "No se pudo enviar la Orden al swap");

	resultado = socket_con_swap->Enviar(socket_con_swap, (void*) proceso, orden_a_swap,
			Serializar_MemSWAP_IniciarProceso);
	Macro_Check_And_Handle_Error(resultado <= 0, "No se pudo enviar la datos de orden al swap");

	socket_con_swap->Recibir(socket_con_swap, &orden_recibida_swap, NULL, NULL);
	Macro_Check_And_Handle_Error(resultado <= 0, "No se recibio respuesta del swap");
	if (orden_recibida_swap.sender_id == SWAP) {
		if (orden_recibida_swap.comando == OK) {
			returnValue = 1;
		} else {
			returnValue = -1;
		}
	} else {
		// Paso algo muy raro, recibi una orden que no es de SWAP cuando deberia de serlo
		error(__func__, "Se recibio algo que no es de SWAP cuando se esperaba respuesta");
		returnValue = -1;
	}

	socket_con_swap->Desconectar(socket_con_swap);
	SOCKET_destruir(socket_con_swap); // para liberar la conexion y la memoria

	return returnValue;

	Error_Handler: error(__func__, "No se pudo enviar Iniciar mProc con SWAP\n");
	socket_con_swap->Desconectar(socket_con_swap);
	SOCKET_destruir(socket_con_swap);
	return -1;
}

int escribir_pagina_de_swap(t_pagina* frameEnviado, char* contenidoFrame) {
	int returnValue = 1;
	// Inicializo todas las variables
	char ipSwap[16];
	char puertoSwap[6];
	t_order_header orden, orden_recibida;

	// Obtengo el puerto e ip de Swap
	conseguirIpPuertoSwap(ipSwap, puertoSwap);

	// Creo el socket con SWAP
	SOCKET_crear(socket_con_swap);
	socket_con_swap->Conectar(socket_con_swap, ipSwap, puertoSwap);

	// Especifico la orden a enviar
	orden.sender_id = ADMMEM;
	orden.comando = ESCRIBIR_FRAME;
	uint32_t resultado;

	// Envio la orden, y la orden con los datos, se envia dos veces y ademas envio el contenido del Frame
	// Porque en realidad el Adm. de memoria, sabe que tiene que escribir el contenido de esa pagina porque se modifico
	resultado = socket_con_swap->Enviar(socket_con_swap, NULL, orden, NULL);
	Macro_Check_And_Handle_Error(resultado <= 0, "No se pudo enviar la Orden al swap");

	resultado = socket_con_swap->Enviar(socket_con_swap, (void*) frameEnviado, orden, Serializar_MemSWAP_Frame);
	Macro_Check_And_Handle_Error(resultado <= 0, "No se pudo enviar datos de pagina al swap");

	socket_con_swap->Enviar(socket_con_swap, (void*) contenidoFrame, orden, Serializar_MemSWAP_ContenidoFrame);
	Macro_Check_And_Handle_Error(resultado <= 0, "No se pudo enviar datos de contenido al swap");

	// Espero que SWAP haga lo que tiene que hacer y...
	// Recibo el Ok de SWAP que proceso la orden
	resultado = socket_con_swap->Recibir(socket_con_swap, &orden_recibida, NULL, NULL);
	Macro_Check_And_Handle_Error(resultado <= 0, "No se pudo recibir respuesta");

	if (orden_recibida.sender_id == SWAP) {
		if (orden_recibida.comando == OK) {
			returnValue = 1;
		} else {
			returnValue = -1;
		}
	} else {
		// Paso algo muy raro, recibi una orden que no es de SWAP cuando deberia de serlo
		error(__func__, "Se recibio algo que no es de SWAP cuando se esperaba respuesta");
		returnValue = -1;
	}

	// Destruyo la conexion con SWAP, si falla en este punto, es problema del Adm. de Memoria y al Ruso le chupa un huevo
	socket_con_swap->Desconectar(socket_con_swap);
	SOCKET_destruir(socket_con_swap); // para liberar la conexion y la memoria

	return returnValue;

	Error_Handler: error(__func__, "No se pudo enviar Iniciar mProc con SWAP\n");
	socket_con_swap->Desconectar(socket_con_swap);
	SOCKET_destruir(socket_con_swap);
	return -1;

}


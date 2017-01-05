#include "cpu.h"
#include "cpu_parametros_config.h"
#include "cpu_utiles.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include "../bibliotecas/comun.h"
#include "../bibliotecas/logs.h"
#include "../bibliotecas/sockets.h"
#include "../bibliotecas/serializacion.h"
#include "../bibliotecas/structs.h"
#include "../lib/commons/collections/list.h"
#include "../lib/commons/process.h"
#include "../lib/commons/string.h"

//prototipos
t_list* calcularUsoCPU_Array(int cantCPUs);

void mostrarContenidoPCB(int nroCPU, t_PCB* pcb);

void inicializarArrayUsoCPU(int cantidadCPUs);

int conectarPlanificador(char** puertoCPUAdmin);

int conectarAdminMemoria();

int avisarCPUDisponible(int nroCPU);

void threadCPU();

void threadServer(char* puerto);

void threadMonitoreoUso(int cantidadCPUs);

int levantarThreadServidor(char* puerto);

int levantarThreadEstadistica(int cantidadCPUs);

int levantarThreadsCPU();

void cambiarEstadoUsoCPU(int nroCPU, int estado);

void ordenPedirUsoCPU();

int obtenerNroCPU();

int ejecutarInstruccion(char* instruccion, t_list* listaResultados, uint32_t mProcID, char** errorDesc, char** tiempoBloqueo);

int ejecutarInstIniciar(char* instruccion, int mProcID, char** resultado, char** errorDesc);

int ejecutarInstLeer(char* instruccion, int mProcID, char** resultado, char** errorDesc);

int ejecutarInstEscribir(char* instruccion, int mProcID, char** resultado, char** errorDesc);

int ejecutarInstEntradaSalida(char* instruccion, int mProcID, char** resultado, char** errorDesc, char** tiempoBloqueo);

int ejecutarInstFinalizar(int mProcID, char** resultado, char** errorDesc);

char* obtenerInstruccion(char* mCod, uint32_t pc);

int enviarResultadoEjecucionRafaga(int comandoAEnviar, t_PCB* pcb, char* errorDesc, int nroCPU, t_list* listaResultados, char* tiempoBloqueo);

int operacionEjecutarRafaga(int nroCPU, t_PCB* pcb);

//variables globales

t_cpu_config cpuConfig;

int contadorNroCPU = 0;

//array para llenar estadísticas de uso
int arrayUsoCPU[STRING_BUFFER_SIZE][CPU_USAGE_TIME_CALC_PARAM] = {{0}};

//asigno un array de uso de cpu con un nro muy grande
int arrayEstadoCPU[STRING_BUFFER_SIZE] = {0};

pthread_mutex_t mutexNroCPU = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t mutexArrayUsoCPU = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t mutexLecturaMCod = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t mutexArrayEstadoCPU = PTHREAD_MUTEX_INITIALIZER;

//función principal
int main() {

	char* puertoCPUAdmin = NULL;

	//borro log anterior
	Comun_borrarArchivo(ARCHIVO_LOG);

	info(__func__, "Main - Iniciando CPU...", cpuConfig.imprimirPorConsola);

	//levanto archivo de config
	info(__func__, "Main - Abro archivo config", cpuConfig.imprimirPorConsola);

	cpuConfig = abrirArchivoConfig(ARCHIVO_CONFIG);

	Macro_Check_And_Handle_Error(false == cpuConfig.esValida, "Main - Error al abrir archivo de configuración");

	debug(__func__, "Main - Config OK");

	//conecto a admin de memoria para verificar que esté levantado
	info(__func__, "Main - Me conecto al admin. de memoria", cpuConfig.imprimirPorConsola);

	Macro_Check_And_Handle_Error(conectarAdminMemoria() <= 0, "Main - Error al conectarse al admin. de memoria [%s]", cpuConfig.ip_memoria);

	//conecto a planificador para verificar que esté levantado
	info(__func__, "Main - Me conecto al planificador", cpuConfig.imprimirPorConsola);

	Macro_Check_And_Handle_Error(conectarPlanificador(&puertoCPUAdmin) <= 0, "Main - Error al conectarse al planificador [%s]", cpuConfig.ip_planificador);

	//levanto el thread servidor
	info(__func__, "Main - Levanto thread servidor", cpuConfig.imprimirPorConsola);

	Macro_Check_And_Handle_Error(-1 == levantarThreadServidor(puertoCPUAdmin), "Main - Error al levantar thread servidor");

	//levanto thread estadísticas
	info(__func__, "Main - Levanto thread de estadisticas", cpuConfig.imprimirPorConsola);

	Macro_Check_And_Handle_Error(-1 == levantarThreadEstadistica(cpuConfig.cantidad_hilos), "Main - Error al levantar thread de estadisticas");

	//levanto cada thread de cpu
	info(__func__, "Main - Levanto threads de cpu", cpuConfig.imprimirPorConsola);

	Macro_Check_And_Handle_Error(-1 == levantarThreadsCPU(), "Main - Error al levantar threads cpu");

	//no debería terminar ya que me quedo esperando infinitamente a que finalicen los hijos
	exit(EXIT_FAILURE);//inalcanzable

	Error_Handler:

	//destruyo mutexes
	pthread_mutex_destroy(&mutexArrayUsoCPU);
	pthread_mutex_destroy(&mutexNroCPU);
	pthread_mutex_destroy(&mutexLecturaMCod);
	pthread_mutex_destroy(&mutexArrayEstadoCPU);

	exit(EXIT_FAILURE);
}

/**
 * Función que levanta el hilo de servidor que escucha en el puerto configurado.
 */
int levantarThreadServidor(char* puerto) {

	pthread_t tid;

	if(0 != pthread_create(&tid, NULL, (void*) &threadServer, puerto)) {
		//error
		return -1;
	}

	return 1;
}

/**
 * Función que levanta el hilo que calcula las estadísticas de uso.
 */
int levantarThreadEstadistica(int cantidadCPUs) {

	pthread_t tid;

	if(0 != pthread_create(&tid, NULL, (void*) &threadMonitoreoUso,(void*) cantidadCPUs)) {
		//error
		return -1;
	}

	return 1;
}

/**
 * Función que levanta todos los hilos de cpu y se queda esperando a que finalicen
 */
int levantarThreadsCPU() {

	int i, result = 0;
	pthread_t tid[cpuConfig.cantidad_hilos];
	void* threadResult[cpuConfig.cantidad_hilos];

	for (i = 0; i < cpuConfig.cantidad_hilos; ++i) {
		//creo un thread
		if(0 != pthread_create(&(tid[i]), NULL, (void*) &threadCPU, NULL)) {
			//error al crear threads
			return -1;
		}
	}

	//joineo a los threads para que no termine el main antes de que terminen los hijos
	for (i = 0; i < cpuConfig.cantidad_hilos; ++i) {

		result = pthread_join(tid[i], &threadResult[i]);

		Macro_CheckError(result, "error al realizar join al thread del cpu %i", i + 1);
	}

	return 1;
}

/**
 * Función que se conecta al planificador para validar que esté levantado
 */
int conectarPlanificador(char** puertoCPUAdmin) {

	int result = 1;
	t_order_header orden;
	t_order_header orden_recibida;
	SOCKET_crear(socket_cpu_planif);

	result = socket_cpu_planif->Conectar(socket_cpu_planif, cpuConfig.ip_planificador , cpuConfig.puerto_planificador);

	if (result <= 0) {
		//error de conexión
		error(__func__, "error al conectar con el Planificador");
		result = -1;
	}

	//conectado OK

	// Obtengo el puerto del Thread de CPU (con funcion Socket)
	*puertoCPUAdmin = malloc(sizeof(char) * LONGITUD_CHAR_PUERTOS);

	if(NULL == *puertoCPUAdmin) {
		//error en malloc
		error(__func__, "error en malloc");
		result = -1;
	}

	strcpy(*puertoCPUAdmin, string_itoa(socket_cpu_planif->Puerto_Cliente));

	orden.sender_id = CPU;
	orden.comando = CONECTAR_CPU_ADMIN;

	result = socket_cpu_planif->Enviar(socket_cpu_planif, NULL, orden, NULL);

	if(result <= 0) {

		error(__func__, "error al conectarse al Planificador.");
		return -1;
	}

	//espero el OK del planificador
	result = socket_cpu_planif->Recibir(socket_cpu_planif, &orden_recibida, NULL, NULL);

	if(result <= 0) {
		//error
		error(__func__, "error al recibir OK del pĺanificador.");
		SOCKET_destruir(socket_cpu_planif);
		return -1;
	}

	if(OK != orden_recibida.comando) {
		//respuesta no OK
		error(__func__, "error al recibir OK del planificador.");
		SOCKET_destruir(socket_cpu_planif);
		return -1;
	}

	result = socket_cpu_planif->Desconectar(socket_cpu_planif);

	if(result <= 0) {

		error(__func__, "error al desconectarse del Planificador.Continuo...");
	}

	SOCKET_destruir(socket_cpu_planif);


	return result;
}

/**
 * Función que se conecta al admin de memoria para validar que esté levantado
 */
int conectarAdminMemoria() {

	int result = 1;
	t_order_header orden;
	t_order_header orden_recibida;

	SOCKET_crear(socket_cpu_admmem);

	result = socket_cpu_admmem->Conectar(socket_cpu_admmem, cpuConfig.ip_memoria , cpuConfig.puerto_memoria);

	if (result <= 0) {
		//error de conexión
		error(__func__, "error al conectar con el admin de memoria");

		result = socket_cpu_admmem->soc_err;
	}

	//conectado ok - envío orden

	orden.comando = CONECTAR_CPU_ADMIN;
	orden.sender_id = CPU;

	//envío orden
	result = socket_cpu_admmem->Enviar(socket_cpu_admmem, NULL, orden, NULL);

	if (result <= 0) {
		//error de conexión
		error(__func__ , "error al enviar orden a admin de memoria.");
		return -1;
	}

	//espero el OK del admin
	result = socket_cpu_admmem->Recibir(socket_cpu_admmem, &orden_recibida, NULL, NULL);

	if(result <= 0) {
		//error
		error(__func__, "error al recibir OK del admin de memoria.");
		return -1;
	}

	if(OK != orden_recibida.comando) {
		//respuesta no OK
		error(__func__, "error al recibir OK del admin de memoria.");
		return -1;
	}

	result = socket_cpu_admmem->Desconectar(socket_cpu_admmem);

	if(result <= 0) {
		//error
		error(__func__, "error al desconectarse del admin de memoria. Continuo.");
	}

	SOCKET_destruir(socket_cpu_admmem);

	return result;
}

/**
 * Thread servidor - escucha en un puerto
 */
void threadServer(char* puerto) {

	int result = 0;

	SOCKET_crear(socket_cpu);

	t_order_header orden;

	//socket_cpu->Escuchar(socket_cpu, cpuConfig.puerto_escucha_cpu);
	socket_cpu->Escuchar(socket_cpu, puerto);

	debug(__func__, string_from_format("cpu admin - esperando conexiones en puerto %s...", puerto));

	while (1) {

		SOCKET_crear(socket_cliente);

		socket_cliente->Aceptar_Cliente(socket_cpu, socket_cliente);

		while (socket_cliente->descriptorSocket > 0) {

			result = socket_cliente->Recibir(socket_cliente, &orden, NULL, NULL);

			if(result > 0) {//recibí algo

				if(PLANIFICADOR == orden.sender_id) {
					switch (orden.comando) {

						case USO_CPU:
							//orden de uso de cpu
							ordenPedirUsoCPU(socket_cliente);
							break;

						default:
							//recibí una orden de otro lado, la ignoro
							warn(__func__, string_from_format("cpu admin - se recibio una orden no soportada (%d)", orden.comando));
							break;
					}
				}


			} else {
				//recibí una orden de otro lado, la ignoro
				warn(__func__, "cpu admin - se recibio una orden que no venia del planificador. La ignoro.");
			}

			//usleep(1000000);
		}

		if (socket_cliente->descriptorSocket == 0) {
			SOCKET_destruir(socket_cliente);
		}
	}
	//incalcanzable..
	SOCKET_destruir(socket_cpu);
}

/**
 * Función a ejecutar por cada thread de cpu
 */
void threadCPU() {

	int result = -1;

	//obtengo idCPU
	int nroCPU = obtenerNroCPU();

	info(__func__, string_from_format("cpu %i - iniciando thread cpu...", nroCPU), cpuConfig.imprimirPorConsola);

	//aviso al planificador que estoy disponible
	char* puertoCPU = string_itoa(avisarCPUDisponible(nroCPU));

	t_PCB* pcb;

	//quedo a la espera de conexiones (el planificador ya me puede enviar cosas)
	SOCKET_crear(socket_cpu);

	t_order_header orden;

	result = socket_cpu->Escuchar(socket_cpu, puertoCPU);

	if(result <= 0) {
		//error al ponerme a escuchar
		error(__func__, string_from_format("cpu %i - error al ponerme a escuchar en el puerto %d", nroCPU, puertoCPU));
		return;
	}

	debug(__func__, string_from_format("cpu %i - escuchando en el puerto %s. Espero conexiones entrantes.", nroCPU, puertoCPU));

	//infinitamente
	while (1) {

		SOCKET_crear(socket_planif);

		result = socket_planif->Aceptar_Cliente(socket_cpu, socket_planif);

		if(result <= 0) {
			//error al aceptar cliente
			error(__func__, string_from_format("cpu %i - error al aceptar un cliente", nroCPU));
			SOCKET_destruir(socket_planif);
			continue;
		}

		result = socket_planif->Recibir(socket_planif, &orden, NULL , NULL);

		if(result <= 0) {
			//error al recibir una orden
			error(__func__, string_from_format("cpu %i - error al recibir una orden", nroCPU));
			SOCKET_destruir(socket_planif);
			continue;
		}

		//verifico el emisor del mensaje
		switch(orden.sender_id) {

			case PLANIFICADOR:

				if(EJECUTAR_RAFAGA != orden.comando) {
					//comando incorrecto
					error(__func__, string_from_format("cpu %i - se recibio una orden del Planificador pero no era de ejecutar rafaga. Se ignora. Orden recibida: %d", nroCPU, orden.comando));
					SOCKET_destruir(socket_planif);
					continue;
				}

				debug(__func__, string_from_format("cpu %i - se recibio una orden del Planificador (ejecutar rafaga)", nroCPU));

				//recibo el pcb
				result = socket_planif->Recibir(socket_planif, &orden, (void**) &pcb , DeSerializar_PCB);

				if(result <= 0) {
					//error
					error(__func__, string_from_format("cpu %i - error al recibir el pcb enviado por el Planificador", nroCPU));
					SOCKET_destruir(socket_planif);
					continue;
				}

				//me desconecto
				result = socket_planif->Desconectar(socket_planif);

				if(result <= 0) {
					//error al desconectarse
					error(__func__, string_from_format("cpu %i - error al desconectar el socket.. continuo", nroCPU));
					SOCKET_destruir(socket_planif);
				}

				SOCKET_destruir(socket_planif);

				//logueo qué se recibió
				mostrarContenidoPCB(nroCPU, pcb);

				//pongo estado de uso del CPU
				cambiarEstadoUsoCPU(nroCPU, 1);

				//ejecuto la ráfaga
				result = operacionEjecutarRafaga(nroCPU, pcb);

				//fin del trabajo, cambio estado de uso del CPU para calcular estadísticas de uso
				cambiarEstadoUsoCPU(nroCPU, 0);

				if(-1 == result) {
					//error
					error(__func__, string_from_format("cpu %i - la ejecucion de la rafaga finalizo de manera incorrecta", nroCPU));
				} else {
					//fin ok
					info(__func__, string_from_format("cpu %i - se finalizo la ejecucion de la rafaga", nroCPU), cpuConfig.imprimirPorConsola);
				}

				break;

			default:
				//orden de origen inesperado
				warn(__func__, string_from_format("cpu %i - Se recibio una orden del sender %i y se definio como interpretarla.", nroCPU, orden.sender_id));
				SOCKET_destruir(socket_planif);
				continue;
				break;
		}
	}
}

/**
 * Función que loguea el contenido del pcb
 */
void mostrarContenidoPCB(int nroCPU, t_PCB* pcb) {

	info("threadCPU", string_from_format("cpu %i - se recibio un pcb (pid: %u; pc: %u; cantidadInstrucciones: %u; mcod: %s)", nroCPU, pcb->id, pcb->PC, pcb->InstruccionesRafaga, pcb->archivomProc), cpuConfig.imprimirPorConsola);

}

/**
 * Thread de monitoreo - genera la lista de estadísticas de uso de CPU segundo a segundo
 */
void threadMonitoreoUso(int cantidadCPUs) {

	int contadorControlTiempo = 0; //lo uso para ir limpiando la lista y no usar tanta memoria
	int estadoActual = -1;
	int i = 0;

	//inicializo el array de uso
	inicializarArrayUsoCPU(cantidadCPUs);

	while(1) {
		//espero 1 segundo
		usleep(1000000);

		//proceso array
		for(i = 1; i <= cantidadCPUs; i++) {
			//obtengo valor de uso para este cpu
			pthread_mutex_lock(&mutexArrayEstadoCPU);
			estadoActual = arrayEstadoCPU[i];
			pthread_mutex_unlock(&mutexArrayEstadoCPU);

			pthread_mutex_lock(&mutexArrayUsoCPU);
			//trace(__func__, string_from_format("agrego %d a cpu %d", estadoActual, i));
			arrayUsoCPU[i - 1][contadorControlTiempo] = estadoActual;
			pthread_mutex_unlock(&mutexArrayUsoCPU);
		}

		//aumento contador de tiempo
		contadorControlTiempo++;

		if(contadorControlTiempo == CPU_USAGE_TIME_CALC_PARAM) {
			//reinicio contador
			contadorControlTiempo = 0;
		}
	}
}

/**
 * Función que inicializa el array de uso de cpu con -1 para poder diferenciar en el primer minuto los valores usados
 */
void inicializarArrayUsoCPU(int cantidadCPUs) {

	int i, j = 0;

	pthread_mutex_lock(&mutexArrayUsoCPU);

	for(i = 0; i < cantidadCPUs; i++) {

		for(j = 0; j < CPU_USAGE_TIME_CALC_PARAM; j++) {

			arrayUsoCPU[i][j] = -1;
		}
	}

	pthread_mutex_unlock(&mutexArrayUsoCPU);
}

/**
 * Función que modifica el estado de uso del cpu pasado por parámetro.
 * El estado puede ser 0 o 1 - representando idle y busy
 */
void cambiarEstadoUsoCPU(int nroCPU, int estado) {
	//pido lock
	pthread_mutex_lock(&mutexArrayEstadoCPU);
	//modifico el estado
	arrayEstadoCPU[nroCPU] = estado;
	//libero lock
	pthread_mutex_unlock(&mutexArrayEstadoCPU);
}

/**
 * Función que ejecuta la ráfaga
 */
int operacionEjecutarRafaga(int nroCPU, t_PCB* pcb) {

	uint32_t mProcID = 0;
	char* mCod = NULL;
	int contadorRafaga;
	int result = -1;
	int offset = 0;
	char* errorDesc = NULL;
	char* proximaInstruccion = NULL;
	char* tiempoBloqueo = NULL;
	t_comando comandoAEnviar;
	t_list* listaResultados = NULL;

	//reservo mem para strings
	errorDesc = malloc(STRING_BUFFER_SIZE);

	if(NULL == errorDesc) {
		//error en malloc
		error(__func__, "error al reservar memoria para string de error");
		return -1;
	}

	tiempoBloqueo = malloc(STRING_BUFFER_SIZE);

	if(NULL == tiempoBloqueo) {
		//error en malloc
		error(__func__, "error al reservar memoria para string de tiempo de bloqueo");
		freeResource(errorDesc);
		return -1;
	}

	pthread_mutex_lock(&mutexLecturaMCod);

	debug(__func__, string_from_format("cpu %i - abro mCod: %s", nroCPU, pcb->archivomProc));
	mCod = leer_mcod(pcb->archivomProc);

	pthread_mutex_unlock(&mutexLecturaMCod);

	if(NULL == mCod) {
		//error al abrir archivo
		error(__func__, string_from_format("cpu %i - error al leer mCod: %s", nroCPU, pcb->archivomProc));
		freeResource(errorDesc);
		freeResource(tiempoBloqueo);
		return -1;
	}

	//genero lista para resultados
	listaResultados = list_create();

	contadorRafaga = 1;

	//ejecuto instrucciones mientras tenga intrucciones y ráfaga disponible
	//y voy agregando los resultados a la lista

	debug(__func__, string_from_format("cpu %i - comienzo ejecucion de rafaga", nroCPU));

	while (contadorRafaga <= pcb->InstruccionesRafaga && result != CPU_FINALIZAR_PROCESO && result != CPU_ERROR_EJECUCION && result != CPU_ERROR_INST_NO_ENCONTRADA && result != CPU_BLOQUEO_ES) {

		//obtengo prox instrucción
		proximaInstruccion = obtenerInstruccion(mCod, pcb->PC);

		if(NULL == proximaInstruccion) {
			//info("IMPORTANTE!!!!!!!!!!", string_from_format("pc: %i - mproc: %s - id: %i", pcb->PC, pcb->archivomProc, pcb->id),cpuConfig.imprimirPorConsola);
			//no se encontró otra instrucción
			//strcpy(errorDesc, "no pude obtener otra instruccion a ejecutar. Puede ser que falte un ; o que no haya mas instrucciones.");
			//error(__func__, errorDesc);
			//result = CPU_ERROR_INST_NO_ENCONTRADA;
			//FIXME clavo un finalizar normal para arreglar un bug de planificador
			result = CPU_FINALIZAR_PROCESO;
			break;
		}

		//calculo el offset para la próxima inst
		offset = strlen(proximaInstruccion) + 1; //+1 por el ';'

		trace(__func__, string_from_format("ejecuto la instruccion: [%s]", proximaInstruccion));

		mProcID = pcb->id;

		//ejecuto la próxima instrucción
		result = ejecutarInstruccion(proximaInstruccion, listaResultados, mProcID, &errorDesc, &tiempoBloqueo);

		if(-1 == result) {
			//aborto la ejecución
			freeResource(proximaInstruccion);
			freeResource(errorDesc);
			freeResource(tiempoBloqueo);
			error(__func__, "fallo la ejecucion.");
			return -1;
		}

		//avanzo el program counter
		pcb->PC = pcb->PC + offset;

		//libero porque ya no la necesito
		//freeResource(proximaInstruccion);

		//aumento contador de ráfaga
		contadorRafaga++;

		//espero el tiempo indicado como retardo para cada instrucción
		debug(__func__, string_from_format("inicio tiempo de retardo: %d", cpuConfig.retardo));
		usleep(cpuConfig.retardo);
		debug(__func__, "fin tiempo de retardo.");
		
	}

	//veo si se finalizó el proceso
	if(CPU_BLOQUEO_ES == result) {
		//aviso que se bloqueó el proceso
		comandoAEnviar = BLOQUEO_ES;
	} else if(CPU_FINALIZAR_PROCESO == result) {
		//aviso que finalizó el proceso
		comandoAEnviar = FIN_NORMAL;
	} else if((CPU_ERROR_EJECUCION == result) || (CPU_ERROR_INST_NO_ENCONTRADA == result)) {
		//aviso que hubo un error en el proceso
		comandoAEnviar = FIN_CON_ERROR;
	} else {
		//no hubo error ni finalizó el proceso por lo que finalizó la ráfaga
		comandoAEnviar = FIN_RAFAGA;
	}

	//envío resultados junto a respuesta (exitosa o error)
	if(-1 == enviarResultadoEjecucionRafaga(comandoAEnviar, pcb, errorDesc, nroCPU, listaResultados, tiempoBloqueo)) {
		//error al enviar resultado
		error(__func__, string_from_format("error al enviar resultado de la ejecucion de la rafaga al planificador (orden: %s, mCod: %s, pc: %d)", comandoAEnviar, pcb->archivomProc, pcb->PC));
		freeResource(errorDesc);
		freeResource(tiempoBloqueo);
		return -1;
	}

	freeResource(errorDesc);
	freeResource(tiempoBloqueo);
	list_destroy_and_destroy_elements(listaResultados, (void*) freeResource);

	//terminó ok
	return 1;
}

/**
 * Función que envía la respuesta de la ejecución de una ráfaga al planificador
 */
int enviarResultadoEjecucionRafaga(int comandoAEnviar, t_PCB* pcb, char* errorDesc, int nroCPU, t_list* listaResultados, char* tiempoBloqueo) {

	debug(__func__, "enviando respuesta de ejecucion de rafaga al Planificador...");

	int result = 1;
	t_order_header orden;
	//t_order_header orden_recibida;
	t_rafaga resultadoRafaga;

	orden.sender_id = CPU;
	orden.comando = comandoAEnviar;

	resultadoRafaga.numeroCPU = nroCPU;
	resultadoRafaga.pcb = *pcb;

	SOCKET_crear(socket_planificador);

	result = socket_planificador->Conectar(socket_planificador, cpuConfig.ip_planificador , cpuConfig.puerto_planificador);

	if(result <= 0) {
		//error
		error(__func__, "error al conectarse al planificador.");
		return -1;
	}

	result = socket_planificador->Enviar(socket_planificador, NULL, orden, NULL);

	if(result <= 0) {
		//error
		error(__func__, "error al enviar primer orden al planificador.");
		return -1;
	}

	//envío t_rafaga
	result = socket_planificador->Enviar(socket_planificador, (void*) &resultadoRafaga, orden, Serializar_Resultado_Rafaga);

	if(result <= 0) {
		//error
		error(__func__, "error al enviar resultado de rafaga al planificador.");
		return -1;
	}


	//imprimo contenido de la lista a enviar
	trace(__func__, "imprimo contenido de lista de resultados a enviar:");
	printListContent(listaResultados);

	//envío lista de resultados
	result = socket_planificador->Enviar(socket_planificador, (void*) listaResultados, orden, Serializar_Lista_Resultados_Rafaga);

	if(result <= 0) {
		//error
		error(__func__, "error al enviar lista de resultados de rafaga al planificador.");
		return -1;
	}

	if(BLOQUEO_ES == comandoAEnviar) {
		result = socket_planificador->Enviar(socket_planificador, (void*) tiempoBloqueo, orden, SerializarString);

		if(result <= 0) {
			//error
			error(__func__, "error al enviar tiempo de bloqueo al planificador.");
			return -1;
		}
	}

	//envío código de error si es necesario
	if(FIN_CON_ERROR == comandoAEnviar) {

		trace(__func__, "se enviara la descripcion del error al planificador.");

		trace(__func__, string_from_format("error: [%s]", errorDesc));

		if(NULL == errorDesc) {
			strcpy(errorDesc, "No se obtuvo descripcion del error.");
		}

		result = socket_planificador->Enviar(socket_planificador, (void*) errorDesc, orden, SerializarString);

		if(result <= 0) {
			//error
			error(__func__, "error al enviar desc error de rafaga al planificador.");
			return -1;
		}

	}

	return 1; //OK!
}

/**
 * Función que lee la siguiente línea de código a ejecutar por el cpu y la devuelve
 */
char* obtenerInstruccion(char* mCod, uint32_t pc) {

	char* filePointerStart = NULL;
	char* filePointerEnd = NULL;
	int difference;
	char* instruction = NULL;

	//me paro en el pc
	filePointerStart = mCod + pc;

	//busco siguiente fin de instrucción
	filePointerEnd = strchr(filePointerStart, ';');

	if(NULL == filePointerEnd) {
		//no encontré el fin de la instrucción.. algo está mal
		error(__func__, string_from_format("no encontre el fin de la instruccion: %s", filePointerStart));
		return NULL;
	}

	difference = filePointerEnd - filePointerStart;

	if(difference <= 0) {
		//error al calcular diferencia
		error(__func__, string_from_format("error al calcular la cantidad de caracteres de la instruccion: %s", filePointerStart));
		return NULL;
	}

	instruction = malloc(difference + 1);

	if(NULL == instruction) {
		//error en malloc
		error(__func__, "error al realizar malloc");
		return NULL;
	}

	strncpy(instruction, filePointerStart, difference);

	instruction[difference] = '\0';

	//devuelvo la instrucción
	return instruction;
}

/**
 * Función a llamar al recibir la orden pedirUsoCPU
 */
void ordenPedirUsoCPU(t_socket* socket_cliente) {

	t_order_header orden;
	t_list* listaEstadisticas = NULL;
	int result = 0;

	info(__func__, "Orden pedirUsoCPU", cpuConfig.imprimirPorConsola);

	//llamo a func que calcula el uso
	listaEstadisticas = calcularUsoCPU_Array(cpuConfig.cantidad_hilos);

	orden.sender_id = CPU;
	orden.comando = USO_CPU;

	//envío la lista resultado
	result = socket_cliente->Enviar(socket_cliente, (void*) listaEstadisticas, orden, Serializar_USOCPU);

	if (result <= 0) {
		//error de conexión
		error(__func__ , "cpu admin - error al enviar lista de uso de cpu al planificador.");
	}

	//destruyo la lista
	list_destroy_and_destroy_elements(listaEstadisticas, (void*) destroy_statistic);

	result = socket_cliente->Desconectar(socket_cliente);

	if(result <= 0) {
		//error
		error(__func__, "error al desconectarse del planificador.");
	}
}

t_list* calcularUsoCPU_Array(int cantCPUs) {

	t_list* listaResultado = NULL;

	pthread_mutex_lock(&mutexArrayUsoCPU);

	listaResultado = process_cpu_usage_array(arrayUsoCPU, cantCPUs);

	pthread_mutex_unlock(&mutexArrayUsoCPU);

	return listaResultado;
}

/**
 * Función que obtiene el nro de cpu en base al contador de threads de cpu, usa un mutex para que no se pisen los mismos
 */
int obtenerNroCPU() {

	//debug(__func__, "pido lock mutex nro cpu");
	pthread_mutex_lock(&mutexNroCPU);
	//debug(__func__, "obtuve lock mutex nro cpu");

	contadorNroCPU++;
	int nroCPU = contadorNroCPU;

	//debug(__func__, "devuelvo lock mutex nro cpu");
	pthread_mutex_unlock(&mutexNroCPU);
	//debug(__func__, "lock mutex nro cpu devuelto");

	return nroCPU;
}

/**
 * Función que ejecuta la instrucción recibida.
 * En caso de tener que finalizar por algún motivo devuelve la constante FINALIZAR_PROCESO
 */
int ejecutarInstruccion(char* instruccion, t_list* listaResultados, uint32_t mProcID, char** errorDesc, char** tiempoBloqueo) {

	char* strResultado = NULL;
	int result = -1;
	char instruccionAuxiliar[strlen(instruccion) + 1];

	//formateo string para que quede sólo la instrucción
	string_trim(&instruccion);
	trimLeftWhitespaces(&instruccion);

	//para comparar, copio la instrucción y le hago un tolower a la copia
	strcpy(instruccionAuxiliar, instruccion);
	string_to_lower(instruccionAuxiliar);
	
	strResultado = malloc(STRING_BUFFER_SIZE);

	if(NULL == strResultado) {
		//error en malloc
		error(__func__, "error en malloc.");
		return CPU_ERROR_EJECUCION;
	}

	LOG_escribirSeparador(ARCHIVO_LOG);
	
	//INICIAR
	if(true == string_starts_with(instruccionAuxiliar, "iniciar")) {

		//info(__func__, string_from_format("Ejecutando instruccion INICIAR - mProc %d", mProcID), cpuConfig.imprimirPorConsola);

		result = ejecutarInstIniciar(instruccion, mProcID, &strResultado, errorDesc);

		if(-1 != result) {
			//instrucción ejecutada correctamente
			list_add(listaResultados, strResultado);
			return 1;
		} else {
			//error al ejecutar instrucción
			return CPU_ERROR_EJECUCION;
		}

	//ESCRIBIR
	}else if(true == string_starts_with(instruccionAuxiliar, "escribir")) {

		//info(__func__, string_from_format("Ejecutando instruccion ESCRIBIR - mProc %d", mProcID), cpuConfig.imprimirPorConsola);

		result = ejecutarInstEscribir(instruccion, mProcID, &strResultado, errorDesc);

		if(-1 != result) {
			//instrucción ejecutada correctamente
			list_add(listaResultados, strResultado);
			return 1;
		} else {
			//error al ejecutar instrucción
			return CPU_ERROR_EJECUCION;
		}

	//LEER
	}else if(true == string_starts_with(instruccionAuxiliar, "leer")) {

		//info(__func__, string_from_format("Ejecutando instruccion LEER - mProc %d", mProcID), cpuConfig.imprimirPorConsola);

		result = ejecutarInstLeer(instruccion, mProcID, &strResultado, errorDesc);

		if(-1 != result) {
			//instrucción ejecutada correctamente
			list_add(listaResultados, strResultado);
			return 1;
		} else {
			//error al ejecutar instrucción
			return CPU_ERROR_EJECUCION;
		}

	//ENTRADA-SALIDA
	}else if(true == string_starts_with(instruccionAuxiliar, "entrada-salida")) {

		//info(__func__, string_from_format("Ejecutando instruccion ENTRADA-SALIDA - mProc %d", mProcID), cpuConfig.imprimirPorConsola);

		result = ejecutarInstEntradaSalida(instruccion, mProcID, &strResultado, errorDesc, tiempoBloqueo);

		if(-1 != result) {
			//instrucción ejecutada correctamente
			list_add(listaResultados, strResultado);
			return CPU_BLOQUEO_ES;
		} else {
			//error al ejecutar instrucción
			return CPU_ERROR_EJECUCION;
		}

	//FINALIZAR
	}else if(true == string_starts_with(instruccionAuxiliar, "finalizar")) {

		//info(__func__, string_from_format("Ejecutando instruccion FINALIZAR - mProc %d", mProcID), cpuConfig.imprimirPorConsola);

		//ejecuto la instrucción
		result = ejecutarInstFinalizar(mProcID, &strResultado, errorDesc);

		list_add(listaResultados, strResultado);

		return CPU_FINALIZAR_PROCESO;
	}

	error(__func__, "Instruccion no encontrada");

	return CPU_ERROR_INST_NO_ENCONTRADA;
}

/**
 * Ejecutar la instrucción "Iniciar"
 */
int ejecutarInstIniciar(char* instruccion, int mProcID, char** resultado, char** errorDesc) {

	int result = -1;
	t_proceso_memoria proceso;
	int cantPaginas = 0;
	t_order_header orden;
	t_order_header orden_recibida;
	char** tokensArray = NULL;

	tokensArray = string_split(instruccion, INSTRUCTION_SEPARATOR);

	if(NULL == tokensArray) {
		//error
		strcpy(*errorDesc, "error al parsear instruccion");
		error(__func__, *errorDesc);
		return -1;
	}

	cantPaginas = atoi(tokensArray[1]);

	freeArgsArray(tokensArray, ARGUMENTOS_INST_INICIAR);

	info(__func__, string_from_format("Ejecutando instruccion INICIAR (mProc: %d; cantPaginas: %u)", mProcID, cantPaginas), cpuConfig.imprimirPorConsola);

	//envío orden a admin de mem
	SOCKET_crear(socket_cpu_admmem);

	result = socket_cpu_admmem->Conectar(socket_cpu_admmem, cpuConfig.ip_memoria , cpuConfig.puerto_memoria);

	if (result <= 0) {
		//error de conexión
		strcpy(*errorDesc, "error al conectar con el admin de memoria");
		error(__func__ , *errorDesc);
		return -1;
	}

	orden.sender_id = CPU;
	orden.comando = INICIAR_PROCESO;

	proceso.cantidadPaginasTotales = cantPaginas;
	proceso.idProceso = mProcID;

	//envío orden
	result = socket_cpu_admmem->Enviar(socket_cpu_admmem, NULL, orden, NULL);

	if (result <= 0) {
		//error de conexión
		strcpy(*errorDesc, "error al enviar orden a admin de memoria.");
		error(__func__ , *errorDesc);
		return -1;
	}

	trace(__func__, string_from_format("envio a admmem - iniciar proceso %d, %d paginas", proceso.idProceso, proceso.cantidadPaginasTotales));

	//envío datos proceso
	result = socket_cpu_admmem->Enviar(socket_cpu_admmem, (void*) &proceso, orden, Serializar_MemSWAP_IniciarProceso);
	
	if (result <= 0) {
		//error de conexión
		strcpy(*errorDesc, "error al enviar datos de proceso a admin de memoria.");
		error(__func__ , *errorDesc);
		return -1;
	}

	//espero el OK del admin
	result = socket_cpu_admmem->Recibir(socket_cpu_admmem, &orden_recibida, NULL, NULL);

	if(result <= 0) {
		//error
		strcpy(*errorDesc, "error al recibir OK del admin de memoria.");
		error(__func__, *errorDesc);
		return -1;
	}

	result = socket_cpu_admmem->Desconectar(socket_cpu_admmem);

	if (result <= 0) {
		//error de conexión
		error(__func__ , "error al desconectarse del admin de memoria.");
	}

	SOCKET_destruir(socket_cpu_admmem);

	switch (orden_recibida.comando) {
		case OK:
			//éxito!
			strcpy(*resultado, string_from_format("mProc %d - Iniciado", mProcID));
			info(__func__, string_from_format("Instruccion INICIAR exitosa (mProc: %d)", mProcID), cpuConfig.imprimirPorConsola);
			break;

		case FRAMES_OCUPADOS:
			//no hay espacio libre
			strcpy(*errorDesc, "no hay espacio libre en memoria para iniciar este proceso.");
			strcpy(*resultado, string_from_format("mProc %d - Fallo", mProcID));
			return -1;
			break;

		case ERROR:
			//error
			strcpy(*resultado, string_from_format("mProc %d - Fallo", mProcID));
			break;

		default:
			//comando inesperado
			strcpy(*errorDesc, "se recibio un comando inesperado del admin de memoria.");
			error(__func__, *errorDesc);
			return -1;
			break;
	}

	return 1;
}

/**
 * Ejecutar la instrucción "Leer"
 */
int ejecutarInstLeer(char* instruccion, int mProcID, char** resultado, char** errorDesc) {

	int result = -1;
	t_order_header orden;
	t_order_header orden_recibida;
	t_pagina framePedido;
	char* contenidoFramePedido = NULL;
	char** tokensArray = NULL;

	//parseo la instrucción
	tokensArray = string_split(instruccion, INSTRUCTION_SEPARATOR);

	if(NULL == tokensArray) {
		//error
		strcpy(*errorDesc, "error al parsear instruccion");
		error(__func__, *errorDesc);
		return -1;
	}

	orden.sender_id = CPU;
	orden.comando = LEER_FRAME;
	
	framePedido.idProceso = mProcID;
	framePedido.idPagina = atoi(tokensArray[1]);

	//libero array
	freeArgsArray(tokensArray, ARGUMENTOS_INST_LEER);

	info(__func__, string_from_format("Ejecutando instruccion LEER (mProc: %d; pagina: %u)", mProcID, framePedido.idPagina), cpuConfig.imprimirPorConsola);

	//me conecto al admin
	SOCKET_crear(socket_cpu_admmem);

	result = socket_cpu_admmem->Conectar(socket_cpu_admmem, cpuConfig.ip_memoria , cpuConfig.puerto_memoria);

	if(result <= 0) {
		//error
		strcpy(*errorDesc, "error al conectarse al admin de memoria");
		error(__func__, *errorDesc);
		return -1;
	}

	// Se envian los dos comandos juntos, el primero solo con la orden, el segundo, con la orden y el contenido
	//envío primero la orden
	result = socket_cpu_admmem->Enviar(socket_cpu_admmem, NULL, orden, NULL);

	if(result <= 0) {
		//error
		strcpy(*errorDesc, "error al enviar primer mensaje al admin de memoria.");
		error(__func__, *errorDesc);
		return -1;
	}

	//envío el frame a pedir
	result = socket_cpu_admmem->Enviar(socket_cpu_admmem, (void*) &framePedido, orden, Serializar_MemSWAP_Frame);
	
	if(result <= 0) {
		//error
		strcpy(*errorDesc, "error al enviar la pagina a pedir al admin de memoria.");
		error(__func__, *errorDesc);
		return -1;
	}

	//espero el OK del admin
	result = socket_cpu_admmem->Recibir(socket_cpu_admmem, &orden_recibida, NULL, NULL);

	if(result <= 0) {
		//error
		strcpy(*errorDesc, "error al recibir OK del admin de memoria.");
		error(__func__, *errorDesc);
		return -1;
	}

	// Verifico si el Admin de Memoria encontro la pagina pedida
	if (orden_recibida.sender_id == ADMMEM) {
		if (orden_recibida.comando == OK) {

			//recibo los datos dle frame pedido
			result = socket_cpu_admmem->Recibir(socket_cpu_admmem, &orden_recibida, (void*) &contenidoFramePedido, DeSerializar_MemSWAP_ContenidoFrame);
	
			if(result <= 0) {
				//error
				strcpy(*errorDesc, "error al recibir datos del frame pedido al admin de memoria.");
				error(__func__, *errorDesc);
				return -1;
			}

		} else if (orden_recibida.comando == FRAMES_OCUPADOS) {
			//no hay espacio libre
			strcpy(*errorDesc, "El Administrador de Memoria informo que la memoria esta llena.");
			error(__func__, *errorDesc);
			return -1;

	    } else if (orden_recibida.comando == ERROR) {
			//error
			strcpy(*errorDesc, "El Administrador de Memoria no encontro la Pagina pedida");
			error(__func__, *errorDesc);
			return -1;
			
		} else {
			//no vino una resp esperada
			strcpy(*errorDesc, "respuesta inesperada al esperar confirmacion del admin de memoria.");
			error(__func__, *errorDesc);
			return -1;
		}
	} else {
		//recibí un mensaje con otro sender
		strcpy(*errorDesc, "el mensaje recibido no corresponde al admin de memoria.");
		error(__func__, *errorDesc);
		return -1;
	}

	result = socket_cpu_admmem->Desconectar(socket_cpu_admmem);

	if(result <= 0) {
		//error
		error(__func__, "error al desconectarme del admin de memoria.");
	}

	SOCKET_destruir(socket_cpu_admmem);

	//se pudo leer correctamente la pagina
	info(__func__, string_from_format("Instruccion LEER exitosa (mProc: %d; resultado: %s)", mProcID, contenidoFramePedido), cpuConfig.imprimirPorConsola);

	strcpy(*resultado, string_from_format("mProc %d - Pagina %d leida: %s\0", mProcID, framePedido.idPagina, contenidoFramePedido));

	return 1;
}

/**
 * Ejecutar la instrucción "Escribir"
 */
int ejecutarInstEscribir(char* instruccion, int mProcID, char** resultado, char** errorDesc) {

	//Implementar
	int result = -1;
	t_order_header orden;
	t_order_header orden_recibida;
	t_pagina frameAEnviar;
	char* contenidoFrame = NULL;
	char** tokensArray = NULL;

	//parseo la instrucción
	tokensArray = string_split(instruccion, INSTRUCTION_SEPARATOR);

	if(NULL == tokensArray) {
		//error
		strcpy(*errorDesc, "error al parsear instruccion.");
		error(__func__, *errorDesc);
		return -1;
	}

	orden.sender_id = CPU;
	orden.comando = ESCRIBIR_FRAME;
	
	frameAEnviar.idPagina = atoi(tokensArray[1]);
	frameAEnviar.idProceso = mProcID;

	//strcpy(contenidoFrame, tokensArray[2]);
	contenidoFrame = formatWriteText(tokensArray[2]);

	if(NULL == contenidoFrame) {
		//error al intentar procesar texto a escribir
		strcpy(*errorDesc, "error al intentar procesar texto a escribir.");
		error(__func__, *errorDesc);
		return -1;
	}

	//libero array de args
	freeArgsArray(tokensArray, ARGUMENTOS_INST_ESCRIBIR);

	info(__func__, string_from_format("Ejecutando instruccion ESCRIBIR (mProc: %d; pagina: %d; contenido: %s)", mProcID, frameAEnviar.idPagina, contenidoFrame), cpuConfig.imprimirPorConsola);

	char* resultadoCorrecto = string_from_format("mProc %d - Pagina %d escrita: %s", mProcID, frameAEnviar.idPagina, contenidoFrame);

	SOCKET_crear(socket_cpu_admmem);

	result = socket_cpu_admmem->Conectar(socket_cpu_admmem, cpuConfig.ip_memoria , cpuConfig.puerto_memoria);

	if(result <= 0) {
		//error
		strcpy(*errorDesc, "error al conectarse al admin de memoria.");
		error(__func__, *errorDesc);
		freeResource(contenidoFrame);
		return -1;
	}

	// Se envian los dos comandos juntos, el primero solo con la orden, el segundo, con la orden y el contenido
	//Envío orden
	result = socket_cpu_admmem->Enviar(socket_cpu_admmem, NULL, orden, NULL);

	if(result <= 0) {
		//error
		strcpy(*errorDesc, "error al enviar la orden al admin de memoria.");
		error(__func__, *errorDesc);
		freeResource(contenidoFrame);
		return -1;
	}

	//Envío página a guardar
	result = socket_cpu_admmem->Enviar(socket_cpu_admmem, (void*) &frameAEnviar, orden, Serializar_MemSWAP_Frame);

	if(result <= 0) {
		//error
		strcpy(*errorDesc, "error al enviar la página al admin de memoria.");
		error(__func__, *errorDesc);
		freeResource(contenidoFrame);
		return -1;
	}

	//Envío contenido del frame  a guardar
	result = socket_cpu_admmem->Enviar(socket_cpu_admmem, (void*) contenidoFrame, orden, Serializar_MemSWAP_ContenidoFrame);

	if(result <= 0) {
		//error
		strcpy(*errorDesc, "error al enviar contenido de la pagina al admin de memoria");
		error(__func__, *errorDesc);
		freeResource(contenidoFrame);
		return -1;
	}
	
	//ya no uso el contenido del frame por lo que lo puedo liberar
	freeResource(contenidoFrame);

	//Espero a recibir el OK
	result = socket_cpu_admmem->Recibir(socket_cpu_admmem, &orden_recibida, NULL, NULL);

	if(result <= 0) {
		//error
		strcpy(*errorDesc, "error al recibir OK del admin de memoria");
		error(__func__, *errorDesc);
		return -1;
	}

	if (orden_recibida.sender_id == ADMMEM) {

		switch (orden_recibida.comando) {
			case OK:
				//se pudo grabar correctamente la pagina
				info(__func__, string_from_format("Instruccion ESCRIBIR exitosa (mProc: %d)", mProcID), cpuConfig.imprimirPorConsola);
				strcpy(*resultado, resultadoCorrecto);
				return 1;
				break;

			case FRAMES_OCUPADOS:
				//no hay espacio libre
				strcpy(*errorDesc, "El Administrador de Memoria informo que la memoria esta llena.");
				error(__func__, *errorDesc);
				return -1;
				break;

			case SEG_FAULT:
				//me pasé con los elementos enviados
				strcpy(*errorDesc, "Segmentation fault :D");
				error(__func__, *errorDesc);
				return -1;
				break;

			case ERROR:
				//hubo un error al intentar escribir del lado del admin
				strcpy(*errorDesc, "hubo un error al intentar escribir del lado del admin");
				error(__func__, *errorDesc);
				return -1;
				break;

			default:
				//no vino una resp esperada
				strcpy(*errorDesc, "respuesta inesperada al esperar confirmacion del admin de memoria");
				error(__func__, *errorDesc);
				return -1;
				break;
		}
	} else {
		//recibí un mensaje con otro sender
		strcpy(*errorDesc, "se recibio mensaje pero de otro sender_id distinto al admin de memoria");
		error(__func__, *errorDesc);
		return -1;
	}

	//me desconecto del socket
	result = socket_cpu_admmem->Desconectar(socket_cpu_admmem);

	if(result <= 0) {
		//error
		error(__func__, "error al desconectarse del admin de memoria. Continuo");
	}

	SOCKET_destruir(socket_cpu_admmem);
}

/**
 * Ejecutar la instrucción "Entrada-Salida"
 */
int ejecutarInstEntradaSalida(char* instruccion, int mProcID, char** resultado, char** errorDesc, char** tiempoBloqueo) {

	char** tokensArray = NULL;
	char* resultadoCorrecto = NULL;

	//parseo la instrucción
	tokensArray = string_split(instruccion, INSTRUCTION_SEPARATOR);

	strcpy(*tiempoBloqueo, tokensArray[1]);

	if(NULL == tokensArray) {
		//error
		strcpy(*errorDesc, "error al parsear instruccion.");
		error(__func__, *errorDesc);
		return -1;
	}

	//libero array
	freeArgsArray(tokensArray, ARGUMENTOS_INST_ES);

	resultadoCorrecto = string_from_format("mProc %d en entrada-salida de tiempo %s", mProcID, *tiempoBloqueo);

	strcpy(*resultado, resultadoCorrecto);

	info(__func__, string_from_format("Ejecutando instruccion ENTRADA-SALIDA (mProc: %d; tiempo: %s). El proceso se bloqueara", mProcID, *tiempoBloqueo), cpuConfig.imprimirPorConsola);

	return 1;
}

/**
 * Ejecutar la instrucción "Finalizar"
 */
int ejecutarInstFinalizar(int mProcID, char** resultado, char** errorDesc) {

	int result = 0;
	t_order_header orden;
	t_order_header orden_recibida;
	t_proceso_memoria proceso;

	strcpy(*resultado, string_from_format("mProc %d finalizado", mProcID));

	info(__func__, string_from_format("Ejecutando instruccion FINALIZAR (mProc %d)", mProcID), cpuConfig.imprimirPorConsola);

	//envío orden a admin de mem
	SOCKET_crear(socket_cpu_admmem);

	result = socket_cpu_admmem->Conectar(socket_cpu_admmem, cpuConfig.ip_memoria , cpuConfig.puerto_memoria);

	if (result <= 0) {
		//error de conexión
		strcpy(*errorDesc, "error al conectar con el admin de memoria");
		error(__func__ , *errorDesc);
		return -1;
	}

	orden.sender_id = CPU;
	orden.comando = FINALIZAR_PROCESO;

	proceso.idProceso = mProcID;

	//envío orden
	result = socket_cpu_admmem->Enviar(socket_cpu_admmem, NULL, orden, NULL);

	if (result <= 0) {
		//error de conexión
		strcpy(*errorDesc, "error al enviar orden a admin de memoria.");
		error(__func__ , *errorDesc);
		return -1;
	}

	trace(__func__, string_from_format("envio a admmem - finalizar proceso %d", proceso.idProceso));

	//envío datos proceso
	result = socket_cpu_admmem->Enviar(socket_cpu_admmem, (void*) &proceso, orden, Serializar_MemSWAP_FinalizarProceso);

	if (result <= 0) {
		//error de conexión
		strcpy(*errorDesc, "error al enviar datos de proceso a admin de memoria.");
		error(__func__ , *errorDesc);
		return -1;
	}

	//espero el OK del admin
	result = socket_cpu_admmem->Recibir(socket_cpu_admmem, &orden_recibida, NULL, NULL);

	if(result <= 0) {
		//error
		strcpy(*errorDesc, "error al recibir OK del admin de memoria.");
		error(__func__, *errorDesc);
		return -1;
	}

	result = socket_cpu_admmem->Desconectar(socket_cpu_admmem);

	if (result <= 0) {
		//error de conexión
		error(__func__ , "error al desconectarse del admin de memoria.");
	}

	SOCKET_destruir(socket_cpu_admmem);

	switch (orden_recibida.comando) {
		case OK:
			//éxito!
			strcpy(*resultado, string_from_format("mProc %d - Finalizado exitosamente", mProcID));
			info(__func__, string_from_format("Instruccion FINALIZAR exitosa (mProc %d)", mProcID), cpuConfig.imprimirPorConsola);
			break;

		case ERROR:
			//error
			strcpy(*errorDesc, "error al finalizar el proceso en memoria.");
			error(__func__, *errorDesc);
			return -1;
			break;

		default:
			//comando inesperado
			strcpy(*errorDesc, "se recibio un comando inesperado del admin de memoria.");
			error(__func__, *errorDesc);
			return -1;
			break;
	}

	return 1;
}


/**
 * Función que envía aviso al planificador de que un cpu está disponible.
 * Devuelve el puerto usado para generar el aviso o error
 */
int avisarCPUDisponible(int nroCPU) {

	int result = -1;

	t_order_header orden;
	t_order_header orden_recibida;
	char puertoCPU[LONGITUD_CHAR_PUERTOS];
	SOCKET_crear(socket_cpu_planif);

	//me conecto al planificador
	result = socket_cpu_planif->Conectar(socket_cpu_planif, cpuConfig.ip_planificador, cpuConfig.puerto_planificador);

	if (result <= 0) {
		//error de conexión
		error(__func__, string_from_format("cpu %i - error al conectar con el Planificador", nroCPU));
		SOCKET_destruir(socket_cpu_planif);
		return -1;
	}

	// Obtengo el puerto del Thread de CPU (con funcion Socket)
	strcpy(puertoCPU, string_itoa(socket_cpu_planif->Puerto_Cliente));

	orden.sender_id = CPU;
	orden.comando = CONECTAR_CPU;

	//envío la orden
	result = socket_cpu_planif->Enviar(socket_cpu_planif, NULL, orden, NULL);

	if (result <= 0) {
		//error de conexión
		error(__func__, string_from_format("cpu %i - error al enviar orden al Planificador", nroCPU));
		SOCKET_destruir(socket_cpu_planif);
		return -1;
	}

	//envío el nro de cpu
	result = socket_cpu_planif->Enviar(socket_cpu_planif, (void*) string_itoa(nroCPU), orden, SerializarString);

	if (result <= 0) {
		//error de conexión
		error(__func__, string_from_format("cpu %i - error al enviar puerto de cpu al Planificador",nroCPU));
		SOCKET_destruir(socket_cpu_planif);
		return -1;
	}

	//Espero a recibir el OK
	result = socket_cpu_planif->Recibir(socket_cpu_planif, &orden_recibida, NULL, NULL);

	if(result <= 0) {
		//error
		error(__func__, string_from_format("cpu %i - error al recibir OK del Planificador.", nroCPU));
		SOCKET_destruir(socket_cpu_planif);
		return -1;
	}

	//me desconecto del planificador
	result = socket_cpu_planif->Desconectar(socket_cpu_planif);

	if (result <= 0) {
		//error de conexión
		error(__func__, string_from_format("cpu %i - error al desconectarse del Planificador", nroCPU));
	}

	SOCKET_destruir(socket_cpu_planif);

	if(OK != orden_recibida.comando) {
		//error
		error(__func__, string_from_format("cpu %i - se esperaba el OK del Planificador pero se recibio otro comando.", nroCPU));
		return -1;
	}

	info(__func__, string_from_format("cpu %d - conectado correctamente al planificador", nroCPU), cpuConfig.imprimirPorConsola);
	LOG_escribirSeparador(ARCHIVO_LOG);

	return atoi(puertoCPU);
}

#include "../lib/commons/config.h"
#include "../bibliotecas/comun.h"
#include "../bibliotecas/sockets.h"

#define CHARSWAP '\0'
#define SWAP_FILE_NAME_LENGTH 50
#define CONFIG_FILE_ROUTE "swap_config.cfg"

// La estructura para guardar la configuracion del SWAP
typedef struct {

	char 		swap_port[5];						//	Puerto de escucha del SWAP
	char 		name_swap[SWAP_FILE_NAME_LENGTH];	//	Nombre del archivo SWAP
	uint32_t	number_pages;						//	Cantidad de paginas que puede ubicar el archivo SWAP
	uint32_t 	page_size;							//	El tamaño de la pagina
	uint32_t	compactation_delay;					//	El retardo de la compactación
	uint32_t	swap_delay;							//	El retardo del SWAP

} t_swap_config;

// La estructura que representa un proceso en el SWAP
typedef struct {

	uint32_t idProceso;								//	El PID del proceso
	uint32_t pageNumberStart;						//	La posicion inicial en el SWAP
	uint32_t totalPages;							//	La cantidad total de las paginas del proceso

} t_process_swap;

// La estructura para guardar el estado del frame (ocupado/libre)
typedef struct {

	char ocupado;									// Valor 1 - frame ocupado, 0 - frame libre

} t_frame_state;


/****************** Ordenes del Administrador de Memoria **********************/

// El handler de la orden "Iniciar proceso" de Administrador de Memoria 
int orderStartProcess(t_socket* socket_cliente);

// El handler de la orden "Escribir frame" de Administrador de Memoria
int orderWriteFrame(t_socket* socket_cliente);

// El handler de la orden "Leer frame" de Administrador de Memoria
int orderReadFrame(t_socket* socket_cliente);

// El handler de la orden "Finalizar frame" de Administrador de Memoria
int orderFinishProcess(t_socket* socket_cliente);

/*******************************************************************************/


/************************* Funciones internas de SWAP **************************/

/*
 * La funcion que trae el contenido de la pagina solicitada
 * processPID - PID del proceso
 * pageNumber - La pagina del proceso a cargar
 * frameData  - La pagina solicitada
 * Devuelve 0 en el caso de que se encuentre pagina
 * Devuelve -1 en el caso de cualquier error
 */
int getFrameSWAP(uint32_t pageNumber, uint32_t processPID, char** frameData);

/*
 * La funcion que busca espacio para el proceso
 * pageNumberStart - numero de pagina donde hay que empezar a guardar
 * processSize     - cantidad de paginas que requiere el proceso
 * Devuelve 0 en el caso de que se encuentre lugar
 * Devuelve -1 si no hay espacio contiguo
 */
int findFreeSpace(uint32_t* pageNumberStart, uint32_t processSize);

// La funcion que se encarga de compactar los datos en SWAP
int defragmentSWAP();

// La funcion que simula el retardo, como parametro recibe el tiempo en segundo
int simulateDelay(int timeSleep);

// Busca el proceso en el SWAP por su pagina inicial
t_process_swap* findProcess(int processPositionStart);

// La funcion mueve el proceso dentro del SWAP a la nuevo posicion
int moveProcess(t_process_swap* processToMove, int freePositionStart);

// La funcion para leer el archivo de configuracion
int readSwapConfigFile();

// La funcion generadora de la particion SWAP
int createSwapFile();

// La funcion que escucha los comandos del Administrador de Memoria
int esperarAdmMem();

// La funcion que busca la posicion del frame en SWAP por el numero de pagina y el PID del proceso
int getReadPosition(uint32_t pageNumber, uint32_t processPID);

/*******************************************************************************/

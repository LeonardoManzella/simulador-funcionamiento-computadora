#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_

	#include <stdbool.h>
	#include <stdint.h>
	#include <time.h>


	#include "../bibliotecas/comun.h"
	#include "../bibliotecas/structs.h"
	#include "../bibliotecas/sockets.h"

	#include "../lib/readline/config.h"
	#include "../lib/readline/readline.h"

	//Estados Posibles de mPro (Error va aparte porque incluye la Razon de porque Fallo)
	#define LISTO 					"Listo"
	#define POR_FINALIZAR 			"Listo - Por Finalizar"		//Estado Especial de Listo para Comando Finalizar
	#define BLOQUEADO 				"Bloqueado"
	#define EJECUTANDO 				"Ejecutando"
	#define EJECUTANDO_FINALIZAR 	"Ejecutando - Finalizara en Proxima Rafaga"
	#define TERMINADO				"TerminadoOK"
	#define ERROR					"[ERROR] Razon:"


	//Para abstraerme de la Biblioteca time.h
	#define ERROR_TimeStamp		((time_t) -1)
	typedef time_t tipo_TimeStamp;				//Para el Interesado, de fondo son como 15 defines de defines que terminan en un long int.

	typedef enum {
		LIBRE,
		OCUPADO,
		DESCONECTADO
	} tipo_CPU_estado;


	typedef struct {
		t_PCB			PCB;
		uint32_t		segundosBloqueo;
		char			estado[100];

		//Para metricas
		tipo_TimeStamp	ultimoTimeStamp;
		double			tiempoRespuesta;
		double			tiempoEjecucion;
		double			tiempoEspera;

		//NOTA: Si agrego cosas aca, recordar modificar Comando "Correr" para inicializar
	} tipo_mPro;


	typedef struct {
		uint32_t			numeroCPU;
		tipo_CPU_estado		estado;
		uint32_t			ID_mPro_Ejecutando;
		char				ip[LONGITUD_CHAR_IP];
		int					puerto;

		//NOTA: Si agrego cosas aca, recordar modificar Orden "ConectarCPU" para inicializar
	} tipo_CPU;


	typedef struct {
		char	ip[LONGITUD_CHAR_IP];
		int		puerto;
	} tipo_AdminCPU;




	//===================== Funciones de Inicio ==========================
	bool estaValidoArchifoConfig();
	void cargarValoresArchivoConfig();
	bool estaEstablecidaRutasAbsoluta();
	bool estanInicializadasEstructurasNecesarias();
	bool estanCreadosModulos();
	//====================================================================


	//Funciones Varias
	long buscarAtrasPuntoComa(FILE* archivoAbierto, long posicionActual);
	long obtenerPosicioninstruccionFinalizar(const char* rutaLinuxArchivo);
	void loguearResultadoEjecucion(t_list* listaResultadosInstrucciones, char* nombre_mPro);
	tipo_CPU*	buscarCPU(uint32_t numeroCPU);		//Usar entre Mutex!
	tipo_mPro*	buscarMPro(uint32_t pid);			//Usar entre Mutex!
	char* generarEstadoError(const char* razon);
	void desconectarCPU(tipo_CPU* cpuPorDesconectar);
	char* obtenerNombreEstadoCPU(tipo_CPU_estado estado);
	int conectarReintentando(struct __t_socket* socketPorConectarse, const char* serverIP, const char* serverPort);		//El Socket Ya debe estar creado previamente


	//=============== Threads de Modulos Independientes ==================
	void ThreadPlanificador_FIFO(void* valorQueNoUso);
	void ThreadPlanificador_RoundRobin(void* valorQueNoUso);

	void ThreadBloqueos(void* valorQueNoUso);
	void ThreadServidor(void* valorQueNoUso);
	//====================================================================


	//=============== Ordenes del Servidor ==================
	void Servidor_diferenciarOrden(t_order_header headerRecibido, t_socket* socketCliente);
	void Servidor_Orden_ConectarAdminCPU(t_socket* socketCliente);
	void Servidor_Orden_ConectarCPU(t_socket* socketCliente);
	void Servidor_Orden_FinPorError(t_socket* socketCliente);
	void Servidor_Orden_FinRafaga(t_socket* socketCliente);
	void Servidor_Orden_BloqueoES(t_socket* socketCliente);
	void Servidor_Orden_FinNormal(t_socket* socketCliente);
	//=======================================================


	//Funcion principal para leer la consola, Basicamente toma lo que escribe el usuario, controla Los Arguemtnos Y selecciona a que Comando(Funcion) Llamar
	int leerConsola();

	//=============== Comandos de Consola =================
	void Consola_Comando_Correr(const char* rutaLinuxArchivo);
	void Consola_Comando_PS();
	void Consola_Comando_Finalizar(const char* mProID);
	void Consola_Comando_CPU();
	void Consola_Comando_DeadLockChecker();
	void Consola_Comando_StatusCpu();
	void Consola_Comando_StatusMPro();
	//=====================================================


	//================= Funciones para Metricas ==========================
	//Sirven Para abstraerme de la Biblioteca time.h
	tipo_TimeStamp	Tiempo_obtenerActual();			//Ante error devuelve ERROR_TimeStamp

	//Devuelve la diferencia de tiempo en Segundos con Respecto al tiempo actual.   Devuelve -1 ante Error
	double	Tiempo_calcularDiferencia( tipo_TimeStamp tiempoViejo );

	void Metricas_inicializar(tipo_mPro* mPro);
	void Metricas_actualizarTiempoEspera(tipo_mPro* mPro);
	void Metricas_actualizarTiempoEjecucion(tipo_mPro* mPro);
	void Metricas_fixBloqueos(tipo_mPro* mPro);
	void Metricas_loguear(tipo_mPro* mPro);
	//====================================================================



// Nombre de funciones para consola
int com_list(char *args);
int com_pwd(char *args);
int com_help(char *args);
int com_cd(char *args);
int com_quit(char *args);
int com_run(char *args);
int com_kill(char *args);
int com_ps(char *args);
int com_cpu(char *args);
int com_deadlock(char *args);
int com_statcpu(char *args);
int com_statmproc(char *args);

// Struct para comandos de consola
typedef struct {
  char *name;			/* Nombre de la funcion */
  rl_icpfunc_t *func;		/* Puntero a funcion */
  char *doc;			/* Descripcion de la funcion  */
  int nParametros;		/* Cantidad de Parametros */
} COMMAND;

COMMAND commands[] = {
  { "cd", com_cd, "Cambia el directorio actual", 1},
  { "help", com_help, "Muestra la ayuda", 0 },
  { "ayuda", com_help, "Sinonimo de 'help'", 0 },
  { "?", com_help, "Synonym for `help'", 0 },
  { "ls", com_list, "Muestra una lista de archivos", 0 },
  { "pwd", com_pwd, "Muestra el directorio actual", 0 },
  { "quit", com_quit, "Sale del sistema", 0 },
  { "salir", com_quit, "Sinonimo de 'quit'", 0 },
  { "correr", com_run, "Ejecuta un programa mCod", 1 },
  { "finalizar", com_kill, "Termina un programa mCod", 1 },
  { "ps", com_ps, "Muestra el estado de los procesos mProc en cola", 0 },
  { "cpu", com_cpu, "Muestra estidisticas de uso del CPU", 0 },
  { "deadlock", com_deadlock, "Verifica si existe un deadlock", 0 },
  { "statuscpu", com_statcpu, "Muestra el detalle de los CPU", 0 },
  { "statusmpro", com_statmproc, "Muestra el detalle de los procesos mProc", 0 },
  { (char *)NULL, (rl_icpfunc_t *)NULL, (char *)NULL, 0 }
};

int ejecutarLinea(char* lineaLeida);
COMMAND* buscar_comando(char* nombrecmd);
char **completar_comandos (const char *text, int start, int end);
char *command_generator (const char *text, int state);
char** valid_argument (char* caller, char* arg);
char **split_arg(const char* arg);

#endif //PLANIFICADOR_H_

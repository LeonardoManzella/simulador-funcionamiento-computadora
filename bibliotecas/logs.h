/*
================================= COMO USAR ==================================
 - Puede llamarse desde cualquier lado para loguear con "LOG_escribir" o "LOG_escribirPantalla" o "LOG_escribirSeparador"
 - Todos los nombres de Archivo deben terminar en ".html"
==============================================================================
*/

/*
========================== USO DE NIVELES DE LOGUEO ==========================
Para los Logueos, utilizar los distintos niveles para hacerlo mas entendible a la hora de buscar problemas o Bugs:

	 Nivel ERROR	->  Para Errores Irrecuperables o cuando Se para la Ejecucion de una tarea
	 Nivel WARNING	->  Errores que pueden Ignorarse porque no rompen todo o Cosas que no deberian pasar
	 Nivel INFO		->  Para Marcar el Inicio y Fin de Tareas u Ordenes en el Servidor
	 Nivel DEBUG	->  Para Marcar cosas que son "Criticas", mas importante que los Mensajes de Seguimiento.
	 Nivel TRACE	->  Sucesos varios utiles para seguimiento, la mayoria de los Logs deben ser de este tipo

 - Por Favor Respeten esta Convencion, asi no son un Caos los Logs. Sino no son utiles.
==============================================================================
 */





#ifndef BIBLIOTECA_LOGS_H_
#define BIBLIOTECA_LOGS_H_

	#include "../lib/commons/log.h"

	//Si no existe el Log, lo Crea. Si ya existia agrega cosas al final
	//NOTA: Permite pasar argumentos %s al estilo Printf
	void LOG_escribir(const char* archivoLoguear, const char* nombreTareaFuncion, t_log_level nivelDeLogueo, const char* textoPorLoguear, ... );

	//Identica Funcion anterior, pero Imprime por Pantalla
	void LOG_escribirPantalla(const char* archivoLoguear, const char* nombreTareaFuncion, t_log_level nivelDeLogueo, const char* textoPorLoguear, ... );


	//Imprime Una Separacion (Varios Espacios en Blanco), muy util para dar Formato y Separacion dentro del archivo de Log. Por Ejemplo usandolo cuando se termina una determinada tarea que se repite cada tanto
	void LOG_escribirSeparador(const char* archivoLoguear);


	//Macro para imprimir en archivo + cuando estamos en Debug que tambien imprima en pantalla
	#ifndef NODEBUG
		#define LOG_escribirDebugPantalla(archivoLoguear, nombreTareaFuncion, nivelDeLogueo, textoPorLoguear, ...)  LOG_escribirPantalla(archivoLoguear, nombreTareaFuncion, nivelDeLogueo, textoPorLoguear, ##__VA_ARGS__);
	#else
		#define LOG_escribirDebugPantalla(archivoLoguear, nombreTareaFuncion, nivelDeLogueo, textoPorLoguear, ...)  LOG_escribir(archivoLoguear, nombreTareaFuncion, nivelDeLogueo, textoPorLoguear, ##__VA_ARGS__);
	#endif

	//NOTA: Para asegurarme que no halla problemas por Logs concurrentes yo utilizo internamente un Mutex (compartido entre Threads, Procesos y Forks) y detecto la primera vez que se utiliza cualquiera de estas funciones.
	//Si esta primera vez falla la inicializacion del mutex, hago que se pare todo (abort) mostrando por pantalla que fallo el mutex. Dando la posibilidad de analizar con GDB el estado del sistema antes de abortar.

#endif /* BIBLIOTECA_LOGS_H_ */

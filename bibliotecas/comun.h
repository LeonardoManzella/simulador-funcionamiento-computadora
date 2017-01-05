#ifndef BIBLIOTECA_COMUN_H_
#define BIBLIOTECA_COMUN_H_

#include <stdbool.h> 			//Para True y False
#include "../lib/commons/process.h" 	//Para Obtener ID de proceso y thread actual, para la "Macro_CheckError"
#include <string.h>				//Para StrError
#include <errno.h>				//Para ErrNo
#include <stdio.h>				//Para los Printf
#include <stdint.h>		//Para Tipo uint32_t

/*	MACRO ESPECIAL Para Hacer Mensajes de Debug
 *	Cuando Compilemos con NODEBUG estas desaparecen. La idea es que no tengamos que andar "Comentando los printf de debug".
 *	NOTA: "ImprimirParaDebug" Imprime como un Printf Comun (sin \n), si quieren que imprima (con \n) datos extras, como la funcion y la linea, usen "ImprimirParaDebugConDatos"   */
#ifdef NODEBUG
	#define Macro_ImprimirParaDebug(Mensaje, ...)
	#define Macro_ImprimirParaDebugConDatos(Mensaje, ...)
#else
	#define Macro_ImprimirParaDebug(Mensaje, ...)              printf(Mensaje, ##__VA_ARGS__);
	#define Macro_ImprimirParaDebugConDatos(Mensaje, ...)      printf("[DEBUG] (%s:%s:%d): " Mensaje "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__);
#endif


/*	MACRO PARA MANEJO DE ERRORES SIMPLES
 * 	Es muy util para tener que evitar Tanto IF, la idea es tener un manejo de errores sencillo en 1 sola linea
 *	Explico que hace: Si el valor que le pases es Negativo detecta que hay un error, imprime distintos datos utiles + el Texto que le pases como 2do argumento. Por ultimo hace un return con el Numero Negativo que le pasastes.
 *	Entonces, cuando la llames en cualquier funcion lo que hace es un return con valor negativo si es que hay un error. Al ser una macro del preprocesador es como si hiciera un Copy-Paste de un if sencillo para control de errores.
 *	Cuando NO Usar: Cuando tenes una funcion que NO devuelve un INT o cuando al detectar error tenes que hacer algo + que imprimir un mensaje (EJ: Liberar la memoria o Cerrar Socekts).
 *	COMO NOTA: La Macro Permite Imprimir como un Printf, por Ejemplo: CheckError(varE,"Contenido %d", variable)
 *	NOTA2: Con  NODEBUG ya no imprime el mensaje de Error, pero sigue controlando y retornando los errores   */
#ifdef NODEBUG
	#define Macro_CheckError(ValorPorEvaluar, Mensaje, ...) 	if( (ValorPorEvaluar) < 0) { return (ValorPorEvaluar); }
#else
	#define Macro_CheckError(ValorPorEvaluar, Mensaje, ...) 	if( (ValorPorEvaluar) < 0) { printf("(%s %s) Funcion: %s Archivo:%s:%d, ERROR: " Mensaje ", PID: %d, ThreadId: %d\n", __DATE__, __TIME__, __func__, __FILE__, __LINE__ , ##__VA_ARGS__, process_getpid(), process_get_thread_id() ); return (ValorPorEvaluar); }
#endif




	//Para poder Ponerle Colores al Imprimir Texto en la Consola
	#define ANSI_COLOR_RED     "\x1b[31;1m"
	#define ANSI_COLOR_YELLOW  "\x1b[33;1m"
	#define ANSI_COLOR_BLUE    "\x1b[34;1m"
	#define ANSI_COLOR_GREEN   "\x1b[32;1m"
	#define ANSI_COLOR_MAGENTA "\x1b[35;1m"
	#define ANSI_COLOR_CYAN    "\x1b[36;1m"
	#define ANSI_COLOR_RESET   "\x1b[0m"

	#define PANTALLA_BORRAR_LINEAS                                  "\033[2J"
	#define PANTALLA_POSICIONARSE_ARRIBA_IZQUIERDA                  "\033[0;0H"
	//Nota: en 'CantidadCaracteres' debe Ponerse el Numero Entre Comillas
	#define PANTALLA_POSICIONARSE_TABULADO( CantidadCaracteres )    "\033[" CantidadCaracteres "C"
	#define PANTALLA_POS_FILA_ARRIBA( CantidadCaracteres )    "\033[" CantidadCaracteres "A"
	#define PANTALLA_POS_FILA_ABAJO( CantidadCaracteres )    "\033[" CantidadCaracteres "B"

	#define EJECUCION_CORRECTA   true				//Seria 1 segun StdBool
	#define EJECUCION_FALLIDA    false  			//Seria 0 segun StdBool




	//Ahora vienen las CONSTANTES COMUNES
	#define MD5_LENGTH 				33		//La longitud del MD5 debe ser de 33 porque son 32 caracteres + 1 caracter de Fin de String (\0)

	#define LONGITUD_CHAR_PUERTOS	6		//5 + 1 por caracter de Fin de String
	#define LONGITUD_CHAR_IP		17		//16 + 1 por caracter de Fin de String

	// Macros especiales para determinar el tipo de la variable e imprimir en funcion de eso
	#define printf_dec_format(x) _Generic((x), \
	char: "%c", \
	signed char: "%hhd", \
	unsigned char: "%hhu", \
	signed short: "%hd", \
	unsigned short: "%hu", \
	signed int: "%d", \
	unsigned int: "%u", \
	long int: "%ld", \
	unsigned long int: "%lu", \
	long long int: "%lld", \
	unsigned long long int: "%llu", \
	float: "%f", \
	double: "%f", \
	long double: "%Lf", \
	char *: "%s", \
	void *: "%p")

	#define Macro_Print(x) 			printf(printf_dec_format(x), x)
	#define Macro_PrintNL(x) 		printf(printf_dec_format(x), x), printf("\n");
	//Fin de macros para tipo de variables


	/*	La Idea Estas 2 Macros es que vayan Juntas.
	 *	la Primera Imprime un Mensaje, para hacer un especie de "Cargando".
	 *	La 2da Imprime (Tabulado respecto de la Primera Macro) un OK o un ERROR segun de True o False el Booleano respectivamente.   */
	#define Macro_ImprimirEstadoInicio(Mensaje)                        printf("%s\r",(Mensaje) );
	#define Macro_ImprimirEstadoFinal(Booleano)                        printf( PANTALLA_POSICIONARSE_TABULADO("70") "%s\n", ( (Booleano) == true )   ?   "[" ANSI_COLOR_GREEN"Ok"ANSI_COLOR_RESET "]"   :   "[" ANSI_COLOR_RED"Error!"ANSI_COLOR_RESET "]")

	#define Macro_ImprimirPos(PosicionEjeX, PosicionEjeY, Mensaje)     printf( "\033[%d;%dH%s",(PosicionEjeX),(PosicionEjeY),(Mensaje) )

	//Permite limpiar la pantalla al estilo del comando "clear" de Linux
	#define Macro_LimpiarPantalla()                                    printf( PANTALLA_BORRAR_LINEAS    PANTALLA_POSICIONARSE_ARRIBA_IZQUIERDA );


	//Macro Muy Util para Imprimir el Valor que tiene Errno, mucho mejor que usar PERROR porque no te imprime en cualquier lado (como lo hace Perror), sino que lo hace secuencialmente a la ejecucion del programa
	//NOTA: La Macro devuelve un CHAR* que debe imprimirse con cualquier otra Macro o Printf
	#define Macro_Obtener_Errno() 										(errno == 0 ? "Errno OK" : strerror(errno) )

	//Macro que permite Imprimir un Mensaje de Error y el ERRNO en ROJO. Ademas le agrega Datos extras: Nombre Archivo, Nombre Funcon y Linea Funcion
	//NOTA: Permite pasar variables a lo PRINTF. EJ: Macro_Imprimir_Error("Valor Variable %s", var);
	#define Macro_Imprimir_Error(Mensaje, ...) 							Macro_ImprimirParaDebug(ANSI_COLOR_RED  "[ERROR][%s:%s:%d] ERRNO:%s - " Mensaje ANSI_COLOR_RESET "\n" , __FILE__, __func__, __LINE__, Macro_Obtener_Errno(), ##__VA_ARGS__ );

   /*	MACRO AVANZADA PARA CHEQUEO Y MANEJO DE ERRORES
	*	Si la expresion Booleana es verdadera hace 3 cosas: Imprime el Mensaje Indicado, Limpia ERRNO y salta al Handler de Errores de la Funcion
	*	La idea de esta Macro es que por Funcion se tenga una UNICA Rutina para manejar Todos los Errores posibles. Esta Rutina debe ir al final del programa indicada con la LABEL "Error_Handler:"
	*
	*	RESTRICCION: 	Como se utiliza GOTO es requerido que TODAS las Variables que se usen en el Handler de Errores sean Inicializadas a Algun Valor al Principio de la Funcion (SI, apenas inicia, asi nos evitamos problemas).
	*					Esto es porque podria pasar que con el GoTo salte al Handler y esa Variable nunca haya sido Inicialiazda y ¡ROMPA Todo!.
	*
	*	Esta Macro se lleva muy bien con la Funcion "Comun_LiberarMemoria". Van a ver que es muy util para el Handler de Errores.
	*	Si no terminan de Entender su Funcionamiento, les recomiendo preguntar a LEONARDO o leer este Link:  http://c.learncodethehardway.org/book/ex20.html     */
	#define Macro_Check_And_Handle_Error(ExpresionBooleana, Mensaje, ...) 			if(ExpresionBooleana) {   Macro_Imprimir_Error(Mensaje,##__VA_ARGS__);     errno=0;    goto Error_Handler;   }









	int Comun_existeArchivo(const char *srutaLinuxArchivo);
	/*	Comprueba si Existe el archivo y si se tienen permisos de Lectura, Escritura y Ejecucion.
	 La ruta debe ser la ruta completa en linux al archivo. EJ: "/home/utnso/archivo.extension"
	 Devuelve 0 si existe el archivo y tiene todos los permisos.
	 Devuelve -1 Si no existe o si no tiene alguno de los 3 permisos.
	 La funcion ya de por si imprime por pantalla el error (si lo hubiese).
	 */
	 
	uint32_t Comun_obtener_tamanio_archivo( const char* sRutaArchivo );
	/*	Dada la ruta (dentro de Linux) de un archivo, usa la funcion "stat" para obtener el tamaño en Bytes del archivo.
		En caso de no poder obtenerse el tamaño, devuelve -1
	*/

	int Comun_borrarArchivo(const char* pathArchivo);
	/*	Borra el archivo que se encuentra en el pathArchivo indicado
	 *  Devuelve 0 en caso de borrar exitosamente el archivo
	 *  Devuelve -1 en caso de algun error
	 */

	int Comun_controlarPermisos();
	/*	Permite saber si en la ubicacion de ejecucion actual se tienen los Permisos para Crear,Cerrar,Leer,Escribir,Ejecutar y Borrar Archivos.
		Devuelve 0 en caso de tener todos los permisos.
		Devuelve -1 en Caso de no tener algun permiso.
		La funcion ya de por si imprime por pantalla el error (si lo hubiese).
	 */

	char* Comun_obtenerRutaDirectorio(const char* rutaCompleta);
	/*	La idea de esta funcion es reemplazar al "DirName" que anda mal.
		Dada una Ruta con "/" obtiene lo que haya antes del "/" final. EJ: Comun_obtenerRutaDirectorio("/carpeta1/carpeta2/archivo") devuelve "/carpeta1/carpeta2"
		NOTA: Como uso las Strings de las Commons que usan Malloc, deben hacer Free cuando terminen de usar este Char.
		NOTA2: En el caso de que termine o empieze con multiples "/" los mantiene. EJ: Comun_obtenerRutaDirectorio("/ruta/algo///") devuelve "/ruta/algo//". Lo deje asi para mantener compatibilidad con el string_split de las Commons
	*/

	void Comun_LiberarMemoria(void** punteroMemoria);
	/*	La idea de esta Funcion es que los Programas Exploten menos por hacer Mal los Free.
	  	Es muy util para hacer Free de los Chars* de las COMMONS
		Debe pasarse los Punteros por Referencia: &<Puntero a Memoria>
		Notar que al ser Void sirve para liberar cualquier tipo de puntero.
	 */
	void Comun_LiberarMemoriaDobleArray(void ***punteroMemoria, int sizeArray);

	void Comun_Pantalla_Separador_Destacar(const char* texto, ... );
	/* Imprime en pantalla un Texto en Azul para Destacarlo en la Consola, con Cuadraditos a los Costados, como un Menu o Separador
	   NOTA: permite Pasarle Argumentos '"%s" Onda StringPrintf
	 */
	void Comun_ImprimirProgreso(char* Mensaje, size_t TamanioTotal, size_t estado);
	void Comun_ImprimirMensajeConBarras(char* Mensaje);
	//char* Comun_obtener_MD5_Bloque(const tipo_bloque* punteroBloque, bool showProgress);


#endif 
//BIBLIOTECA_COMUN_H_


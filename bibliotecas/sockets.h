//Contiene todas las funciones relacionadas con el uso de Sockets. Para conectar, desconectar, enviar y recibir datos.
//TODAS las funciones ya manejan errores y los imprimen por pantalla. Asi que ni hace falta usar un if para atajar el error, a menos que quieras hacer algo especial ante un error determinado.


/*
=============================================ORDEN DE USO==================================================
-->Sea cliente o servidor, en ambos casos debo crear previamente la variable, y NO olvidarme al final de destruirla
	Nota: Todas las funciones despues llaman a la variable internamente!
	- La variable se crea con "SOCKET_crear(NombreDeVariable)"
	- La variable se elimna con "SOCKET_destruir(NombreDeVariable);


-->SIENDO UN SERVIDOR:
	-Primero me pongo a escuchar "NombreVariable->Escuchar(NombreDeVariable, Puerto"
	-Creo otro socket para aceptar y conectar al cliente usando "SOCKET_crear(Socket_Cliente)"
	-Acepto al Cliente y le paso su socket
		"NombreVariable->Aceptar_Cliente(NombreDeVariable, Socket_Cliente)"
		-Ya puedo usar tanto "NombreVariable->Enviar" como "NombreVariable->Recibir" las veces que quiera
		-Cuando dejo de atender a un cliente uso "NombreVariable->Desconectar" con el Socket Conectado al Cliente
	-Antes de Salir del programa uso "NombreVariable->Desconectar" y "SOCKET_destruir" con el Socket en Escucha
	

-->SIENDO UN CLIENTE:
	-Uso "NombreVariable->Conectar"
		-Ya puedo usar tanto "NombreVariable->Enviar" como "NombreVariable->Recibir" las veces que quiera
	-Antes de Salir del programa uso "NombreVariable->Desconectar" con el Socket conectado al Servidor
	
--NOTA: Cambio el envio y reccepcion de headers, ver funciones de enviar y recibir (mas abajo)
--NOTA: Cambio el manejo de errores, ahora 0 o Negativo significa error y Mayor a 0 significa OK.
=======================================================================================================
*/

#ifndef BIBLIOTECA_SOCKETS_H
#define BIBLIOTECA_SOCKETS_H

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "serializacion.h"
#include <stdbool.h>
#include <stdlib.h>

//Estructura que define el header del paquete de datos
typedef struct __attribute__ ((__packed__)) __t_payload_header {
	bool compressed;
	uint32_t payload_compressed_size;
	uint32_t payload_size; //tamanio del paquete completo
} t_payload_header;

//Estructura que define el paquete completo.
typedef struct __attribute__ ((__packed__)) __t_package {
	t_order_header* order_header; //header del mensaje
	t_payload_header payload_header;
	char* package_payload; //contenido del mensaje
} t_package;

//Esta a la vista por si necesitamos modificar el Header en el Futuro
#define TAMANIO_PAYLOAD_HEADER sizeof(t_payload_header)
#define TAMANIO_HEADER TAMANIO_ORDER_HEADER + TAMANIO_PAYLOAD_HEADER


//===================================CODIGOS DE ERRORES=====================================
//Estos son los Posibles Codigos de Error que devuelven las Funciones, notar que todos son menor a 0
//Estos codigos se setean automaticamente  en el atributo "soc_err" despues de llamar a las funciones/metodos
#define SE_IPPORT -1 		/* Direccion IP o Numero de Puerto Invalido*/
#define SE_GETADDR -2 		/* Error al procesar GetAddres */
#define SE_CONNECT -3 		/* Error en connect */
#define SE_NE -4 			/* Socket ya estaba cerrado o nunca se abrio */
#define SE_CLOSE -5 		/* Error al cerrar el Socket */
#define SE_BIND -6 			/* Error al hacer Bind */
#define SE_LISTEN -7 		/* Error al hacer listen */
#define SE_NULLPTR -8 		/* Puntero Nulo */
#define SE_ACCEPT -9 		/* Error al hacer accept */
#define SE_NULLVAL -10 		/* Error al pasar un parametro NULL que no corresponde */
#define SE_RECEP -11 		/* Error no especificado en la recepcion de los datos, ver el recv y errno, o algun problema en la cantidad de datos */
#define SE_CONNRESET -12 	/* Resetaron la conexion del otro lado del Socket al RECIBIR datos */
#define SE_NOMEM -13 		/* No se pudo realizar un malloc por falta de memoria */
#define SE_DECOMPRESS -14 	/* Error al descomprimir */
#define SE_DESSER -15 		/* Error al DesSerializar los datos */
#define SE_SERIALIZE -16 	/* Error al Serializar los datos */
#define SE_COMPRESS -17 	/* Error al comprimir */
#define SE_ENVIAR -18 		/* Error al enviar datos */
#define SE_SEND -19 		/* Error en funcion send */
#define SE_PEND -20 		/* Hay datos pendientes de envio en chunk */
//====================================================================================


//Estructura de la CLASE SOCKET
typedef struct __attribute__ ((__packed__)) __t_socket {
	//Variables Publicas:
	int soc_err;			//Para ver el Tipo de Error
	uint32_t chunk_size;
	bool ComprimirDatos;
	uint32_t nReintentos; 		// Para establecer la cantidad de Reintendos posibles, default es 1!

	//Variables Privadas de la Clase (NO TOCAR ni llamar dentro del codigo):
	uint32_t chunks_send;
	char *datosSerializados;
	uint32_t sizeDatosSerializados;
	int descriptorSocket;							// Es un tipo int por lo que el valor 0 significa no conectado;
	struct sockaddr_in tDireccionClienteRemoto; 	//Solo lo usa el servidor, para guardar la Direccion del Cliente cuando el se conecte (y yo lo acepte).


	//Estos Punteros hacen referencia a funciones del mismo nombre el el archivo ".c", solo que adelante llevan "SOCKET_"
	int (*Conectar)(struct __t_socket *datosDelSocket, const char* serverIP, const char* serverPort);
	int (*Desconectar)(struct __t_socket *datosDelSocket);
	int (*Escuchar)(struct __t_socket *datosDelSocket, const char* puertoDeEscucha);
	int (*Aceptar_Cliente)(struct __t_socket *datosDelSocketServidor, struct __t_socket *datosDelSocketCliente);
	char* ipCliente;
	int Puerto_Cliente;
	uint32_t (*Recibir)(struct __t_socket *datosDelSocket, t_order_header* headerRecibido, void** Resultado, uint32_t (*Deserializar)(const char *datosSerializados, uint32_t sizeDatosSerializados, void **Resultado));
	uint32_t (*Enviar)(struct __t_socket *datosDelSocket, void* datosParaSerializar, t_order_header headerEnviar,  uint32_t (*Serializar)(char** datosEnviar, uint32_t* sizeDatosEnviar, void* datosParaSerializar));
	uint32_t (*Enviar_Chunk)(struct __t_socket *datosDelSocket);
} t_socket;




//En vez de esta fucion, mejor llamen a la macro "SOCKET_crear" (esta mas abajo)
//Para llamarla Usar: struct t_socket* claseSocket = Init_Socket(malloc(sizeof(struct t_socket)));
t_socket* SOCKET_Inicializar(t_socket *datosSocket);

//Se utiliza para crear un tipo "clase socket", pongan en"x" el nombre de la variable que deseen
//OJO, no se olviden de Liberar la  memoria con la macro de mas abajo
#define SOCKET_crear(x) t_socket* (x) = SOCKET_Inicializar(malloc(sizeof(t_socket)));

//Se utiliza para liberar la memoria del socket. Usar DESPUES de llamar a 'Desconectar'
#define SOCKET_destruir(x) if(x != NULL){ if (x->ipCliente != NULL) { free(x->ipCliente); x->ipCliente = NULL; } free(x);  x=NULL; };



/*
===================================FUNCIONES DE LA CLASE SOCKET======================================
	#SOCKET_IP_Cliente
		-Dado un socketServidorConClienteAceptado devuelve la IP del Cliente. SI yo lo aplico a un Socket que no tenga un Cliente Aceptado, devuelve Basura.
		-Osea que NO puede aplicarse a Sockets donde YO soy el Cliente.

	#SOCKET_Puerto_Cliente
		-Dado un socketServidorConClienteAceptado devuelve el Puerto del Cliente. SI yo lo aplico a un Socket que no tenga un Cliente Aceptado, devuelve Basura.
		-Osea que NO puede aplicarse a Sockets donde YO soy el Cliente.

	#SOCKET_Recibir(t_socket* datosDelSocket, t_order_header* headerRecibido, void** Resultado, int (*Deserializar)(const char *datosSerializados, uint32_t sizeDatosSerializados, void **Resultado))
		-headerRecibido debe ser pasado por Referencia (con &)
		-void** Resultado: Tiene que ser SI O SI un puntero a algo (un struct por ejemplo) y ese puntero debe pasarse por refencia
		-Deserializar: Es la Funcion de DesSerializacion. Ver Comentario Aparte, mas abajo
		-Si Quiro SOLO Recibir el HEADER: Puedo poner NULL tanto el "Resultado" como en la funcion de DesSerializacion
		-Devuelve 0 si ocurrio un error o algo mayor a 0 si fue exitoso
		-En caso de ser 0, puedo ver el error especifico consultaco soc_err

	#SOCKET_Enviar( t_socket* datosDelSocket, void* datosParaSerializar, t_order_header headerEnviar, uint32_t (*Serializar)(char** datosEnviar, int* sizeDatosEnviar, void* datosParaSerializar))
		-datosParaSerializar: Debe pasarse un puntero a algo (un struct o un char)
		-Serializar: Es la Funcion de Serializacion, ver comentarios mas abajo.
		-NOTA: Si esta definido el atributo Chunk_Size en algo mayor a 0, entonces va a enviar los datos de ese taman�o, si
		 alcanza con el primer chunk, entonces se envia el paquete entero, sino quedan datos pendientes
		 que se envian utilizando la funcion Enviar_Chunk(NombreDelSocket)

	#SOCKET_Enviar_Chunk
		-Basicamente se encarga solo de cortar en pedazos lo que se necesite enviar.
		-Corta en pedazos del tama�o del atributo Chunk_Size.
		-Si esta definido en 0 no se utiliza esta funcion y envia sin cortar en pedazos.
		-El funcionamiento es asi: Vos llamas una sola vez a enviar y luego llamas a "SOCKET_Enviar_Chunk" hasta que el atributo privado "chunks_send" sea 0. Igual lo ideal es hacer un FOR dividiendo el tamanio de chunck porel del archivo asi sabes cuantos chuncks hay que enviar.
=================================================================================================
 */




/*
========================FUNCIONES DE (DES)SERIALIZACION====================================
	-NOTA: Pueden basarse en las funciones "SerializarString" y "DeSerializarString" existentes para crear las suyas.
	-Es muy importante que hagan ese malloc y que retornen 0 en NO error. Cualquier otro numero se considera error.

	#Serializacion
		- int Funcion(char** resultado, int* tamanioResultado, void* string)
		- el primer parametro es el parametro serializado final, el 2do es el tamanio de la serializacion y el 3ero es el dato a serializar

	#DesSerializacion
		- DeSerializarString(const char* datosSerializados, int sizeDatosSerializados, void** resultado)
		- el 1er parametro es el dato a desserializar, el 2do es el tamanio de la serializacion (podrias no necesitarlo, pero tenes que defirnilo igual) y el 3ero es el dato YA desserializado
============================================================================================
 */
#endif 
//BIBLIOTECA_SOCKETS_H

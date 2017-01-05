/*
Contiene todas las funciones relacionadas con el uso de Sockets. Para conectar, desconectar, enviar y recibir datos. 
*/
 
#include "sockets.h"
#include "serializacion.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>

#include <sys/wait.h>
#include <signal.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <syslog.h>
#include <sys/socket.h>
//#include <zlib.h>

#define NODEBUG
#ifdef NODEBUG
	#define Macro_ImprimirParaDebug(Mensaje, ...)
#else
	#define Macro_ImprimirParaDebug(Mensaje, ...) printf("[DEBUG] (%s:%s:%d): " Mensaje "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__); fflush(stdout);
#endif
#undef NODEBUG

#define ANSI_COLOR_RED     "\x1b[31;1m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define Macro_Obtener_Errno() 										(errno == 0 ? "Errno OK" : strerror(errno) )

#define CONEXIONES_MAXIMAS_DE_ESCUCHA	400

#define T_SOCKET_INIT { .nReintentos = 1, .chunk_size =0, .chunks_send = 0, .datosSerializados = NULL, .soc_err = 0, .descriptorSocket = 0, .ComprimirDatos = false, .Conectar = &SOCKET_Conectar_Servidor, .Desconectar = &SOCKET_Desconectar_Servidor, .Escuchar = &SOCKET_Escuchar, .Enviar = &SOCKET_Enviar, .Recibir = &SOCKET_Recibir, .Aceptar_Cliente = &SOCKET_Aceptar_Cliente, .Enviar_Chunk = &SOCKET_Enviar_Chunk, .ipCliente = NULL }

static int SOCKET_Conectar_Servidor(t_socket *datosDelSocket, const char* serverIP, const char* serverPort) {

	Macro_ImprimirParaDebug("Intentando Conectar al Servidor en IP: %s y Puerto: %s...\n", serverIP, serverPort );

	//Variable para Manejar Errores
	int iResultado;

	// Verifico que ninguno de los parametros sea NULL
	if (datosDelSocket == NULL) {
		Macro_ImprimirParaDebug("El dato del socket esta en NULL y no deberia");
		return SE_NULLVAL;
	}

	//TODO: Aca aplicaria verificacion que el parametro claseSocket corresponde y que existe atrapando el SEGFAULT en caso de error

	// Verifico que el socket que me estan pasando no se encuentr ya abierto
	if (datosDelSocket->descriptorSocket > 0) {
		Macro_ImprimirParaDebug("El socket ya se esta utilizando");
		datosDelSocket->soc_err = SE_NE;
		return SE_NE;
	}

	//Primero Hago Chequeos de IP
	if ((strlen(serverIP) < 9) || (strlen(serverIP) > 15)) {
		Macro_ImprimirParaDebug("Me mandaste cualquier cosa como IP, Fijate que me pediste la IP: %s \n", serverIP);
		datosDelSocket->soc_err = SE_IPPORT;
		return SE_IPPORT;
	}
	//Chequeo que no manden cualquier cosa de Puerto
	if ((strlen(serverPort) < 4) || (atol(serverPort) > 65532)) {
		Macro_ImprimirParaDebug("Me mandaste cualquier cosa como Puerto, Fijate que me pediste el puerto: %s \n", serverPort);
		datosDelSocket->soc_err = SE_IPPORT;
		return SE_IPPORT;
	}
	//Chequeo que no usen sockets Reservados
	if (atol(serverPort) < 1024) {
		Macro_ImprimirParaDebug("Estas usando un puerto Reservado (Menor a 1024), Me pediste el puerto: %s \n", serverPort);
		datosDelSocket->soc_err = SE_IPPORT;
		return SE_IPPORT;
	}

	//Estructuras para la busqueda de IPs disponibles donde escuchar (Esto es porque por alguna razon pueden haber duplicados[Ej: duplicadas las direcciones por IPv4 e IPv6] y algunos de esos duplicados pueden no funcionar).
	struct addrinfo busqueda;
	struct addrinfo *resultadoBusqueda = NULL;

	//Me aseguro que la structura "busqueda" este vacio, asi que la lleno con Ceros.
	memset(&busqueda, 0, sizeof(busqueda));
	//Establesco que el protocolo que quiero es IPv4
	busqueda.ai_family = AF_INET;
	//Establesco que el protocolo que quiero es TCP
	busqueda.ai_socktype = SOCK_STREAM;
	//Establesco mi IP, me llena mi IP por mi asi no hay que usar un parametro ni hardcodear..
	busqueda.ai_flags = AI_PASSIVE;


	//Funcion que sirve para cargarme todas los "struct de struct de struct" necesarios, asi no debo hacerlo a mano.
	//Supuestamente tambien hace un chequeo por DNS y revisa que exista y pueda conectarse al servidor en su IP.
	iResultado = getaddrinfo(serverIP, serverPort, &busqueda, &resultadoBusqueda);
	if (iResultado != 0) {
		Macro_ImprimirParaDebug("Hubo un error al usar Getaddrinfo para cargar las estructuras de datos del Nuevo Socket, el Error Detectado es: %s\n", gai_strerror(iResultado));
		freeaddrinfo(resultadoBusqueda);
		datosDelSocket->soc_err = SE_GETADDR;
		return SE_GETADDR;
	}

	//Ciclamos entre todos los Resultados obtenidos y me Enlazo (bind) al primero que puedo
	//Puntero para ciclar por los Resultados
	struct addrinfo *punteroResultado = NULL;
	for(punteroResultado = resultadoBusqueda; punteroResultado != NULL; punteroResultado = punteroResultado->ai_next) {
		//Creo un nuevo socket con los valores obtenidos por "getaddrinfo".
		datosDelSocket->descriptorSocket = socket(resultadoBusqueda->ai_family, resultadoBusqueda->ai_socktype, resultadoBusqueda->ai_protocol);
		if( datosDelSocket->descriptorSocket <= 0 ){
			Macro_ImprimirParaDebug("Error al crear el Socket.\n");
			Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());
			Macro_ImprimirParaDebug("Reintentando...\n");
			//Como hubo un error con este resultado, salto al proximo ciclo con el proximo resultado.
			continue;
		}

		//Establesco los Flags Necesarios en el socket recien creado.
		//SO_REUSEADDR hace que se pueda reutilizar inmediatamente el socket despues de hacerle un Close (asi no da error de "socket ocupado")

		//Variable para Establecer los Flag en True.
		int optVal=1;
		if (setsockopt(datosDelSocket->descriptorSocket, SOL_SOCKET, SO_REUSEADDR, &optVal,  sizeof(int)) == -1) {
			Macro_ImprimirParaDebug("No se pudieron establecer las Opciones/Flags Necesarias en el socket\n");
			Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());
			Macro_ImprimirParaDebug("Reintentando...\n");

			// Desconecto el socket
			close(datosDelSocket->descriptorSocket);
			datosDelSocket->descriptorSocket = 0;

			//Como hubo un error con este resultado, salto al proximo ciclo con el proximo resultado.
			continue;
		}

		//Me conecto al Servidor remoto con los valores obtenidos por "getaddrinfo".
		iResultado = connect(datosDelSocket->descriptorSocket, resultadoBusqueda->ai_addr, resultadoBusqueda->ai_addrlen);
		if( iResultado != 0 ){
			Macro_ImprimirParaDebug("Error al conectarse Al Servidor. \n");
			Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());
			Macro_ImprimirParaDebug("Reintentando...\n");

			// Desconecto el socket
			close(datosDelSocket->descriptorSocket);
			datosDelSocket->descriptorSocket = 0;
			continue;
		}

		//Si llego hasta aca es que no hubo errores y se puedo conectar, entonces salgo del Ciclo.
		break;
	}

	//Chequeo que no se hallan recorridos todos los resultados, porque entonces significa que no nos pudimos conectar de ninguna manera al servidor.
	if (punteroResultado == NULL) {
		Macro_ImprimirParaDebug( "No hubo ninguna manera de Conectarse al Servidor.\n");
		freeaddrinfo(resultadoBusqueda);
		datosDelSocket->soc_err = SE_CONNECT;
		return SE_CONNECT;
	}

	//Por los memory Leaks libero aca al struct "resultadoBusqueda" que use con "getaddrinfo".
	freeaddrinfo(resultadoBusqueda);

	// Para obtener el puerto real al cual queda conectado
	// Los datos que necesito	
	struct sockaddr_in sin;
	socklen_t len = sizeof(sin);
	
	// Obtengo el puerto
	getsockname(datosDelSocket->descriptorSocket, (struct sockaddr*) &sin, &len);
	
	// Finalmente lo asigno a la estructura para que pueda llamarse
	datosDelSocket->Puerto_Cliente = ntohs(sin.sin_port);

	//Como llegados aca me pude Conectar al Servidor, Imprimo y Devuelvo el Socket al servidor Ya conectado
	Macro_ImprimirParaDebug("Conectado al Servidor en IP: %s y Puerto: %s \n", serverIP, serverPort );

	return 1;
}

static int SOCKET_Desconectar_Servidor(t_socket* datosDelSocket) {

	int iResultado;

	//Verifico que el parametro no sea null, porque daria un SegFault sino
	if (datosDelSocket == NULL) {
		Macro_ImprimirParaDebug("Error, No se puede Cerrar un Socket que es NULL (fijate que el puntero que pasaste para cerrar apunta a NULL)\n");
		return SE_NULLVAL;
	}

	//Primero Reviso que la conexion este abierta
	if( datosDelSocket->descriptorSocket <= 0 ){
		Macro_ImprimirParaDebug("Error, No se puede Cerrar un Socket que ya esta Cerrado\n");
		//Corto la ejecucion, sino da error en el "close" mas abajo.
		datosDelSocket->soc_err = SE_NE;
		return SE_NE;
	}

	iResultado = close(datosDelSocket->descriptorSocket);
	if( iResultado == -1 ){
		Macro_ImprimirParaDebug("Hubo un problema y no se pudo Cerrar el Socket. \n Esto es muy raro, no deberia pasar nunca. \n");
		Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());
		datosDelSocket->soc_err = SE_CLOSE;
		return SE_CLOSE;
	}
	datosDelSocket->descriptorSocket = 0;
	return 1;
}

static int SOCKET_Escuchar(t_socket *datosDelSocket, const char* puertoDeEscucha) {

	//Variable para Manejar Errores
	int iResultado;

	//Verifico que el parametro no sea null, porque daria un SegFault sino
	if (datosDelSocket == NULL) {
		Macro_ImprimirParaDebug("Pasate un parametro NULL cuando debia ser un socket");
		return SE_NULLVAL;
	}

	// Verifico que el socket que me estan pasando no se encuentr ya abierto
	if (datosDelSocket->descriptorSocket > 0) {
		Macro_ImprimirParaDebug("El socket ya se esta utilizando");
		datosDelSocket->soc_err = SE_NE;
		return SE_NE;
	}

	//Chequeo que no manden cualquier cosa de Puerto
	if ((strlen(puertoDeEscucha) < 4) || (atol(puertoDeEscucha) > 65532)) {
		Macro_ImprimirParaDebug("Me mandaste cualquier cosa como Puerto, Fijate que me pediste el puerto: %s \n", puertoDeEscucha);
		datosDelSocket->soc_err = SE_IPPORT;
		return SE_IPPORT;
	}
	//Chequeo que no usen sockets Reservados
	if (atol(puertoDeEscucha) < 1024) {
		Macro_ImprimirParaDebug("Estas usando un puerto Reservado (Menor a 1024), Me pediste el puerto: %s \n", puertoDeEscucha);
		datosDelSocket->soc_err = SE_IPPORT;
		return SE_IPPORT;
	}


	//Estructuras para la busqueda de IPs disponibles donde escuchar (Esto es porque por alguna razon pueden haber duplicados[Ej: duplicadas las direcciones por IPv4 e IPv6] y algunos de esos duplicados pueden no funcionar).
	struct addrinfo busqueda;
	struct addrinfo *resultadoBusqueda = NULL;

	//Me aseguro que la structura "busqueda" este vacio, asi que la lleno con Ceros.
	memset(&busqueda, 0, sizeof(busqueda));

	//Establesco que el protocolo que quiero es IPv4
	busqueda.ai_family = AF_INET;
	//Establesco que el protocolo que quiero es TCP
	busqueda.ai_socktype = SOCK_STREAM;
	//Establesco mi IP, me llena mi IP por mi asi no hay que usar un parametro ni hardcodear..
	busqueda.ai_flags = AI_PASSIVE;


	//Funcion que sirve para cargarme todas los "struct de struct de struct" necesarios, asi no debo hacerlo a mano.
	//Supuestamente tambien hace un chequeo para ver si puede escuchar el servidor en el Puerto que le indico.
	iResultado = getaddrinfo(NULL, puertoDeEscucha, &busqueda, &resultadoBusqueda);
	if (iResultado != 0) {
		Macro_ImprimirParaDebug("getaddrinfo error: %s\n", gai_strerror(iResultado));
		freeaddrinfo(resultadoBusqueda);
		datosDelSocket->soc_err = SE_GETADDR;
		return SE_GETADDR;
	}


	//Ciclamos entre todos los Resultados obtenidos y me Enlazo (bind) al primero que puedo
	//Puntero para ciclar por los Resultados
	struct addrinfo *punteroResultado = NULL;
	for(punteroResultado = resultadoBusqueda; punteroResultado != NULL; punteroResultado = punteroResultado->ai_next) {
		//Intento crear un socket con los resultados obtenidos (Familia IPv4, Socket Tipo TCP, Solo para el Protocolo TCP IP)
		if ((datosDelSocket->descriptorSocket = socket(punteroResultado->ai_family, punteroResultado->ai_socktype, punteroResultado->ai_protocol)) == -1) {
			Macro_ImprimirParaDebug("No se pudo crear un Socket con esta Direccion, Reintentando... \n");
			Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());
			//Como hubo un error con este resultado, salto al proximo ciclo con el proximo resultado.
			continue;
		}

		//Establesco los Flags Necesarios en el socket recien creado.
		//SO_REUSEADDR hace que se pueda reutilizar inmediatamente el socket despues de hacerle un Close (asi no da error de "socket ocupado")
		//Variable para Establecer los Flag en True.
		int optVal=1;
		if (setsockopt(datosDelSocket->descriptorSocket, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(int)) == -1) {
			Macro_ImprimirParaDebug("No se pudieron establecer las Opciones/Flags Necesarias en el socket, Reintentando...\n");
			Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());

			// Desconecto el socket
			close(datosDelSocket->descriptorSocket);
			datosDelSocket->descriptorSocket = 0;

			//Como hubo un error con este resultado, salto al proximo ciclo con el proximo resultado.
			continue;
		}

		//Debemos hacer Bind porque luego vamos a hacer Listen, asi sabe que puerto va a escuchar.
		if (bind(datosDelSocket->descriptorSocket, punteroResultado->ai_addr, punteroResultado->ai_addrlen) == -1) {
			Macro_ImprimirParaDebug("No se pudo enlazar en esta Direccion, Reintentando...\n");
			Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());

			// Desconecto el socket
			close(datosDelSocket->descriptorSocket);
			datosDelSocket->descriptorSocket = 0;

			//Como hubo un error con este resultado, salto al proximo ciclo con el proximo resultado.
			continue;
		}

		//Si llego hasta aca es que no hubo errores y se puedo enlazar, entonces salgo del Ciclo.
		break;
	}

	//Chequeo que no se hallan recorridos todos los resultados, porque entonces significa que no nos pudimos enlazar a ninguno.
	if (punteroResultado == NULL) {
		Macro_ImprimirParaDebug("No se pudo Enlazar el Socket.\n");
		freeaddrinfo(resultadoBusqueda);
		datosDelSocket->soc_err = SE_BIND;
		return SE_BIND;
	}

	//Libero de la memoria la estructura usada
	freeaddrinfo(resultadoBusqueda);

	//Hacemos el Listen
	if (listen(datosDelSocket->descriptorSocket, CONEXIONES_MAXIMAS_DE_ESCUCHA) == -1) {
		Macro_ImprimirParaDebug("No se pudo poner en Escucha al Socket.\n");
		Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());

		//Como hubo un error al hacer el Listen, desconecto el socket
		close(datosDelSocket->descriptorSocket);
		datosDelSocket->descriptorSocket = 0;

		datosDelSocket->soc_err = SE_LISTEN;
		return SE_LISTEN;
	}

	//Como ya se pudo enlazar y poner en escucha el socket, imprimo y lo devuelvo.
	Macro_ImprimirParaDebug("Estoy escuchando en Puerto: %s\n", puertoDeEscucha);

	return 1;
}



static int SOCKET_IP_Cliente(t_socket* datosDelSocket, char **ipCliente ){

	//Verifico que el parametro no sea null, porque daria un SegFault sino
	if (datosDelSocket == NULL) {
		Macro_ImprimirParaDebug("No puedo obtener la IP de un Socket NULL, fijate que me pasaste un puntero NULL.\n");
		return 0;
	}

	//Chequeo Argumentos
	if(datosDelSocket->descriptorSocket <= 0){
		Macro_ImprimirParaDebug("No puedo obtener la IP de un Socket Cerrado.\n");
		datosDelSocket->soc_err = SE_NE;
		return SE_NE;
	}

	//String donde guardo la IP del Cliente Remoto, defino de IPv6 para que seguro alcanze el tamaño
	//Es una pelotudes pero necesito SI o SI una variable porque asi trabaja inet_ntop.
	//static char ipCliente[INET6_ADDRSTRLEN];
	*ipCliente = malloc(sizeof(char)*INET6_ADDRSTRLEN);

	//Inicializo la variable con Ceros, para evitar problemas al ser static
	memset(*ipCliente, '0', INET6_ADDRSTRLEN);

	//Obtengo la IP del cliente remoto
	inet_ntop(AF_INET, &(datosDelSocket->tDireccionClienteRemoto.sin_addr), *ipCliente, INET6_ADDRSTRLEN);

	//NOTA Especial: Tuve que definir el Char* a devolver como static porque sino devuelve basura (tiene que ver conque si no uso static se guarda en la pila y al terminarse el programa se libera la pila, con static guarda la variable en el heap y cuando se termina el programa entonces puede devolver el valor)
	return 1;
}



static int SOCKET_Puerto_Cliente(t_socket* datosDelSocket ){

	//Verifico que el parametro no sea null, porque daria un SegFault sino
	if (datosDelSocket == NULL) {
		Macro_ImprimirParaDebug("No puedo obtener el Puerto de un Socket NULL, fijate que me pasaste un puntero NULL.\n");
		return SE_NULLVAL;
	}

	//Chequeo Argumentos
	if(datosDelSocket->descriptorSocket == 0){
		Macro_ImprimirParaDebug("No puedo obtener el Puerto de un Socket Cerrado.\n");
		datosDelSocket->soc_err = SE_NE;
		return SE_NE;
	}

	return ntohs(datosDelSocket->tDireccionClienteRemoto.sin_port);
}


static int SOCKET_Aceptar_Cliente(t_socket *datosDelSocketServidor, t_socket *datosDelSocketCliente) {

	//Verifico que el parametro no sea null, porque daria un SegFault sino
	if (datosDelSocketServidor == NULL) {
		Macro_ImprimirParaDebug("Pasate un parametro NULL cuando debia ser un socket");
		return SE_NULLVAL;
	}

	//Primero chequeo que el servidor este escuchando
	if( datosDelSocketServidor->descriptorSocket <= 0 ){
		Macro_ImprimirParaDebug("Error, No puedo Aceptar Clientes por un Socket que esta Cerrado\n");
		//Corto la ejecucion porque daria error al recibir datos sino (mas abajo).
		datosDelSocketServidor->soc_err = SE_NE;
		return SE_NE;
	}

	// Verificar que la direccion de memoria de datosDelSocketCliente este creada
	if (datosDelSocketCliente == NULL) {
		Macro_ImprimirParaDebug("Error, No puedo guardar los datos del Cliente en una puntero NULL (Debe inicializarse antes)\n");
		//Corto la ejecucion porque daria error al recibir datos sino (mas abajo).
		datosDelSocketServidor->soc_err = SE_NULLPTR;
		return SE_NULLPTR;
	}

	//Estructuras necesarias para pasarle al Accept
	struct sockaddr_in tDireccionClienteRemoto;
	socklen_t tamanioDireccionCliente = sizeof(tDireccionClienteRemoto);

	int nRetry = 0;

	do {
		//Acepto al Cliente
		datosDelSocketCliente->descriptorSocket = accept(datosDelSocketServidor->descriptorSocket, (struct sockaddr*)&tDireccionClienteRemoto, &tamanioDireccionCliente);
		//Chequeo Errores
		if (datosDelSocketCliente->descriptorSocket == -1) {
			if ((errno == EINTR) && (nRetry < datosDelSocketServidor->nReintentos)) {
				Macro_ImprimirParaDebug("Se detecto una interrupcion por SysCall al Aceptar Cliente, reintento!\n");
				datosDelSocketServidor->soc_err = 0;
				errno = 0;
				nRetry++;
			} else {
				Macro_ImprimirParaDebug("No se pudo aceptar al Cliente\n");
				Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());
				datosDelSocketServidor->soc_err = SE_ACCEPT;
				return SE_ACCEPT;
			}
		} else {
			nRetry = (datosDelSocketServidor->nReintentos + 1);
		}
	} while (nRetry <= datosDelSocketServidor->nReintentos);
	//Guardo dentro del tipo_socket la direccion (IP y Puerto) del cliente remoto
	datosDelSocketCliente->tDireccionClienteRemoto = tDireccionClienteRemoto;

	//char* ipCliente = malloc(sizeof);
	char *ipCliente;
	SOCKET_IP_Cliente(datosDelSocketCliente, &ipCliente);
	//Macro_ImprimirParaDebug("Se acepto una Conexion por Socket desde la IP: %s  Puerto: %d \n", ipCliente, SOCKET_Puerto_Cliente(datosDelSocketCliente));
	datosDelSocketCliente->ipCliente = ipCliente;
	datosDelSocketCliente->Puerto_Cliente = SOCKET_Puerto_Cliente(datosDelSocketCliente);
	Macro_ImprimirParaDebug("Se acepto una Conexion por Socket desde la IP: %s  Puerto: %d \n", datosDelSocketCliente->ipCliente , datosDelSocketCliente->Puerto_Cliente );
	//free(ipCliente);
	return 1;
}

// Funcion interna para recibir los Datos en un Socket y Controlar todos los errores posibles
static uint32_t SOCKET_Procesar_Recepcion_Datos(t_socket* datosDelSocket, char* Buffer, const uint32_t tamanioBuffer) {
	// No hago chequeo de datosDelSocket, del Buffer o del tamanio porque es interno a mi clase y siempre paso estos datos!
	// Lo unico que podria explotar es sino esta hecho el define de TAMANIO_HEADER, pero en ese caso, ni siquiera compila!

	Macro_ImprimirParaDebug("Recibiendo %d Bytes..\n", tamanioBuffer);
	//Hago el Recv
	//Para que no llegan paquetes a la mitad uso el Flag MSG_WAITALL, que SIEMPRE espera a que llegen todos los datos completos especificados en "tamanioBuffer",
	// y NO se queda tildado si esperamos un paquete de 400 y solo envian 50. (Lo probe y sigue detectando bien los cortes de conexion)
	uint32_t bytesRecibidos = recv(datosDelSocket->descriptorSocket, Buffer, tamanioBuffer, MSG_WAITALL);

	//Controlo Errores el tipo nunca va a ser menor a 0
	if ((bytesRecibidos == 0) || (bytesRecibidos == -1)) {
		Macro_ImprimirParaDebug("Se desconectaron del otro lado del Socket.\n");
		//Ahora uso el "errno" que es el Numero de Error Estandar para atrapar el caso que se Resetio la Conexion
		if( errno==ECONNRESET ){
			Macro_ImprimirParaDebug("Se Resetio la Conexion del otro lado a medio Recibir. \nPuede haberse terminado el proceso o el thread (del otro lado del socket) por alguna causa. \n");
			Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());

			datosDelSocket->soc_err = SE_CONNRESET;
		}else{
			Macro_ImprimirParaDebug("Error al recibir Datos\n");
			Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());

			datosDelSocket->soc_err = SE_RECEP;
		}
	}

	return bytesRecibidos;
}

static uint32_t SOCKET_Recibir(t_socket* datosDelSocket, t_order_header* headerRecibido, void** Resultado, uint32_t (*Deserializar)(const char *datosSerializados, uint32_t sizeDatosSerializados, void **Resultado)) {
	int iResultado = 0;

	//Verifico que el parametro no sea null, porque daria un SegFault sino
	if (datosDelSocket == NULL) {
		Macro_ImprimirParaDebug("Error, No se puede Recibir Datos por un Socket que es NULL (fijate que el puntero que pasaste apunta a NULL)\n");
		return 0;
	}

	//Primero chequeo que no sea NULL el puntero al Socket. Luego hago otros Chequeos.
	if( datosDelSocket->descriptorSocket <= 0 ){
		Macro_ImprimirParaDebug("Error, No se puede Recibir Datos por un Socket que esta Cerrado\n");
		//Corto la ejecucion porque daria error al recibir datos sino (mas abajo).
		datosDelSocket->soc_err = SE_NE;
		return 0;
	}

	//Primero deserealizo el Header
	//Variable para el header serializado
	char sHeaderSerializado[TAMANIO_HEADER];

	uint32_t bytesRecibidos;
	int nRetry = 0;

	do {
		//Llamo a mi funcion interna para recibir los datos, para que haga el "trabajo sucio"
		bytesRecibidos = SOCKET_Procesar_Recepcion_Datos(datosDelSocket, (char*) sHeaderSerializado, TAMANIO_HEADER);

		//Controlo Errores
		if ((bytesRecibidos == 0) || (bytesRecibidos == -1)) {
			if ((errno == EINTR) && (nRetry < datosDelSocket->nReintentos)) {
				Macro_ImprimirParaDebug("Se recibieron una interrupcion por SysCall, se reintenta una vez! \n");
				datosDelSocket->soc_err = 0;
				errno=0;
				nRetry++;
			} else {
				//Si los Bytes recibidos son 0 hubo un error, asi que yo cierro el Socket, asi evitamos problemas.
				SOCKET_Desconectar_Servidor(datosDelSocket);
				// El error del socket queda definido por la funcion SOCKET_Procesar_Recepcion_Datos
				//datosDelSocket->soc_err = SE_RECEP;
				return 0;
			}
		} else {
			nRetry = (datosDelSocket->nReintentos + 1);
		}
	} while (nRetry <= datosDelSocket->nReintentos);

	Macro_ImprimirParaDebug("Se recibieron %d bytes! Corresponden al Header. \n", bytesRecibidos);

	if (bytesRecibidos != TAMANIO_HEADER) {
		Macro_ImprimirParaDebug("Se recibieron menos bytes de los esperados para el Header\n");
		datosDelSocket->soc_err = SE_RECEP;
		return 0;
	}

	t_package paqueteRecibido;
	memcpy(headerRecibido,sHeaderSerializado,TAMANIO_ORDER_HEADER);
	paqueteRecibido.order_header = headerRecibido;
	memcpy(&paqueteRecibido.payload_header,sHeaderSerializado+TAMANIO_ORDER_HEADER,TAMANIO_PAYLOAD_HEADER);

	// Si no pido devolver resultados, o si la funcion Deserializar es NULL entonces ya salgo de la funcion
	// Lo mas seguro es que no mande ningun dato, y solo fue una orden
	if ((Resultado == NULL) || (Deserializar == NULL)) {
		return bytesRecibidos;
	}

	uint32_t bytesRecibidos_payload;
	char *Buffer = NULL;

	// Verifico que estoy enviando un payload, sino es solo el header que deserializo
	if ((paqueteRecibido.payload_header.compressed == true) && (paqueteRecibido.payload_header.payload_compressed_size > 0)) {
/*
		//Reservo la Memoria para el Payload
		Buffer = malloc(sizeof(char)*paqueteRecibido.payload_header.payload_size);
		char *Buffer_Comp  = malloc(sizeof(char)*paqueteRecibido.payload_header.payload_compressed_size);
		// Verifico que se reservara la memoria exitosamente
		if ((Buffer == NULL) || (Buffer_Comp == NULL)) {
			Macro_ImprimirParaDebug("Error al hacer malloc del Buffer o del Buffer_Comp\n");
			if (Buffer != NULL )
				free(Buffer);
			if (Buffer_Comp != NULL)
				free(Buffer_Comp);
			datosDelSocket->soc_err = SE_NOMEM;
			return 0;
		}
		//Llamo a mi funcion interna para recibir los datos, para que haga el "trabajo sucio"
		bytesRecibidos_payload = SOCKET_Procesar_Recepcion_Datos(datosDelSocket, Buffer_Comp, paqueteRecibido.payload_header.payload_compressed_size);

		//Controlo Errores
		if (bytesRecibidos_payload == 0) {
			free(Buffer);
			free(Buffer_Comp);
			//Si los Bytes recibidos son 0 o menos, hubo un error, asi que yo cierro el Socket, asi evitamos problemas.
			SOCKET_Desconectar_Servidor(datosDelSocket);
			return bytesRecibidos_payload;
		}

		Macro_ImprimirParaDebug("Se recibieron %d bytes! Corresponden al Payload. \n", bytesRecibidos_payload);
		Macro_ImprimirParaDebug("Los datos Comprimidos son '%d' y los datos totales son '%d'", paqueteRecibido.payload_header.payload_compressed_size, paqueteRecibido.payload_header.payload_size);


		if (bytesRecibidos_payload != paqueteRecibido.payload_header.payload_compressed_size) {
			Macro_ImprimirParaDebug("Se recibieron menos bytes de los esperados para el payload Comprimido \n");
			free(Buffer);
			free(Buffer_Comp);
			datosDelSocket->soc_err = SE_RECEP;
			return 0;
		}

		// Descomprimo los datos a Buffer
		iResultado = uncompress((Bytef *)Buffer,(uLong *)&paqueteRecibido.payload_header.payload_size,(Bytef *)Buffer_Comp, paqueteRecibido.payload_header.payload_compressed_size);
		if (iResultado != Z_OK) {
			Macro_ImprimirParaDebug("Error al descomprimir la informacion del Payload\n");
			free(Buffer);
			free(Buffer_Comp);
			datosDelSocket->soc_err = SE_DECOMPRESS;
			return 0;
		}

		free(Buffer_Comp);
		Macro_ImprimirParaDebug("Se pudo descomprimir la informacion del Payload! \n");
*/
		// TODO: Devoler error de que se recibieron datos comprimidos y la funcion no esta implementada
	} else if (paqueteRecibido.payload_header.payload_size > 0) {
		Buffer = malloc(sizeof(char)*paqueteRecibido.payload_header.payload_size);
		if (Buffer == NULL) {
			Macro_ImprimirParaDebug("Error al hacer malloc del Buffer o del Buffer_Comp\n");
			datosDelSocket->soc_err = SE_NOMEM;
			return 0;
		}

		bytesRecibidos_payload = SOCKET_Procesar_Recepcion_Datos(datosDelSocket, Buffer, paqueteRecibido.payload_header.payload_size);

		//Controlo Errores
		if (bytesRecibidos <= 0) {
			free(Buffer);
			//Si los Bytes recibidos son 0 o menos, hubo un error, asi que yo cierro el Socket, asi evitamos problemas.
			SOCKET_Desconectar_Servidor(datosDelSocket);
			return bytesRecibidos_payload;
		}

		Macro_ImprimirParaDebug("Se recibieron %d bytes! Corresponden al Payload. \n", bytesRecibidos_payload);
		Macro_ImprimirParaDebug("Los datos totales son '%d'", paqueteRecibido.payload_header.payload_size);

		if (bytesRecibidos_payload != paqueteRecibido.payload_header.payload_size) {
			Macro_ImprimirParaDebug("Se recibieron menos bytes de los esperados para el payload \n");
			free(Buffer);
			datosDelSocket->soc_err = SE_RECEP;
			return 0;
		}

	} else {
		// No se encontro ningun payload para descargar
		return bytesRecibidos;
	}

	Macro_ImprimirParaDebug("Voy a deserializar los datos del Payload \n");
//	if (strlen(Buffer) == 0) {
//		Macro_ImprimirParaDebug("Error del Payload: No hay Datos para Deserializar \n");
//		datosDelSocket->soc_err = SE_DESSER;
//		return 0;
//	}
	iResultado = Deserializar(Buffer, paqueteRecibido.payload_header.payload_size, Resultado);
	if (iResultado <= 0) {
		Macro_ImprimirParaDebug("Error al deseralizar los datos del Payload \n");
		datosDelSocket->soc_err = SE_DESSER;
		return 0;
	}

	free(Buffer);

	Macro_ImprimirParaDebug("Se Deserializaron exitosamente los datos del Payload \n");

	return (bytesRecibidos + bytesRecibidos_payload);
}

// Funcion interna para enviar Datos por un Socket y Controlar todos los errores posibles
static uint32_t SOCKET_Procesar_Envio_Datos( t_socket* datosDelSocket, const char* Buffer, const uint32_t bytesPorEnviar) {
	//Variables para Control de Errores
	uint32_t uiReturn = 0;
	uint32_t bytesEnviados = 0;

	//Todos los Datos puede que no se envien por una unica invocacion de "send" (culpa del Maximo de 64KB por paquete de TCPIP),
	// asi que hago un While hasta que se envien todos los Datos
	while (bytesEnviados < bytesPorEnviar) {
		Macro_ImprimirParaDebug("Enviando %d Bytes..\n", bytesPorEnviar-bytesEnviados);
		//Voy a ir moviendome mas adelante de lo que tengo por enviar si no lo envia de una, por eso sumo al puntero char*. Tambien voy restando los bytes que ya envie, porque me queda menos por enviar. Asi hasta que se envie completo.
		//NOTA: Si hay error porque del otro lado del socket dejaron de hacer "recv" y me esta generando el signal "SIGPIPE", cambiar el 0 por el flag MSG_NOSIGNAL asi almenos no se genera el Signal, luego el error se va a atrapar por "retorno".
		uiReturn = send(datosDelSocket->descriptorSocket, (char*)(Buffer+bytesEnviados), bytesPorEnviar-bytesEnviados, 0);

		//Controlo Errores
		if (( uiReturn == 0 ) || (uiReturn == -1)) {
			//Si no envie ningun byte, es que hubo un problema, corto el envio y devuelvo el numero de error.
			//NOTA: Lo que se envio al destino, ya se envio y no tengo manera de cancelarlo, OJO con eso.
			Macro_ImprimirParaDebug("Error al enviar Datos (se corto el Paquete Enviado), solo se enviaron %d bytes de los %d bytes totales por enviar\n", bytesEnviados, (int)bytesPorEnviar);
			Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());
			//Aca siempre deberia enviar -1, pero por las dudas lo dejo asi que es mas compatible.
			bytesEnviados = uiReturn;
			datosDelSocket->soc_err = SE_SEND;
			break;
		}
		//Si no hay problemas, sigo acumulando bytesEnviados
		bytesEnviados += uiReturn;
	}
	//Ahora controlo que la cantidad de bytesEnviados coincida con los bytesPorEnviar
	if( bytesEnviados!=bytesPorEnviar ){
		Macro_ImprimirParaDebug("No se enviaron todos los bytes pedidos. Esto es Raro, No deberia pasar.\n Se enviaron %d bytes y vos pediste enviar %d bytes\n", bytesEnviados, bytesPorEnviar);
		return 0;
	}

	return bytesEnviados;
}

static uint32_t SOCKET_Enviar_Datos( t_socket* datosDelSocket, const char* datosSerializados, uint32_t sizeDatosSerializados) {

	uint32_t bytesPorEnviar = sizeDatosSerializados;

	//Controlo que no enviaron cosas raras
	if (bytesPorEnviar <= 0) {
		Macro_ImprimirParaDebug("Error al Enviar, fijate que en el header de los datos serializdos (2do argumento) me pasaste un tamaño 0 o negativo.\n Tamanio detectado: %d", (int)bytesPorEnviar );
		datosDelSocket->soc_err = SE_SERIALIZE;
		return 0;
	}

	uint32_t bytesEnviados = 0;
	int nRetry = 0;
	do {
		//Llamo a mi funcion interna para enviar los datos, para que haga el "trabajo sucio"
		bytesEnviados = SOCKET_Procesar_Envio_Datos(datosDelSocket, datosSerializados, bytesPorEnviar);

		//Controlo Errores
		if ((bytesEnviados == 0) || (bytesEnviados == -1)) {
			if ((errno == EINTR) && (nRetry < datosDelSocket->nReintentos)) {
				// reintento enviar nuevamente
				Macro_ImprimirParaDebug("Se recibio una interrupcion por SysCall al enviar, se reintenta una vez! \n");
				datosDelSocket->soc_err = 0;
				errno=0;
				nRetry++;
			} else {
				//Si los Bytes enviados son 0 o menos, hubo un error, asi que yo cierro el Socket, asi evitamos problemas.
				SOCKET_Desconectar_Servidor(datosDelSocket);
			}
		} else {
			nRetry = (datosDelSocket->nReintentos + 1);
		}
	} while (nRetry <= datosDelSocket->nReintentos);

	return bytesEnviados;
}

static uint32_t SOCKET_Enviar( t_socket* datosDelSocket, void* datosParaSerializar, t_order_header headerEnviar, uint32_t (*Serializar)(char** datosEnviar, uint32_t* sizeDatosEnviar, void* datosParaSerializar)) {
	uint32_t uiReturn;
	t_package paqueteDatos;
	//char *datosSerializados;
	//int sizeDatosSerializados = 0;
	char *offset_buffer;
	int iResultado = 0;

	//Verifico que el parametro no sea null, porque daria un SegFault sino
	if (datosDelSocket == NULL) {
		Macro_ImprimirParaDebug("Error, No se puede Enviar Datos por un Socket que es NULL (fijate que el puntero que pasaste apunta a NULL)\n");
		return 0;
	}

	//Primero chequeo que no sea NULL el puntero al Socket.
	if( datosDelSocket->descriptorSocket <= 0 ){
		Macro_ImprimirParaDebug("Error, No se puede Enviar Datos por un Socket que esta Cerrado\n");
		//Corto la ejecucion porque daria error al enviar datos sino (mas abajo).
		datosDelSocket->soc_err = SE_NE;
		return 0;
	}

	// Verifico que no haya una transferencia pendiente
	if (datosDelSocket->datosSerializados != NULL) {
		Macro_ImprimirParaDebug("Aun hay datos pendientes de envio");
		datosDelSocket->soc_err = SE_PEND;
		return 0;
	}

	Macro_ImprimirParaDebug("Serializo los datos del Header\n");
	paqueteDatos.order_header = &headerEnviar;
	paqueteDatos.payload_header.compressed = datosDelSocket->ComprimirDatos;

	if (datosParaSerializar == NULL) {
		paqueteDatos.payload_header.payload_compressed_size = 0;
		paqueteDatos.payload_header.payload_size = 0;
		datosDelSocket->datosSerializados = malloc(TAMANIO_HEADER);
		if (datosDelSocket->datosSerializados == NULL) {
			Macro_ImprimirParaDebug("Error al hacer Malloc\n");
			datosDelSocket->soc_err = SE_NOMEM;
			return 0;
		}
		datosDelSocket->sizeDatosSerializados = TAMANIO_HEADER;
		offset_buffer = mempcpy(datosDelSocket->datosSerializados,paqueteDatos.order_header,TAMANIO_ORDER_HEADER);
		offset_buffer = mempcpy(offset_buffer ,&paqueteDatos.payload_header,TAMANIO_PAYLOAD_HEADER);
		Macro_ImprimirParaDebug("El Header Unicamente fue Serializado con exito! \n");
	} else {
		char *payload_Serializado = NULL;
		uint32_t size_payload_Serializado = 0;

		Macro_ImprimirParaDebug("Serializo los datos del payload a enviar...\n");
		iResultado = Serializar(&payload_Serializado, &size_payload_Serializado, datosParaSerializar);
		if (iResultado <= 0) {
			Macro_ImprimirParaDebug("Error al Serializar los datos del payload \n");
			datosDelSocket->soc_err = SE_SERIALIZE;
			return 0;
		}
		Macro_ImprimirParaDebug("Se termino de serializar los datos del payload a enviar!\n");

		if (payload_Serializado == NULL) {
			Macro_ImprimirParaDebug("Hay algun problema con el mensaje del payload que esta vacio\n");
			datosDelSocket->soc_err = SE_SERIALIZE;
			return 0;
		}

		if (paqueteDatos.payload_header.compressed == true) {
/*			Macro_ImprimirParaDebug("Comienzo a procesar los datos Serializados y Comprimirlos\n");
			uLong sizeCompress = compressBound(size_payload_Serializado);
			char *payload_Compress = malloc(sizeof(char)*sizeCompress);
			if (payload_Compress == NULL) {
				Macro_ImprimirParaDebug("Error en Malloc, seguramente no hay memoria\n");
				free(payload_Serializado);
				datosDelSocket->soc_err = SE_NOMEM;
				return 0;
			}

			Macro_ImprimirParaDebug("Comienzo la compresion de los datos totales: '%d', alocado: '%d'", size_payload_Serializado, (int)sizeCompress);
			iResultado = compress2((Bytef *)payload_Compress, &sizeCompress, (Bytef *)payload_Serializado, size_payload_Serializado, Z_BEST_SPEED);
			if (iResultado != Z_OK) {
				free(payload_Compress);
				free(payload_Serializado);
				Macro_ImprimirParaDebug("Error al comprimir los datos codigo: %d", iResultado);
				datosDelSocket->soc_err = SE_COMPRESS;
				return 0;
			}

			free(payload_Serializado);

			paqueteDatos.payload_header.payload_compressed_size = sizeCompress;
			paqueteDatos.payload_header.payload_size = size_payload_Serializado;
			datosDelSocket->datosSerializados = malloc((sizeof(char)*paqueteDatos.payload_header.payload_compressed_size) + TAMANIO_HEADER);
			if (datosDelSocket->datosSerializados == NULL) {
				Macro_ImprimirParaDebug("Error en Malloc, posiblemente no hay memoria \n");
				free(payload_Compress);
				datosDelSocket->soc_err = SE_NOMEM;
				return 0;
			}

			datosDelSocket->sizeDatosSerializados = paqueteDatos.payload_header.payload_compressed_size + TAMANIO_HEADER;
			offset_buffer = mempcpy(datosDelSocket->datosSerializados,paqueteDatos.order_header,TAMANIO_ORDER_HEADER);
			offset_buffer = mempcpy(offset_buffer ,&paqueteDatos.payload_header,TAMANIO_PAYLOAD_HEADER);
			offset_buffer = mempcpy(offset_buffer ,payload_Compress , paqueteDatos.payload_header.payload_compressed_size);
			free(payload_Compress);

			Macro_ImprimirParaDebug("Se termino de serializar los datos del Header y Payload Comprimidos! \n");
*/
			// TODO: Devolver error de que se quisieron enviar datos comprimidos y la funcion no es implementada
		} else {
			paqueteDatos.payload_header.payload_compressed_size = size_payload_Serializado;
			paqueteDatos.payload_header.payload_size = size_payload_Serializado;
			datosDelSocket->datosSerializados = malloc((sizeof(char)*paqueteDatos.payload_header.payload_size) + TAMANIO_HEADER);
			if (datosDelSocket->datosSerializados == NULL) {
				Macro_ImprimirParaDebug("Error en Malloc, posiblemente no hay memoria \n");
				free(payload_Serializado);
				datosDelSocket->soc_err = SE_NOMEM;
				return 0;
			}

			datosDelSocket->sizeDatosSerializados = paqueteDatos.payload_header.payload_size + TAMANIO_HEADER;
			offset_buffer = mempcpy(datosDelSocket->datosSerializados,paqueteDatos.order_header,TAMANIO_ORDER_HEADER);
			offset_buffer = mempcpy(offset_buffer ,&paqueteDatos.payload_header,TAMANIO_PAYLOAD_HEADER);
			offset_buffer = mempcpy(offset_buffer ,payload_Serializado , paqueteDatos.payload_header.payload_size);
			free(payload_Serializado);

			Macro_ImprimirParaDebug("Se termino de serializar los datos del Header y Payload! \n");
		}
	}

	if (datosDelSocket->chunk_size == 0) {
		uiReturn = SOCKET_Enviar_Datos(datosDelSocket, datosDelSocket->datosSerializados, datosDelSocket->sizeDatosSerializados);
		free(datosDelSocket->datosSerializados);
		datosDelSocket->datosSerializados = NULL;
	} else {
		if (datosDelSocket->chunk_size >= datosDelSocket->sizeDatosSerializados) {
			uiReturn = SOCKET_Enviar_Datos(datosDelSocket, datosDelSocket->datosSerializados, datosDelSocket->sizeDatosSerializados);
			free(datosDelSocket->datosSerializados);
			datosDelSocket->datosSerializados = NULL;
			return uiReturn;
		}
		uiReturn = SOCKET_Enviar_Datos(datosDelSocket, datosDelSocket->datosSerializados, datosDelSocket->chunk_size);
		datosDelSocket->chunks_send += datosDelSocket->chunk_size;
	}
	return uiReturn;
}

static uint32_t SOCKET_Enviar_Chunk( t_socket* datosDelSocket) {
	uint32_t uiReturn;

	//Verifico que el parametro no sea null, porque daria un SegFault sino
	if (datosDelSocket == NULL) {
		Macro_ImprimirParaDebug("Error, No se puede Enviar Datos por un Socket que es NULL (fijate que el puntero que pasaste apunta a NULL)\n");
		return 0;
	}

	//Primero chequeo que no sea NULL el puntero al Socket.
	if( datosDelSocket->descriptorSocket <= 0 ){
		Macro_ImprimirParaDebug("Error, No se puede Enviar Datos por un Socket que esta Cerrado\n");
		//Corto la ejecucion porque daria error al enviar datos sino (mas abajo).
		datosDelSocket->soc_err = SE_NE;
		return 0;
	}

	// Verifico que no haya una transferencia pendiente
	if (datosDelSocket->datosSerializados == NULL) {
		Macro_ImprimirParaDebug("No hay datos pendientes de envio");
		datosDelSocket->soc_err = SE_PEND;
		return 0;
	}

	if ((datosDelSocket->chunks_send + datosDelSocket->chunk_size) >= datosDelSocket->sizeDatosSerializados) {
		uiReturn = SOCKET_Enviar_Datos(datosDelSocket, datosDelSocket->datosSerializados + datosDelSocket->chunks_send, (datosDelSocket->sizeDatosSerializados - datosDelSocket->chunks_send));
		free(datosDelSocket->datosSerializados);
		datosDelSocket->datosSerializados = NULL;
		datosDelSocket->chunks_send = 0;
		return uiReturn;
	}
	uiReturn = SOCKET_Enviar_Datos(datosDelSocket, datosDelSocket->datosSerializados + datosDelSocket->chunks_send, datosDelSocket->chunk_size);
	datosDelSocket->chunks_send += datosDelSocket->chunk_size;

	return uiReturn;
}

//Posible Mejora: usar inline
t_socket* SOCKET_Inicializar(t_socket *datosSocket) {
	if (datosSocket == NULL) {
		Macro_ImprimirParaDebug("Puntero NULL, puede haber fallado Malloc");
	}

	memset(datosSocket,0,sizeof(t_socket));
	*datosSocket = (t_socket) T_SOCKET_INIT;

	return datosSocket;
}


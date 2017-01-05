#include "inicializadores.h"

#include <signal.h>
#include <stddef.h>
#include <unistd.h>

pthread_t iniciarThread(void *(*rutina_a_correr)(void *), void* argumento) {
	pthread_t idThread;

	int resultado = 0;

	resultado = pthread_create(&idThread, NULL, rutina_a_correr, argumento);
	if (resultado != 0) {
		//Error al crear el thread, devuelvo -1
		return -1;
	}

	resultado = pthread_detach(idThread);
	if (resultado != 0) {
		//Error al "Desligar" el thread del join, devuelvo -1		(En Realidad esto hace que automaticamente libere sus recursos al terminar el thread, y no lo tengamos que hacer a mano)
		return -1;
	}

	return idThread;
}

pid_t iniciarHijo(void (*rutina_a_correr)(void *), void* datos) {

	pid_t idHijo;

	idHijo = fork();

	if (idHijo < 0) {
		//si es menor a 0, dio error el fork, devuelvo error
		return -1;
	} else if (idHijo != 0) {
		//si es distinto de 0 es el padre, no sigo corriendo la logica, pero si devuelvo mayor a 0
		//para que el padre sepa que se creo algo
		return idHijo;
	}

	//si no es ninguno de esos dos, soy el hijo. Mando a correr la rutina
	rutina_a_correr(datos);

	//Si termine la rutina, siendo el hijo, me aseguro de morir, me auto envio un kill -9
	idHijo = getpid();
	kill(idHijo, SIGKILL);

	//Se supone no llega aca, dado que el kill -9 me auto elimino
	return idHijo;

}

int iniciarHandler(int signum, void (*handler)(int sig)) {

	// Se define la estructura para la señal
	struct sigaction actionSignal;

	// Se indica la función handler que se utilizará al recibir la señal en cuestión
	actionSignal.sa_handler = handler;

	// Se vacia la lista de las señales que quedan bloqueadas durante la ejecución del handler
	if( sigemptyset(&actionSignal.sa_mask) ) {
		return -1;
	}

	// Por el momento se inicializa sin flags
	actionSignal.sa_flags = 0;

	// Se asigna el handler a la señal definida por signum
	if ( sigaction(signum, &actionSignal, NULL) ) {
		return -1;
	}

	return 0;

}

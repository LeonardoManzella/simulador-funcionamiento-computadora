#ifndef INICIALIZADORES_H_
#define INICIALIZADORES_H_

#include <pthread.h>


/*
 * Inicia un thread a partir de la rutina pasada como dato, con el argumento pasado (puede ser NULL)
 * Llama internamente al pthread_detach
 * Devuelve el id del thread creado, o si falla, devuelve -1
 */
pthread_t iniciarThread(void *(*rutina_a_correr)(void *), void* argumento);

/*
 * Inicia un proceso hijo mediante fork y manda a correr la rutina. Una vez respondida, el hijo busca eliminarse
 * Retorna el id del hijo al padre, negativo si falla, y el hijo no llega a ver su valor de retorno
 */
pid_t iniciarHijo(void (*rutina_a_correr)(void *), void* datos);

/*
 * Establece un nuevo handler definido por una función (*handler)
 * a la señal signum. El parametro sig puede no utilizarse en la función handler
 * Devuelve 0 en caso exitoso
 * Devuelve -1 en caso de error
 */
int iniciarHandler(int signum, void (*handler)(int sig));

#endif /* INICIALIZADORES_H_ */

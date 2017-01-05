#ifndef HEADERS_ADMIN_H_
#define HEADERS_ADMIN_H_


#include <pthread.h>
#include "../bibliotecas/sockets.h"
#include "../bibliotecas/structs.h"

//recursos de admin de memoria
void* memoria;					//Espacio de memoria donde estan las direcciones
t_list* lista_de_procesos;		//Lista de las procesos actuales
t_list* listaTLB;				//Lista que apunta a paginas donde esta la TLB
t_bitarray* array_posiciones;	//Bit Array que marca las posiciones libres y ocupadas de la memoria

//mutexes de admin de memoria
pthread_mutex_t* mutex_memoria; //Sincroniza los cambios en la memoria cuando se asignan o liberan posiciones(array y puntero memoria)
pthread_mutex_t* mutex_procesos; 	//Sincroniza la lista de procesos actuales, por si hay pedidos simultaneos
pthread_mutex_t* mutex_TLB; 			//Sincroniza el uso de la tlb

int EsperarCPU();
void initSignals(); 	//Inicia el manejo de senales para las interrupciones
void handle_signals(int signal); //Manejo de senales recibidas
void initMutexes();		//Inicia los mutexes a ser usados para sincronizar los recursos
void orden_conectar_CPU(t_socket* socket_cliente);
void orden_iniciar_proceso(t_socket* socket_cliente);
void orden_CPU_leer_datos(t_socket* socket_cliente);
void orden_CPU_Escribir_Datos(t_socket* socket_cliente);
void orden_finalizar_proceso(t_socket* socket_cliente);
int reservarMarcoLibre();
int traer_o_apuntar_frame_local(t_pagina* frameATraer, t_pagina** frameLocalADevolver);
void agregar_a_TLB(t_pagina* pagina_a_agregar);
void borrar_de_TLB(t_pagina* pagina_a_borrar);
void imprimir_tlb();

#endif /* HEADERS_ADMIN_H_ */

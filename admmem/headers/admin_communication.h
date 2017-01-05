#ifndef ADMMEM_SRC_ADMIN_COMMUNICATION_H_
#define ADMMEM_SRC_ADMIN_COMMUNICATION_H_

#include "../../bibliotecas/structs.h"

int conectar_swap();

int iniciar_proceso_en_swap(t_proceso_memoria* procesoNuevo);

int finalizar_proceso_en_swap(t_proceso_memoria* proceso);

int leer_pagina_de_swap(t_pagina* framePedido, char** paginaARetornar);

int escribir_pagina_de_swap(t_pagina* frameEnviado, char* contenidoFrame);

#endif /* ADMMEM_SRC_ADMIN_COMMUNICATION_H_ */

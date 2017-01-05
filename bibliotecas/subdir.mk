C_SRCS += \
bibliotecas/inicializadores.c \
bibliotecas/logs.c \
bibliotecas/sockets.c \
bibliotecas/serializacion.c \
bibliotecas/comun.c

OBJS += \
obj/bibliotecas/inicializadores.o \
obj/bibliotecas/logs.o \
obj/bibliotecas/sockets.o \
obj/bibliotecas/serializacion.o \
obj/bibliotecas/comun.o

C_DEPS += \
obj/bibliotecas/inicializadores.d \
obj/bibliotecas/logs.d \
obj/bibliotecas/sockets.d \
obj/bibliotecas/serializacion.d \
obj/bibliotecas/comun.d

obj/bibliotecas/%.o: bibliotecas/%.c
	@mkdir -p obj/bibliotecas
	@printf "$(MSGOBJ) $< \r"
	@if gcc $(FLAGS) -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<" $(OUTPUT) ; then \
	printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
	else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi



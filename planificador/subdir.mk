C_SRCS_PLANIF += \
planificador/planificador.c

OBJS_PLANIF += \
obj/planificador/planificador.o

C_DEPS_PLANIF += \
obj/planificador/planificador.d

obj/planificador/%.o: planificador/%.c
	@mkdir -p obj/planificador
	@printf "$(MSGOBJ) $< \r"
	@if gcc $(FLAGS) -I../lib/readline -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<" $(OUTPUT) ; then \
	printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
	else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi


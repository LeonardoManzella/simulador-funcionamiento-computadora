C_SRCS_CPU += \
cpu/cpu.c \
cpu/cpu_utiles.c

OBJS_CPU += \
obj/cpu/cpu.o \
obj/cpu/cpu_utiles.o

C_DEPS_CPU += \
obj/cpu/cpu.d \
obj/cpu/cpu_utiles.d \

obj/cpu/%.o: cpu/%.c
	@mkdir -p obj/cpu
	@printf "$(MSGOBJ) $< \r"
	@if gcc $(FLAGS) -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<" $(OUTPUT) ; then \
	printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
	else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi



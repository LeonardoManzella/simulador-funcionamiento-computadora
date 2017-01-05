C_SRCS_ADMMEM += \
admmem/src/admin_configuration.c \
admmem/src/admin_communication.c \
admmem/src/admin_helper.c

OBJS_ADMMEM += \
obj/admmem/src/admin_configuration.o \
obj/admmem/src/admin_communication.o \
obj/admmem/src/admin_helper.o

C_DEPS_ADMMEM += \
obj/admmem/src/admin_configuration.d \
obj/admmem/src/admin_communication.d \
obj/admmem/src/admin_helper.d

obj/admmem/src/%.o: admmem/src/%.c
	@mkdir -p obj/admmem/src
	@printf "$(MSGOBJ) $< \r"
	@if gcc $(FLAGS) -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<" $(OUTPUT) ; then \
	printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
	else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi



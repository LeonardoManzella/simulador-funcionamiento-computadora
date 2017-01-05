C_SRCS_ADMMEM += \
admmem/admin.c 

OBJS_ADMMEM += \
obj/admmem/admin.o 

C_DEPS_ADMMEM += \
obj/admmem/admin.d 

obj/admmem/%.o: admmem/%.c
	@mkdir -p obj/admmem
	@printf "$(MSGOBJ) $< \r"
	@if gcc $(FLAGS) -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<" $(OUTPUT) ; then \
	printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
	else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi



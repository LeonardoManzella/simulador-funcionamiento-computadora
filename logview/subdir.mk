C_SRCS_LOGVIEW += \
logview/logview.c

OBJS_LOGVIEW += \
obj/logview/logview.o

C_DEPS_LOGVIEW += \
obj/logview/logview.d

obj/logview/%.o: logview/%.c
	@mkdir -p obj/logview
	@printf "$(MSGOBJ) $< \r"
	@if gcc $(FLAGS) -DHAVE_CONFIG_H -Ilib/cdk/include -lncurses -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<" $(OUTPUT) ; then \
	printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
	else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi


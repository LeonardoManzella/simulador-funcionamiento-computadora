C_SRCS_SWAP += \
swap/swap.c \
swap/swap_log.c 

OBJS_SWAP += \
obj/swap/swap.o \
obj/swap/swap_log.o 

C_DEPS_SWAP += \
obj/swap/swap.d \
obj/swap/swap_log.d 

obj/swap/%.o: swap/%.c
	@mkdir -p obj/swap
	@printf "$(MSGOBJ) $< \r"
	@if gcc $(FLAGS) -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<" $(OUTPUT) ; then \
	printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
	else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi



C_SRCS_LIBCOMMON += \
lib/commons/collections/dictionary.c \
lib/commons/collections/list.c \
lib/commons/collections/queue.c

OBJS_LIBCOMMON += \
obj/commons/collections/dictionary.o \
obj/commons/collections/list.o \
obj/commons/collections/queue.o

C_DEPS_LIBCOMMON += \
obj/commons/collections/dictionary.d \
obj/commons/collections/list.d \
obj/commons/collections/queue.d \

# agregar flag -fPIC para shared lib

obj/commons/collections/%.o: lib/commons/collections/%.c
	@mkdir -p obj/commons/collections
	@printf "$(MSGOBJ) $< \r"
	@if gcc $(FLAGS) -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<" $(OUTPUT) ; then \
	printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
	else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi



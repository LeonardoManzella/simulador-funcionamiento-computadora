C_SRCS_LIBCOMMON += \
lib/commons/bitarray.c \
lib/commons/config.c \
lib/commons/error.c \
lib/commons/log.c \
lib/commons/process.c \
lib/commons/string.c \
lib/commons/temporal.c \
lib/commons/txt.c

OBJS_LIBCOMMON += \
obj/commons/bitarray.o \
obj/commons/config.o \
obj/commons/error.o \
obj/commons/log.o \
obj/commons/process.o \
obj/commons/string.o \
obj/commons/temporal.o \
obj/commons/txt.o

C_DEPS_LIBCOMMON += \
obj/commons/bitarray.d \
obj/commons/config.d \
obj/commons/error.d \
obj/commons/log.d \
obj/commons/process.d \
obj/commons/string.d \
obj/commons/temporal.d \
obj/commons/txt.d

# agregar flag -fPIC para shared lib

obj/commons/%.o: lib/commons/%.c
	@mkdir -p obj/commons
	@printf "$(MSGOBJ) $< \r"
	@if gcc $(FLAGS) -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<" $(OUTPUT) ; then \
	printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
	else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi



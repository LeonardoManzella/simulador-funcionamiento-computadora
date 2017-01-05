OBJS_HIST += \
obj/readline/history.o \
obj/readline/histexpand.o \
obj/readline/histfile.o \
obj/readline/histsearch.o \
obj/readline/shell.o \
obj/readline/mbutil.o

OBJS_TILDE += \
obj/readline/tilde.o

OBJS_COLORS += \
obj/readline/colors.o \
obj/readline/parse-colors.o

OBJS_READLINE += \
obj/readline/readline.o \
obj/readline/vi_mode.o \
obj/readline/funmap.o \
obj/readline/keymaps.o \
obj/readline/parens.o \
obj/readline/search.o \
obj/readline/rltty.o \
obj/readline/complete.o \
obj/readline/bind.o \
obj/readline/isearch.o \
obj/readline/display.o \
obj/readline/signals.o \
obj/readline/util.o \
obj/readline/kill.o \
obj/readline/undo.o \
obj/readline/macro.o \
obj/readline/input.o \
obj/readline/callback.o \
obj/readline/terminal.o \
obj/readline/text.o \
obj/readline/nls.o \
obj/readline/misc.o \
obj/readline/compat.o $(OBJS_HIST) $(OBJS_TILDE) $(OBJS_COLORS) $(OBJS_X)

C_SRCS_X += \
lib/readline/xmalloc.c \
lib/readline/xfree.c

OBJS_X += \
obj/readline/xmalloc.o \
obj/readline/xfree.o

C_DEPS_X += \
obj/readline/xmalloc.d \
obj/readline/xfree.d 

# agregar flag -fPIC para shared lib

obj/readline/%.o : lib/readline/%.c 
	@mkdir -p obj/readline
	@printf "$(MSGOBJ) $< \r"
	@if gcc $(FLAGS) -DHAVE_CONFIG_H -Ilib/readline -ltermcap -lncurses -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<" $(OUTPUT) ; then \
	printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
	else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi

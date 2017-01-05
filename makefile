NARANJA := \033[0;35m
ROJO := \033[0;31m
SINCOLOR := \033[0m
VERDE := \033[0;32m
COLUMNA :=  \033[60C
COMPILADO = "Compilado"
ERROR = "Error!"
RM := rm -rf
DEBUG := -O0 -g3 -D_LARGEFILE_SOURCE=1 -D_LARGEFILE64_SOURCE=1  -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE #-w -D_GNU_SOURCES ignora warnings 
BUILD := -s -DNODEBUG -O3 -D_LARGEFILE_SOURCE=1 -D_LARGEFILE64_SOURCE=1 -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE
FLAGS := $(DEBUG)
CURRENT_DIR := $(shell pwd)
#LIBSGCC =  -Llib -Wl,-rpath=$(CURRENT_DIR)/lib
LIBSGCC = -Llib
#LIBS_SHR := -lpthread -lcommons 
LIBS_SHR := bin/libcommons.a bin/libgenesys.a bin/libreadline.a bin/libhistory.a bin/libcdk.a -lpthread -lm -lrt -ltermcap -lncurses
OBJS_SWAP :=
C_SRCS_SWAP :=
C_DEPS_SWAP :=
OBJS :=
C_SRCS :=
C_DEPS :=
MSGOBJ :=  #Compilando: 
MSGEXE := 
OUTPUT := 2>> builderr.log
TARGET := _commons _readline _cdk _genesys _logview _planificador _cpu _swap _admmem 
SUBDIRS := \
swap \
admmem \
cpu \
planificador \

-include bibliotecas/subdir.mk
-include swap/subdir.mk
-include planificador/subdir.mk
-include admmem/src/subdir.mk
-include admmem/subdir.mk
-include cpu/subdir.mk
-include lib/commons/collections/subdir.mk
-include lib/commons/subdir.mk
-include lib/readline/subdir.mk
-include lib/cdk/subdir.mk
-include logview/subdir.mk

ifneq (,$(findstring r, $(MAKEFLAGS)))
	FLAGS := $(BUILD)
endif

menuconfig:
	@./deploy/install.sh

help: 
	@printf "make help\n La version mas facil es make menuconfig"

all: $(TARGET)
	@printf "Compilacion Completa!\nPara ver Warnings y Errores vea el archivo builderr.log\n"


# Tool invocations

_planificador: destdirobj destdirbin _commons _genesys _readline $(OBJS_PLANIF)
	@mkdir -p bin/planificador
	@printf "$(NARANJA)$(MSGEXE) bin/planificador/$@ $(SINCOLOR) \r"
	@if gcc $(LIBSGCC) -DHAVE_CONFIG_H -Ilib/readline -o "bin/planificador/planificador" $(OBJS) $(OBJS_PLANIF) $(LIBS_SHR) $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi
	@cp planificador/ConfigPlanificador.cfg bin/planificador/ConfigPlanificador.cfg

_cpu: destdirobj destdirbin _commons _genesys $(OBJS_CPU)
	@mkdir -p bin/cpu
	@printf "$(NARANJA)$(MSGEXE) bin/cpu/$@ $(SINCOLOR) \r"
	@if gcc $(LIBSGCC) -o "bin/cpu/cpu" $(OBJS) $(OBJS_CPU) $(LIBS_SHR) $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi
	@cp cpu/cpu.cfg bin/cpu/cpu.cfg

_swap: destdirobj destdirbin _commons _genesys $(OBJS_SWAP)
	@mkdir -p bin/swap
	@printf "$(NARANJA)$(MSGEXE) bin/swap/$@ $(SINCOLOR) \r"
	@if gcc $(LIBSGCC) -o "bin/swap/swap" $(OBJS) $(OBJS_SWAP) $(LIBS_SHR) $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi
	@cp swap/swap_config.cfg bin/swap/swap_config.cfg

_admmem: destdirobj destdirbin _commons _genesys $(OBJS_ADMMEM)
	@mkdir -p bin/admmem
	@printf "$(NARANJA)$(MSGEXE) bin/admmem/$@ $(SINCOLOR) \r"
	@if gcc $(LIBSGCC) -o "bin/admmem/admmem" $(OBJS) $(OBJS_ADMMEM) $(LIBS_SHR) $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi
	@cp admmem/config_admin.cfg bin/admmem/config_admin.cfg

_commons: destdirobj destdirbin $(OBJS_LIBCOMMON)
	@mkdir -p bin/
	@printf "$(NARANJA)$(MSGEXE) bin/$@ $(SINCOLOR) \r"
	@if ar rcs "bin/libcommons.a" $(OBJS_LIBCOMMON) $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi

_readline.a: destdirobj destdirbin $(OBJS_READLINE)
	@mkdir -p bin/
	@printf "$(NARANJA)$(MSGEXE) bin/$@ $(SINCOLOR) \r"
	@if ar rcs "bin/libreadline.a" $(OBJS_READLINE) $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi

_history.a: destdirobj destdirbin $(OBJS_HIST) $(OBJS_X)
	@mkdir -p bin/
	@printf "$(NARANJA)$(MSGEXE) bin/$@ $(SINCOLOR) \r"
	@if ar rcs "bin/libhistory.a" $(OBJS_HIST) $(OBJS_X) $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi

_readline: _readline.a _history.a

_genesys: destdirobj destdirbin _commons $(OBJS)
	@mkdir -p bin/
	@printf "$(NARANJA)$(MSGEXE) bin/$@ $(SINCOLOR) \r"
	@if ar rcs "bin/libgenesys.a" $(OBJS) $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi

_cdk: destdirobj destdirbin $(OBJS_CDK)
	@mkdir -p bin/
	@printf "$(NARANJA)$(MSGEXE) bin/$@ $(SINCOLOR) \r"
	@if ar rcs "bin/libcdk.a" $(OBJS_CDK) $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi

_logview: destdirobj destdirbin _cdk $(OBJS_LOGVIEW)
	@mkdir -p bin/
	@printf "$(NARANJA)$(MSGEXE) bin/$@ $(SINCOLOR) \r"
	@if gcc $(LIBSGCC) -DHAVE_CONFIG_H -Ilib/cdk/include -o "bin/logview" $(OBJS_LOGVIEW) $(LIBS_SHR) $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi

_curses:
	@sudo dpkg -i lib/libncurses5-dev.deb

clean:
	-$(RM) $(OBJS) $(OBJS_PLANIF) $(OBJS_CPU) $(OBJS_ADMMEM) $(OBJS_SWAP) 
	-$(RM) $(C_DEPS) $(C_DEPS_PLANIF) $(C_DEPS_ADMMEM) $(C_DEPS_CPU) $(C_DEPS_SWAP) 
	-$(RM) bin/planificador/planificador bin/cpu/cpu bin/admmem/admmem bin/swap/swap bin/libgenesys.a
	-$(RM) builderr.log
	-$(RM) bin/planificador/ConfigPlanificador.cfg bin/cpu/cpu.cfg bin/swap/swap_config.cfg bin/admmem/config_admin.cfg
	#-$(RM) bin/swap/swap.data bin/planificador/*.html bin/cpu/*.html bin/admmem/*.html bin/swap/*.html

cleanall:
	@rm -rf bin
	@rm -rf obj
	#-$(RM) $(OBJS) $(OBJS_PLANIF) $(OBJS_CPU) $(OBJS_ADMMEM) $(OBJS_SWAP) $(OBJS_LIBCOMMON)
	#-$(RM) $(C_DEPS) $(C_DEPS_PLANIF) $(C_DEPS_ADMMEM) $(C_DEPS_CPU) $(C_DEPS_SWAP) $(C_DEPS_LIBCOMMON) 
	#-$(RM) bin/planificador/planificador bin/cpu/cpu bin/admmem/admmem bin/swap/swap bin/libcommons.a bin/libgenesys.a
	#-$(RM) builderr.log
	#-$(RM) bin/planificador/ConfigPlanificador.cfg bin/cpu/cpu.cfg bin/swap/swap_config.cfg bin/admmem/config_admin.cfg
	#-$(RM) bin/swap/swap.data bin/planificador/*.html bin/cpu/*.html bin/admmem/*.html bin/swap/*.html

destdirobj:
	@mkdir -p obj/$(SUBDIRS)

destdirbin:
	@mkdir -p bin

.PHONY: all clean
.SECONDARY:

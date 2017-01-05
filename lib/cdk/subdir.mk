OBJS_CDK += \
obj/cdk/alphalist.o \
obj/cdk/binding.o \
obj/cdk/buttonbox.o \
obj/cdk/button.o \
obj/cdk/calendar.o \
obj/cdk/cdk.o \
obj/cdk/cdk_compat.o \
obj/cdk/cdk_display.o \
obj/cdk/cdk_objs.o \
obj/cdk/cdk_params.o \
obj/cdk/cdkscreen.o \
obj/cdk/debug.o \
obj/cdk/dialog.o \
obj/cdk/draw.o \
obj/cdk/dscale.o \
obj/cdk/entry.o \
obj/cdk/fscale.o \
obj/cdk/fselect.o \
obj/cdk/fslider.o \
obj/cdk/get_index.o \
obj/cdk/get_string.o \
obj/cdk/graph.o \
obj/cdk/histogram.o \
obj/cdk/itemlist.o \
obj/cdk/label.o \
obj/cdk/marquee.o \
obj/cdk/matrix.o \
obj/cdk/mentry.o \
obj/cdk/menu.o \
obj/cdk/popup_dialog.o \
obj/cdk/popup_label.o \
obj/cdk/position.o \
obj/cdk/radio.o \
obj/cdk/scale.o \
obj/cdk/scroll.o \
obj/cdk/scroller.o \
obj/cdk/select_file.o \
obj/cdk/selection.o \
obj/cdk/slider.o \
obj/cdk/swindow.o \
obj/cdk/template.o \
obj/cdk/traverse.o \
obj/cdk/uscale.o \
obj/cdk/uslider.o \
obj/cdk/version.o \
obj/cdk/viewer.o \
obj/cdk/view_file.o \
obj/cdk/view_info.o

# agregar flag -fPIC para shared lib

obj/cdk/%.o : lib/cdk/%.c 
	@mkdir -p obj/cdk
	@printf "$(MSGOBJ) $< \r"
	@if gcc $(FLAGS) -DHAVE_CONFIG_H -Ilib/cdk/include -lncurses -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<" $(OUTPUT) ; then \
	printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
	else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi

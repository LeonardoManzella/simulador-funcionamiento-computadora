#!/bin/bash

DIALOG_CANCEL=1
DIALOG_ESC=255

#function display_output() 
#dialog --separate-widget $'\n' \
planif_ip=
planif_puerto=
planif_algoritmo=
planif_quantum=
mem_ip=
mem_puerto=
mem_max_frame_proc=
mem_frame_n=
frame_s=
mem_tlb_s=
mem_tlb=
mem_retardo=
mem_algoritmo=
cpu_puerto=
cpu_hilos=
cpu_retardo=
swap_ip=
swap_puerto=
swap_file=
swap_frame_n=
swap_compac=
swap_retardo=
swap_config_file="swap/swap_config.cfg"
planif_config_file="planificador/ConfigPlanificador.cfg"
mem_config_file="admmem/config_admin.cfg"
cpu_config_file="cpu/cpu.cfg"
modo=DEBUG

function init_config() {
	planif_puerto="$(sed -n -e 's/^\s*PUERTO_ESCUCHA\s*=\s*//p' $planif_config_file)"
	planif_algoritmo="$(sed -n -e 's/^\s*ALGORITMO_PLANIFICACION\s*=\s*//p' $planif_config_file)"
	planif_quantum="$(sed -n -e 's/^\s*QUANTUM\s*=\s*//p' $planif_config_file)"
	mem_puerto="$(sed -n -e 's/^\s*PUERTO_ESCUCHA\s*=\s*//p' $mem_config_file)"
	mem_max_frame_proc="$(sed -n -e 's/^\s*MAXIMO_MARCOS_POR_PROCESO\s*=\s*//p' $mem_config_file)"
	mem_frame_n="$(sed -n -e 's/^\s*CANTIDAD_MARCOS\s*=\s*//p' $mem_config_file)"
	frame_s="$(sed -n -e 's/^\s*TAMANIO_MARCO\s*=\s*//p' $mem_config_file)"
	mem_tlb_s="$(sed -n -e 's/^\s*ENTRADAS_TLB\s*=\s*//p' $mem_config_file)"
	mem_tlb="$(sed -n -e 's/^\s*TLB_HABILITADA\s*=\s*//p' $mem_config_file)"
	mem_retardo="$(sed -n -e 's/^\s*RETARDO_MEMORIA\s*=\s*//p' $mem_config_file)"
	mem_algoritmo="$(sed -n -e 's/^\s*ALGORITMO_REEMPLAZO\s*=\s*//p' $mem_config_file)"
	swap_ip="$(sed -n -e 's/^\s*IP_SWAP\s*=\s*//p' $mem_config_file)"
	cpu_puerto="$(sed -n -e 's/^\s*PUERTO_ESCUCHA_CPU\s*=\s*//p' $cpu_config_file)"
	cpu_hilos="$(sed -n -e 's/^\s*CANTIDAD_HILOS\s*=\s*//p' $cpu_config_file)"
	cpu_retardo="$(sed -n -e 's/^\s*RETARDO\s*=\s*//p' $cpu_config_file)"
	planif_ip="$(sed -n -e 's/^\s*IP_PLANIFICADOR\s*=\s*//p' $cpu_config_file)"
	mem_ip="$(sed -n -e 's/^\s*IP_MEMORIA\s*=\s*//p' $cpu_config_file)"
	swap_puerto="$(sed -n -e 's/^\s*PUERTO_ESCUCHA\s*=\s*//p' $swap_config_file)"
	swap_file="$(sed -n -e 's/^\s*NOMBRE_SWAP\s*=\s*//p' $swap_config_file)"
	swap_frame_n="$(sed -n -e 's/^\s*CANTIDAD_PAGINAS\s*=\s*//p' $swap_config_file)"
	swap_compact="$(sed -n -e 's/^\s*RETARDO_COMPACTACION\s*=\s*//p' $swap_config_file)"
	swap_retardo="$(sed -n -e 's/^\s*RETARDO_SWAP\s*=\s*//p' $swap_config_file)"
}

function save_config {
	sed -i "/^PUERTO_ESCUCHA_CPU=/s/=.*/=$cpu_puerto/" $cpu_config_file
	sed -i "/^IP_PLANIFICADOR=/s/=.*/=$planif_ip/" $cpu_config_file
	sed -i "/^PUERTO_PLANIFICADOR=/s/=.*/=$planif_puerto/" $cpu_config_file
	sed -i "/^IP_MEMORIA=/s/=.*/=$mem_ip/" $cpu_config_file
	sed -i "/^PUERTO_MEMORIA=/s/=.*/=$mem_puerto/" $cpu_config_file
	sed -i "/^CANTIDAD_HILOS=/s/=.*/=$cpu_hilos/" $cpu_config_file
	sed -i "/^RETARDO=/s/=.*/=$cpu_retardo/" $cpu_config_file
	sed -i "/^PUERTO_ESCUCHA=/s/=.*/=$planif_puerto/" $planif_config_file
	sed -i "/^ALGORITMO_PLANIFICACION=/s/=.*/=$planif_algoritmo/" $planif_config_file
	sed -i "/^QUANTUM=/s/=.*/=$planif_quantum/" $planif_config_file
	sed -i "/^PUERTO_ESCUCHA=/s/=.*/=$mem_puerto/" $mem_config_file
	sed -i "/^IP_SWAP=/s/=.*/=$swap_ip/" $mem_config_file
	sed -i "/^PUERTO_SWAP=/s/=.*/=$swap_puerto/" $mem_config_file
	sed -i "/^MAXIMO_MARCOS_POR_PROCESO=/s/=.*/=$mem_max_frame_proc/" $mem_config_file
	sed -i "/^CANTIDAD_MARCOS=/s/=.*/=$mem_frame_n/" $mem_config_file
	sed -i "/^TAMANIO_MARCO=/s/=.*/=$frame_s/" $mem_config_file
	sed -i "/^ENTRADAS_TLB=/s/=.*/=$mem_tlb_s/" $mem_config_file
	sed -i "/^TLB_HABILITADA=/s/=.*/=$mem_tlb/" $mem_config_file
	sed -i "/^RETARDO_MEMORIA=/s/=.*/=$mem_retardo/" $mem_config_file
	sed -i "/^ALGORITMO_REEMPLAZO=/s/=.*/=$mem_algoritmo/" $mem_config_file
	sed -i "/^PUERTO_ESCUCHA=/s/=.*/=$swap_puerto/" $swap_config_file
	sed -i "/^NOMBRE_SWAP=/s/=.*/=$swap_file/" $swap_config_file
	sed -i "/^CANTIDAD_PAGINAS=/s/=.*/=$swap_frame_n/" $swap_config_file
	sed -i "/^TAMANIO_PAGINA=/s/=.*/=$frame_s/" $swap_config_file
	sed -i "/^RETARDO_COMPACTACION=/s/=.*/=$swap_compact/" $swap_config_file
	sed -i "/^RETARDO_SWAP=/s/=.*/=$swap_retardo/" $swap_config_file
}

values=

function config_Planif() {
	exec 3>&1
	values=$(./deploy/dialog --clear --no-shadow --backtitle "TP S.O." \
		--title "Configuracion Planificador" \
		--form "Parametros de Configuracion" 20 60 11 \
			"IP: " 1 2 "$planif_ip" 1 12 16 15 \
			"Puerto: " 2 2 "$planif_puerto" 2 12 6 5 \
			"Algoritmo: " 3 2 "$planif_algoritmo" 3 12 5 4 \
			"Quantum: " 4 2 "$planif_quantum" 4 12 3 2 \
			2>&1 1>&3) 
	exit_status=$?
	exec 3>&-
	case $exit_status in
		$DIALOG_CANCEL)
			return
			;;
		$DIALOG_ESC)
			return
			;;
		*)
			readarray -t resultado <<< "$values"
			planif_ip=${resultado[0]}
			planif_puerto=${resultado[1]}
			planif_algoritmo=${resultado[2]}
			planif_quantum=${resultado[3]}
			save_config
			return
			;;
	esac
}

function config_CPU() {
	exec 3>&1
	values=$(./deploy/dialog --clear --no-shadow --backtitle "TP S.O." \
		--title "Configuracion CPU" \
		--form "Parametros de Configuracion" 20 60 11 \
			"Puerto: " 1 2 "$cpu_puerto" 1 12 6 5 \
			"Cant. Hilos: " 2 2 "$cpu_hilos" 2 12 3 2 \
			"Retardo: " 3 2 "$cpu_retardo" 3 12 10 9 \
			2>&1 1>&3) 
	exit_status=$?
	exec 3>&-
	case $exit_status in
		$DIALOG_CANCEL)
			return
			;;
		$DIALOG_ESC)
			return
			;;
		*)
			readarray -t resultado <<< "$values"
			cpu_puerto=${resultado[0]}
			cpu_hilos=${resultado[1]}
			cpu_retardo=${resultado[2]}
			save_config
			return
			;;
	esac
}

function config_Memoria() {
	exec 3>&1
	values=$(./deploy/dialog --clear --no-shadow --backtitle "TP S.O." \
		--title "Configuracion Memoria" \
		--form "Parametros de Configuracion" 20 60 11 \
			"IP: " 1 2 "$mem_ip" 1 15 16 15 \
			"Puerto: " 2 2 "$mem_puerto" 2 15 6 5 \
			"Max Frame Proc: " 3 2 "$mem_max_frame_proc" 3 15 3 2 \
			"Frame Num: " 4 2 "$mem_frame_n" 4 15 4 3 \
			"Frame Size: " 5 2 "$frame_s" 5 15 4 3 \
			"TLB Number: " 6 2 "$mem_tlb_s" 6 15 3 2 \
			"TLB: " 7 2 "$mem_tlb" 7 15 6 5 \
			"Retardo: " 8 2 "$mem_retardo" 8 15 10 9 \
			"Algoritmo: " 9 2 "$mem_algoritmo" 9 15 8 7 \
			2>&1 1>&3) 
	exit_status=$?
	exec 3>&-
	case $exit_status in
		$DIALOG_CANCEL)
			return
			;;
		$DIALOG_ESC)
			return
			;;
		*)
			readarray -t resultado <<< "$values"
			mem_ip=${resultado[0]}
			mem_puerto=${resultado[1]}
			mem_max_frame_proc=${resultado[2]}
			mem_frame_n=${resultado[3]}
			frame_s=${resultado[4]}
			mem_tlb_s=${resultado[5]}
			mem_tlb=${resultado[6]}
			mem_retardo=${resultado[7]}
			mem_algoritmo=${resultado[8]}
			save_config
			return
			;;
	esac
}

function config_SWAP() {
	exec 3>&1
	values=$(./deploy/dialog --clear --no-shadow --backtitle "TP S.O." \
		--title "Configuracion SWAP" \
		--form "Parametros de Configuracion" 20 60 11 \
			"IP: " 1 2 "$swap_ip" 1 15 16 15 \
			"Puerto: " 2 2 "$swap_puerto" 2 15 6 5 \
			"Archivo: " 3 2 "$swap_file" 3 15 24 25 \
			"Frame Num: " 4 2 "$swap_frame_n" 4 15 4 3 \
			"Compact Delay: " 5 2 "$swap_compact" 5 15 11 10 \
			"Retardo: " 6 2 "$swap_retardo" 6 15 10 9 \
			2>&1 1>&3) 
	exit_status=$?
	exec 3>&-
	case $exit_status in
		$DIALOG_CANCEL)
			return
			;;
		$DIALOG_ESC)
			return
			;;
		*)
			readarray -t resultado <<< "$values"
			swap_ip=${resultado[0]}
			swap_puerto=${resultado[1]}
			swap_file=${resultado[2]}
			swap_frame_n=${resultado[3]}
			swap_compact=${resultado[4]}
			swap_retardo=${resultado[5]}
			save_config
			return
			;;
	esac
}

function Compilar() {
while true
do

exec 3>&1
selection=$(./deploy/dialog --no-shadow  --backtitle  "TP S.O." \
	--title "Opciones de Compilacion" \
	--cancel-label "Salir"  \
	--menu "Por favor selecciona una opcion" 20 60 11 \
		"Todo" "Compilar Todos los Modulos" \
		"Clean" "Limpia la compilacion" \
		"GenesysLib" "Compilar biblioteca Genesys" \
		"LogView" "Compilar para ver Logs" \
		"Planif" "Compilar el Planificador" \
		"CPU" "Compilar la CPU" \
		"Memoria" "Compilar el Administrador Memoria" \
		"SWAP" "Compilar SWAP" \
		"Errores" "Ver Log de Errores de Compilacion" \
		"Modo" "Cambiar Modo de Compilacion ($modo)" \
		2>&1 1>&3)
exit_status=$?
exec 3>&-
case $exit_status in
	$DIALOG_CANCEL)
		return
		;;
	$DIALOG_ESC)
		return
		;;
esac
if [ "$modo" == "RELEASE" ]; then
	compilador="make -r "
else
	compilador="make "
fi
case $selection in
	0)
		return
		;;
	Planif )
		clear
		$compilador _planificador
		read -N 1 -t 10 -p "Presione una tecla para continuar o espere 10 segundos"
		;;
	CPU )
		clear
		$compilador _cpu
		read -N 1 -t 10 -p "Presione una tecla para continuar o espere 10 segundos"
		;;
	Memoria )
		clear
		$compilador _admmem
		read -N 1 -t 10 -p "Presione una tecla para continuar o espere 10 segundos"
		;;
	SWAP )
		clear
		$compilador _swap
		read -N 1 -t 10 -p "Presione una tecla para continuar o espere 10 segundos"
		;;
	LogView )
		clear
		$compilador _logview
		read -N 1 -t 10 -p "Presione una tecla para continuar o espere 10 segundos"
		;;
	GenesysLib )
		clear
		$compilador _genesys
		read -N 1 -t 10 -p "Presione una tecla para continuar o espere 10 segundos"
		;;
	Todo )
		clear
		$compilador all
		read -N 1 -t 10 -p "Presione una tecla para continuar o espere 10 segundos"
		;;
	Clean )
		clear
		make clean
		read -N 1 -t 10 -p "Presione una tecla para continuar o espere 10 segundos"
		;;
	Errores )
		./deploy/dialog --extra-button --extra-label "Eliminar" --textbox builderr.log 20 80
		exit_status=$?
		if [ $exit_status -eq 3 ]; then
			rm builderr.log
		fi
		;;
	Modo)
		if [ "$modo" == "DEBUG" ]; then
			modo=RELEASE
		else
			modo=DEBUG
		fi
		;;
esac
done

}

function enviar_config() {
	exec 3>&1
	password=$(./deploy/dialog --clear --backtitle "TP S.O." --title "Deploy" --passwordbox "Ingrese la clave para los equipos remotos" 20 60 2>&1 1>&3)
	exec 3>&-
	clear
	printf "Copiando Config CPU\n"
	./deploy/sshpass -p $password ssh -o StrictHostKeyChecking=no $planif_ip "pkill -9 -u utnso cpu"
	./deploy/sshpass -p $password scp -o StrictHostKeyChecking=no -pC cpu/cpu.cfg $planif_ip:/home/utnso/cache13/cpu.cfg
	
	printf "Copiando config Planificador\n"
	./deploy/sshpass -p $password ssh -o StrictHostKeyChecking=no $planif_ip "pkill -9 -u utnso planificador"
	./deploy/sshpass -p $password scp -o StrictHostKeyChecking=no -pC planificador/ConfigPlanificador.cfg $planif_ip:/home/utnso/cache13/ConfigPlanificador.cfg
	
	printf "Copiando config SWAP\n"
	./deploy/sshpass -p $password ssh -o StrictHostKeyChecking=no $swap_ip "pkill -9 -u utnso swap"
	./deploy/sshpass -p $password scp -o StrictHostKeyChecking=no -pC swap/swap_config.cfg $swap_ip:/home/utnso/cache13/swap_config.cfg
	
	printf "Copiando config Administrador Memoria\n"
	./deploy/sshpass -p $password ssh -o StrictHostKeyChecking=no $mem_ip "pkill -9 -u utnso admmem"
	./deploy/sshpass -p $password scp -o StrictHostKeyChecking=no -pC admmem/config_admin.cfg $mem_ip:/home/utnso/cache13/config_admin.cfg
	
	read -N 1 -t 10 -p "Presione una tecla para continuar o espere 10 segundos"
}

function enviar_sistema() {
	exec 3>&1
	password=$(./deploy/dialog --clear --backtitle "TP S.O." --title "Deploy" --passwordbox "Ingrese la clave para los equipos remotos" 20 60 2>&1 1>&3)
	exec 3>&-
	clear
	printf "Copiando archivos CPU\n"
	./deploy/sshpass -p $password ssh -o StrictHostKeyChecking=no $planif_ip "pkill -9 -u utnso cpu"
	./deploy/sshpass -p $password ssh -o StrictHostKeyChecking=no $planif_ip "mkdir -p /home/utnso/cache13"
	./deploy/sshpass -p $password scp -o StrictHostKeyChecking=no -pC bin/cpu/cpu $planif_ip:/home/utnso/cache13/cpu
	./deploy/sshpass -p $password scp -o StrictHostKeyChecking=no -pC bin/cpu/cpu.cfg $planif_ip:/home/utnso/cache13/cpu.cfg
	
	printf "Copiando archivos Planificador\n"
	./deploy/sshpass -p $password ssh -o StrictHostKeyChecking=no $planif_ip "pkill -9 -u utnso planificador"
	./deploy/sshpass -p $password ssh -o StrictHostKeyChecking=no $planif_ip "mkdir -p /home/utnso/cache13"
	./deploy/sshpass -p $password scp -o StrictHostKeyChecking=no -pC bin/planificador/planificador $planif_ip:/home/utnso/cache13/planificador
	./deploy/sshpass -p $password scp -o StrictHostKeyChecking=no -pC bin/planificador/ConfigPlanificador.cfg $planif_ip:/home/utnso/cache13/ConfigPlanificador.cfg
	
	printf "Copiando archivos SWAP\n"
	./deploy/sshpass -p $password ssh -o StrictHostKeyChecking=no $swap_ip "pkill -9 -u utnso swap"
	./deploy/sshpass -p $password ssh -o StrictHostKeyChecking=no $swap_ip "mkdir -p /home/utnso/cache13"
	./deploy/sshpass -p $password scp -o StrictHostKeyChecking=no -pC bin/swap/swap $swap_ip:/home/utnso/cache13/swap
	./deploy/sshpass -p $password scp -o StrictHostKeyChecking=no -pC bin/swap/swap_config.cfg $swap_ip:/home/utnso/cache13/swap_config.cfg
	
	printf "Copiando archivos Administrador Memoria\n"
	./deploy/sshpass -p $password ssh -o StrictHostKeyChecking=no $mem_ip "pkill -9 -u utnso admmem"
	./deploy/sshpass -p $password ssh -o StrictHostKeyChecking=no $mem_ip "mkdir -p /home/utnso/cache13"
	./deploy/sshpass -p $password scp -o StrictHostKeyChecking=no -pC bin/admmem/admmem $mem_ip:/home/utnso/cache13/admmem
	./deploy/sshpass -p $password scp -o StrictHostKeyChecking=no -pC bin/admmem/config_admin.cfg $mem_ip:/home/utnso/cache13/config_admin.cfg
	
	printf "Copiando visor de Logs\n"
	./deploy/sshpass -p $password ssh -o StrictHostKeyChecking=no $mem_ip "pkill -9 -u utnso logview"
	./deploy/sshpass -p $password ssh -o StrictHostKeyChecking=no $swap_ip "pkill -9 -u utnso logview"
	./deploy/sshpass -p $password ssh -o StrictHostKeyChecking=no $planif_ip "pkill -9 -u utnso logview"
	./deploy/sshpass -p $password scp -o StrictHostKeyChecking=no -pC bin/logview $mem_ip:/home/utnso/cache13
	./deploy/sshpass -p $password scp -o StrictHostKeyChecking=no -pC bin/logview $swap_ip:/home/utnso/cache13
	./deploy/sshpass -p $password scp -o StrictHostKeyChecking=no -pC bin/logview $planif_ip:/home/utnso/cache13
	read -N 1 -t 10 -p "Presione una tecla para continuar o espere 10 segundos"
}

init_config
while true
do

exec 3>&1
selection=$(./deploy/dialog --no-shadow  --backtitle  "TP S.O." \
	--title "Menu Principal" \
	--cancel-label "Salir"  \
	--menu "Por favor selecciona una opcion" 20 60 11 \
		"Planif" "Confiurar Planificador" \
		"CPU" "Configurar CPU" \
		"Memoria" "Configurar Administrador Memoria" \
		"SWAP" "Configurar SWAP" \
		"Compilar" "Compila el Programa" \
		"Deploy" "Distribuir el programa" \
		"NCurses" "Dependencias de ncurses" \
		"Genesys" "Go for it!" \
		"Config" "Copia Configuracion" \
		"Copiar" "Copia local para Debug" \
		2>&1 1>&3)
exit_status=$?
exec 3>&-
case $exit_status in
	$DIALOG_CANCEL)
		clear
		exit
		;;
	$DIALOG_ESC)
		clear
		exit 1
		;;
esac
case $selection in
	0)
		clear
		exit
		;;
	Planif )
		config_Planif
		;;
	CPU )
		config_CPU
		;;
	Memoria )
		config_Memoria
		;;
	SWAP )
		config_SWAP
		;;
	Compilar )
		Compilar
		;;
	Deploy )
		enviar_sistema
		;;
	NCurses )
		echo utnso | sudo -S dpkg -i lib/libncurses5-dev.deb
		read -N 1 -t 2 -p "Presione una tecla para continuar o espere 2 segundos"
		;;
	Genesys)
		clear
		make clean
		clear
		make -r all
		read -N 1 -t 2 -p "Presione una tecla para continuar o espere 2 segundos"
		enviar_sistema
		;;
	Config)
		enviar_config
		;;
	Copiar) 
		clear
		make clean
		clear
		if [ "$modo" == "RELEASE" ]; then
			make -r all
		else
			make all
		fi
		pkill -9 -u utnso cpu
		pkill -9 -u utnso admmem
		pkill -9 -u utnso planificador
		pkill -9 -u utnso swap
		pkill -9 -u utnso logview
		cp bin/cpu/cpu ../cache13/cpu
		cp bin/admmem/admmem ../cache13/admmem
		cp bin/planificador/planificador ../cache13/planificador
		cp bin/swap/swap ../cache13/swap
		cp bin/logview ../cache13/logview
		rm ../cache13/*.html
		rm ../cache13/swap.data
		cp bin/cpu/cpu.cfg ../cache13/cpu.cfg
		cp bin/admmem/config_admin.cfg ../cache13/config_admin.cfg
		cp bin/planificador/ConfigPlanificador.cfg ../cache13/ConfigPlanificador.cfg
		cp bin/swap/swap_config.cfg ../cache13/swap_config.cfg
		read -N 1 -t 10 -p "Presione una tecla para continuar o espere 10 segundos"
		;;
esac
done


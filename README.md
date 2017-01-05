# Sistemas Operativos - TP 2015 - Genesys
Instalacion
-----------

Para proceder a la instalacion primero es necesario instalar los header de la libreria ncurses, utilizando el siguiente comando:

sudo apt-get install libncurses5-dev

Una vez instalada la libreria, para compilar el sistema se debe ejecutar `make`

Donde aparece un menu de opciones. Las primeras cuatro referidas a Planificador, CPU, SWAP y Memoria, cambian la configuracion de los modulos. Dentro de las mismas se pueden especificar las Direcciones IP y puertos donde ejecuta cada una. Planificador y CPU estaran en el mismo equipo.

Realizada la configuracion, para un deploy rapido, se puede seleccionar la opcion **Genesys** la cual hara un clean build en modo realease y copiara los archivos necesarios.

La opcion **Copiar** hace tambien un clean y un build, pero copia todos los archivos localmente a la carpeta `/home/utnso/cache13`.

Otra opcion es, una vez configurado cada modulo, con su correspondiente direccion IP, se selecciona el menu **Compilar**.
En el mismo, la ultima opcion establece si la **Compilacion es Release o Debug**.

Se recomienda hacer un **Clean antes de compilar un modulo o Todos**.

Hay una opcion para **ver el Log de errores de compilacion**, desde el mismo visor se puede elimnar el archivo.

Dentro del menu, se encuentra la opcion **Deploy**, la cual copia los binarios y sus configuraciones a los diferentes equipos, segun las direcciones IP que tienen asignadas. 

En los distintos equipos, los binarios se encuentran en la carpeta `/home/utnso/cache13`.

NOTA: El comando clean, no limpia los archivos de bibliotecas de commons, readline o history, en caso de ser necesario borrar estos archivos se puede ejecutar manualmente un `make cleanall`

NOTA2: Se agrego un comando especial que es `make _logview`, el cual compila un programa llamado logview en la carpeta bin/. Este programa no se copia en forma automatica (lo discutimos si quieren). Y se uso de la siguiente forma `logview archivo.html`. Como el nombre bien lo indica es para ver los Logs.

**That's all folks!***

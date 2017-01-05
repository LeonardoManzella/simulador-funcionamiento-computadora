/* $Id: scroll_ex.c,v 1.24 2012/03/21 23:42:48 tom Exp $ */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <regex.h>

#include <cdk_test.h>

typedef struct __attribute__ ((__packed__)) {
	char color[6];
	char hora[9];
	char tipo[7];
	char modulo[26];
	char *desc;
} t_datos_log; 

char **Lineas(char* Texto, int* Elementos);
char* parseLine(char* texto);
char* formatTexto(const char* texto, char *tagValue);
char *strip_tag(char **texto);
void handlerAlarm(int iSignal);

char* FileName = NULL;
ssize_t bytesLeidos = 0;
int ultimoItem = 0;
int largoLinea = 256;
CDKSCROLL *scrollList = 0;

void handlerAlarm(int iSignal) {
	if (iSignal != SIGALRM) {
		return;
	}
	int fdLog = open(FileName, O_RDONLY | O_NONBLOCK );
	ssize_t bytesNuevos = 0;
	char **items;
	int itemsCount = 0, iPos = 0;
	char* contenidoLog = malloc(4096);
	lseek(fdLog, bytesLeidos, SEEK_SET);
	bytesNuevos = read(fdLog, contenidoLog, 4096); // cambiar por tamaño bloque o diferencia tamaño con ultima lectura
	if (bytesNuevos > 0) {
		items = Lineas(contenidoLog, &itemsCount);
		if (itemsCount > 1) {
			while (iPos < itemsCount) {
				addCDKScrollItem (scrollList, items[iPos]);
				iPos++;
			}
		} else {
			addCDKScrollItem (scrollList, items[0]);
		}
		if (getCDKScrollCurrentItem(scrollList) == (ultimoItem-1)) {
			ultimoItem += itemsCount;
			setCDKScrollCurrentItem(scrollList, ultimoItem-1);
		} else {
			ultimoItem += itemsCount;
		}
   		refreshCDKScreen (ScreenOf (scrollList));
		bytesLeidos += bytesNuevos;
		bytesNuevos = 0;
		free(contenidoLog);
	}
	close(fdLog);
	return;
}


int main (int argc, char **argv)
{	

	/* Declare variables. */
	CDKSCREEN *cdkscreen = 0;
	WINDOW *cursesWin = 0;

	char **items = NULL;
	char *textoDelLog = NULL;
	int itemsCount = 0;
	int bytesNuevos = 0;
	char *contenidoLog = malloc(sizeof(char) * 4096);
	FileName = argv[1];
	int fdLog = open(FileName, O_RDONLY | O_NONBLOCK );
	bytesNuevos = read(fdLog, contenidoLog, 4096); // cambiar por tamaño bloque o diferencia tamaño con ultima lectura	
	textoDelLog = malloc(bytesNuevos);
	memcpy(textoDelLog, contenidoLog, bytesNuevos);
	bytesLeidos += bytesNuevos;
	while (bytesNuevos != 0) { 
		bytesNuevos = read(fdLog, contenidoLog, 4096); // cambiar por tamaño bloque o diferencia tamaño con ultima lectura	
		textoDelLog = realloc(textoDelLog, bytesLeidos + bytesNuevos);
		memcpy(textoDelLog + bytesLeidos, contenidoLog, bytesNuevos);
		bytesLeidos += bytesNuevos;
	}
	//textoDelLog[bytesLeidos] = '\0';
	items = Lineas(textoDelLog, &itemsCount);
	ultimoItem = itemsCount;
	free(contenidoLog);
	free(textoDelLog);

	close(fdLog);
	


   //const char *title = "</05>Pick a file";
	char *title = malloc(largoLinea + 1);
	sprintf(title, "</57>%8s  %-6s  %-25s  %s", "HORA", "TIPO", "FUNCION", "MENSAJE");
	int strSize = strlen(title);
	if (strSize < largoLinea) {
		memset(title + strSize, ' ', largoLinea-strSize);
		title[largoLinea] = '\0';
	}
   char **item = 0;
   const char *mesg[5];
   char temp[256];
   int selection;

   CDK_PARAMS params;

   CDKparseParams (argc, argv, &params, "cs:t:" CDK_CLI_PARAMS);

   /* Set up CDK. */
   cursesWin = initscr ();
   cdkscreen = initCDKScreen (cursesWin);
   int scr_h, scr_w;
   getmaxyx(cursesWin, scr_w, scr_h);
   /* Set up CDK Colors. */
   initCDKColor ();

   /* Create the scrolling list. */
   scrollList = newCDKScroll (cdkscreen,
			      CDKparamValue (&params, 'X', CENTER),
			      CDKparamValue (&params, 'Y', CENTER),
			      CDKparsePosition (CDKparamString2 (&params,
								 's',
								 "RIGHT")),
			      CDKparamValue (&params, 'H', scr_h),
			      CDKparamValue (&params, 'W', scr_w),
			      CDKparamString2(&params, 'c', title),
			      (CDKparamNumber (&params, 'c')
			       ? 0
			       : (CDK_CSTRING2) items),
			      (CDKparamNumber (&params, 'c')
			       ? 0
			       : itemsCount),
			      FALSE,
			      A_REVERSE,
			      CDKparamValue (&params, 'N', TRUE),
			      CDKparamValue (&params, 'S', FALSE));

   /* Is the scrolling list null? */
   if (scrollList == 0)
   {
      /* Exit CDK. */
      destroyCDKScreen (cdkscreen);
      endCDK ();

      printf ("Cannot make scrolling list. Is the window too small?\n");
      ExitProgram (EXIT_FAILURE);
   }

   if (CDKparamNumber (&params, 'c'))
  {
      setCDKScrollItems (scrollList, (CDK_CSTRING2) items, itemsCount, TRUE);
   }

	/* Activa la actualizacion del archivo */
	struct sigaction sa;
	struct itimerval timer;
	//sigset_t mask;
	timer.it_interval.tv_sec = 1;
	timer.it_interval.tv_usec = 0;
	timer.it_value.tv_usec = 0;
	timer.it_value.tv_sec = 1;

	sa.sa_handler = &handlerAlarm;
	sigfillset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sigaction(SIGALRM, &sa, NULL);

	setitimer(ITIMER_REAL, &timer, 0);
	//sigprocmask(0,NULL, &mask);

	//sigdelset(&mask, SIGALRM);

	//sigsuspend(&mask);
   	/* Activate the scrolling list. */
   	selection = activateCDKScroll (scrollList, NULL);

   /* Determine how the widget was exited. */
   if (scrollList->exitType == vESCAPE_HIT)
   {
      mesg[0] = "<C>You hit escape. No file selected.";
      mesg[1] = "";
      mesg[2] = "<C>Press any key to continue.";
      popupLabel (cdkscreen, (CDK_CSTRING2) mesg, 3);
   }
   else if (scrollList->exitType == vNORMAL)
   {
      char *theItem = chtype2Char (scrollList->item[selection]);
      mesg[0] = "<C>You selected the following line";
      sprintf (temp, "<C>%.*s", (int)(sizeof (temp) - 20), theItem);
      mesg[1] = temp;
      mesg[2] = "<C>Press any key to continue.";
      popupLabel (cdkscreen, (CDK_CSTRING2) mesg, 3);
      freeChar (theItem);
   }

   /* Clean up. */
   CDKfreeStrings (item);
   destroyCDKScroll (scrollList);
   destroyCDKScreen (cdkscreen);
   endCDK ();
   ExitProgram (EXIT_SUCCESS);
}

char **Lineas(char* Texto, int* Elementos){
	int iPos = 0, offset = 0;
	int largoString = strlen(Texto);
	char **stringResult = malloc(sizeof(char*));
	int itemsCount = 0;
	char *tmp = NULL;
	while (iPos <= largoString) {
		if (Texto[iPos] == '\n') {
			tmp = malloc((iPos - offset)+1);
			tmp[(iPos-offset)] = '\0';
			memcpy(tmp, Texto + offset, (iPos-offset));
			if ((strlen(tmp) > 1) && ((tmp[0] == '1') || (tmp[0] == '0') || (tmp[0] == '2') || (tmp[0] == '<') ) )   {
				stringResult[itemsCount] = parseLine(tmp);
				free(tmp);
				tmp = NULL;
				itemsCount++;
				stringResult = realloc(stringResult, sizeof(char*) * (itemsCount+1));
			} else {
				free(tmp);
				tmp = NULL;
			}
			iPos++;
			offset = iPos;
		}
		iPos++;
	}
	*Elementos = itemsCount;
	return stringResult;
}

char* parseLine(char* texto) {
	int largoString = strlen (texto);
	char* tagValue = NULL;
	if (texto[0] == '<') {
		tagValue = strip_tag(&texto);
		largoString -= strlen(tagValue);
	}
	return formatTexto(texto, tagValue);
}

char* formatTexto(const char* texto, char *tagValue) {
	int iPos = 0, offset = 0;
	int color = 0;
	t_datos_log datosDelLog;
	int largoString = strlen(texto);
	if (tagValue != NULL) { 
		if (tagValue[0] == 'B') {
			memcpy(datosDelLog.color, "</40>", 5);
			datosDelLog.color[5] = '\0';
			color = 5;
		} else if (tagValue[0] == 'R') {
			memcpy(datosDelLog.color, "</16>", 5);
			datosDelLog.color[5] = '\0';
			color = 5;
		} else if (tagValue[0] == 'Y') {
			memcpy(datosDelLog.color, "</32>", 5);
			datosDelLog.color[5] = '\0';
			color = 5;
		} else {
			datosDelLog.color[0] = '\0';
			color = 0;
		}
		
	}
	while (iPos < largoString) {
		if ((iPos >= 0) && (iPos <= 8)) {
			if (iPos == 8) {
				datosDelLog.hora[offset] = '\0';
				offset = 0;
			} else {
				datosDelLog.hora[offset] = texto[iPos];
				offset++;
			}
		}
		if (texto[iPos] == '[') {
			iPos++;
			while((texto[iPos] != ']') && (iPos < largoString)) {
				if ((texto[iPos] != ' ') && (offset < (sizeof(datosDelLog.tipo)-1)) ) {
					datosDelLog.tipo[offset] = texto[iPos];
					iPos++;
					offset++;
				} else {
					iPos++;
				}
			}
			iPos++;
			datosDelLog.tipo[offset] = '\0';
			offset = 0;
			
			while ((texto[iPos] != '(') && (iPos < largoString)) {
				if (offset < (sizeof(datosDelLog.modulo)-1) ) {
					datosDelLog.modulo[offset] = texto[iPos];
					offset++;
				}
				iPos++;
			}
			datosDelLog.modulo[offset] = '\0';
			offset = 0;
		}
		if (texto[iPos] == ')') {
			iPos++;
			datosDelLog.desc = malloc((largoString - iPos) + 1);
			while ((texto[iPos] != '\0') && (iPos < largoString)) {
				datosDelLog.desc[offset] = texto[iPos];
				offset++;
				iPos++;
			}
			datosDelLog.desc[offset] = '\0';
			break;
		}
		iPos++;
	}
	char* resultString = malloc(largoLinea+color+1);
	resultString[largoLinea+color] = '\0';
	memset(resultString, ' ', largoLinea+color);
	int largoBasico = sizeof(datosDelLog.hora) + sizeof(datosDelLog.tipo) + sizeof(datosDelLog.modulo) + 6;
	if (strlen(datosDelLog.desc) > (largoLinea - largoBasico)) {
		datosDelLog.desc = realloc(datosDelLog.desc, (largoLinea - (largoBasico + 1)));
		datosDelLog.desc[largoLinea - (largoBasico)] = '\0';
	}
	if (color == 0) {
		sprintf(resultString, "%8s  %-6s  %-25s  %s", datosDelLog.hora, datosDelLog.tipo, datosDelLog.modulo, datosDelLog.desc);
	} else {
		sprintf(resultString, "%5s%8s  %-6s  %-25s  %s", datosDelLog.color, datosDelLog.hora, datosDelLog.tipo, datosDelLog.modulo, datosDelLog.desc);
	}
	largoString = strlen(resultString);
	if (largoString < (largoLinea+color)) {
		memset(resultString + largoString, ' ', (largoLinea+color) - largoString);
		resultString[largoLinea+color] = '\0';
	}
	free(tagValue);
	return resultString;
}

char *strip_tag(char **texto) {
	int largoString = strlen(*texto);
	char *tmp = malloc(largoString+1);
	int iPos = 0, offset = 0;
	bool openTag = false;
	char *returnString = NULL;
	while (iPos < largoString) {
		if ((*texto)[iPos] == '<') {
			openTag = true;
			iPos++;
		}
		if ((*texto)[iPos] == '>') {
			openTag = false;
			//tmp[offset] = (*texto)[iPos];
			tmp[offset+1] = '\0';
			break;
		}
		if (openTag == true) {
			tmp[offset] = (*texto)[iPos];
			offset++;
		}
		iPos++;
	}
	if (strlen(tmp) != 0) {
		//largoString -= (iPos + 1);
		offset = 0;
		while (iPos < largoString) {
			(*texto)[offset] = (*texto)[iPos+1];
			iPos++;
			offset++;
		}
		(*texto)[offset] = '\0';
		largoString = strlen(tmp);
		returnString = malloc(largoString + 1);
		memcpy(returnString, tmp, largoString+1);
	}
	free(tmp);
	return returnString;
}


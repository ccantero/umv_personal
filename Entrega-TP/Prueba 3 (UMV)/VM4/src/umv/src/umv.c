/*
 * umv.c
 *
 *  Created on: 13/05/2014
 *      Author: utnso
 */


#include "umv.h"

int main(int argc, char *argv[])
{
	signal(SIGINT, depuracion);
	initscr();
	echo();
	scrollok(stdscr, TRUE);
	keypad(stdscr, TRUE);
	logger = log_create("Log.txt", "UMV", false, LOG_LEVEL_INFO);
	GetInfoConfFile();
	pthread_t th1;
	pthread_t conexiones[MAXCONEXIONES];
	int cant_conexiones = 0;
	pthread_create(&th1, NULL, (void *)consola, NULL);
	memoria = malloc(space);
	memoria_libre = space;
	list_programas = list_create();

	int sockfd;
	int newfd;
	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr;
	int sin_size;
	int yes = 1;
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		log_error(logger, "Error creando socket.");
		endwin();
		exit(1);
	}
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
	{
		log_error(logger, "Error en setsockopt.");
		endwin();
		exit(1);
	}
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0', 8);
	if(bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
	{
		log_error(logger, "Error en bind.");
		endwin();
		exit(1);
	}
	if(listen(sockfd, MAXCONEXIONES) == -1)
	{
		log_error(logger, "Error en listen.");
		endwin();
		exit(1);
	}
	while(1)
	{
		sin_size = sizeof(struct sockaddr_in);
		if((newfd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1)
		{
			log_error(logger, "Error en accept.");
			continue;
		}
		pthread_create(&conexiones[cant_conexiones], NULL, (void *)conexion_nueva, (void *)&newfd);
		cant_conexiones++;
	}
	pthread_join(th1, NULL);
	endwin();
	return 0;
}

void conexion_nueva(void *param)
{
	log_info(logger, "Se lanzo un hilo por conexion nueva.");
	int conexion;
	conexion = *((int *)param);
	int tipo_conexion;
	t_msg_handshake msg;
	if (recv(conexion, &msg, sizeof(t_msg_handshake), 0) == 0)
	{
		log_error(logger, "Conexion nueva desconectada.");
		endwin();
		exit(1);
	}
	log_info(logger, "Se recibio handshake de conexion nueva.");
	tipo_conexion = msg.tipo;
	msg.tipo = UMV;
	send(conexion, &msg, sizeof(t_msg_handshake), 0);
	if (tipo_conexion == CPU)
	{
		log_info(logger, "Se conecto una cpu.");
	}
	if (tipo_conexion == KERNEL)
	{
		log_info(logger, "Se conecto el kernel.");
	}
	// Bucle principal
	while(1)
	{
		switch(tipo_conexion)
		{
		case CPU:
			atender_cpu(conexion);
			break;
		case KERNEL:
			atender_kernel(conexion);
			break;
		}
	}
}

void atender_cpu(int sock)
{
	t_mensaje mensaje;
	int retorno;
	if (recv(sock, &mensaje, sizeof(t_mensaje), 0) == 0)
	{
		log_error(logger, "CPU desconectada.");
		pthread_exit(&retorno);
	}
	t_msg_cambio_proceso_activo msg;
	t_msg_solicitud_bytes msg2;
	t_msg_envio_bytes msg3;
	int respuesta;
	switch(mensaje.tipo)
	{
	case SOLICITUDBYTES:
		pthread_mutex_lock(&semRetardo);
		usleep(retardo);
		pthread_mutex_unlock(&semRetardo);
		pthread_mutex_lock(&semProcesoActivo);
		pthread_mutex_lock(&semCompactacion);
		log_info(logger, "Recibi solicitud de bytes de cpu.");
		if (recv(sock, &msg, sizeof(t_msg_cambio_proceso_activo), 0) == 0)
		{
			log_error(logger, "CPU desconectada.");
			pthread_exit(&retorno);
		}
		proceso_activo = msg.id_programa;
		if (recv(sock, &msg2, sizeof(t_msg_solicitud_bytes), 0) == 0)
		{
			log_error(logger, "CPU desconectada.");
			pthread_exit(&retorno);
		}
		respuesta = atender_solicitud_bytes(msg2.base, msg2.offset, msg2.tamanio, sock, NULL);
		if (respuesta != 0)
		{
			mensaje.tipo = respuesta;
			mensaje.id_proceso = proceso_activo;
			send(sock, &mensaje, sizeof(t_mensaje), 0);
		}
		pthread_mutex_unlock(&semCompactacion);
		pthread_mutex_unlock(&semProcesoActivo);
		log_info(logger, "Devolvi solicitud de bytes a cpu; respuesta. %d.", respuesta);
		break;
	case ENVIOBYTES:
		pthread_mutex_lock(&semRetardo);
		usleep(retardo);
		pthread_mutex_unlock(&semRetardo);
		pthread_mutex_lock(&semProcesoActivo);
		pthread_mutex_lock(&semCompactacion);
		log_info(logger, "Recibi envio de bytes de cpu.");
		if (recv(sock, &msg, sizeof(t_msg_cambio_proceso_activo), 0) == 0)
		{
			log_error(logger, "CPU desconectada.");
			pthread_exit(&retorno);
		}
		proceso_activo = msg.id_programa;
		if (recv(sock, &msg3, sizeof(t_msg_envio_bytes), 0) == 0)
		{
			log_error(logger, "CPU desconectada.");
			pthread_exit(&retorno);
		}
		respuesta = atender_envio_bytes(msg3.base, msg3.offset, msg3.tamanio, sock);
		if (respuesta != 0)
		{
			mensaje.tipo = respuesta;
			mensaje.id_proceso = proceso_activo;
			send(sock, &mensaje, sizeof(t_mensaje), 0);
		}
		pthread_mutex_unlock(&semCompactacion);
		pthread_mutex_unlock(&semProcesoActivo);
		log_info(logger, "Devolvi el envio de bytes de cpu.");
		break;
	}
}

int atender_envio_bytes(int base, int offset, int tam, int sock)
{
	char buffer[tam];
	if (sock != 0)
	{
		if (recv(sock, &buffer, sizeof(buffer), 0) == 0)
		{
			log_error(logger, "Conexion desconectada.");
		}
	}
	else
	{
		printw("Ingrese datos a escribir en memoria: \n");
		getstr(buffer);
	}
	int i;
	t_info_programa *prog;
	t_info_segmento *seg;
	int encontre_programa = 0;
	int encontre_segmento = 0;
	t_mensaje msg;
	for (i = 0; i < list_size(list_programas); i++)
	{
		prog = list_get(list_programas, i);
		if (prog->programa == proceso_activo)
		{
			encontre_programa = 1;
			break;
		}
	}
	if (encontre_programa == 1)
	{
		for (i = 0; i < list_size(prog->segmentos); i++)
		{
			seg = list_get(prog->segmentos, i);
			if (seg->dirLogica == base)
			{
				encontre_segmento = 1;
				break;
			}
		}
		if (encontre_segmento == 1)
		{
			if (offset > seg->tamanio && ((seg->tamanio - offset) >= tam))
			{
				// Devuelvo segmentation fault
				return SEGMENTATION_FAULT;
			}
			else
			{
				// La posicion de memoria es válida
				memcpy(&memoria[seg->dirFisica + offset], buffer, tam);
			}
		}
		else
		{
			return SEGMENTATION_FAULT;
		}
	}
	else
	{
		return PROGRAMA_INVALIDO;
	}
	if (sock != 0)
	{
		msg.tipo = ENVIOBYTES;
		send(sock, &msg, sizeof(t_mensaje), 0);
	}
	return 0;
}

int atender_solicitud_bytes(int base, int offset, int tam, int sock, char **buffer)
{
	t_mensaje msg;
	char buf[tam];
	int i;
	t_info_programa *prog;
	t_info_segmento *seg;
	int encontre_programa = 0;
	int encontre_segmento = 0;

	for (i = 0; i < list_size(list_programas); i++)
	{
		prog = list_get(list_programas, i);
		if (prog->programa == proceso_activo)
		{
			encontre_programa = 1;
			break;
		}
	}
	if (encontre_programa == 1)
	{
		for (i = 0; i < list_size(prog->segmentos); i++)
		{
			seg = list_get(prog->segmentos, i);
			if (seg->dirLogica == base)
			{
				encontre_segmento = 1;
				break;
			}
		}
		if (encontre_segmento == 1)
		{
			if (tam > seg->tamanio ||
				offset > seg->tamanio ||
				(seg->dirFisica+offset+tam) > (seg->dirFisica+seg->tamanio))
			{
				// Devuelvo segmentation fault
				return SEGMENTATION_FAULT;
			}
			else
			{
				// La posicion de memoria es válida
				memcpy(buf, &memoria[seg->dirFisica + offset], sizeof(buf));
			}
		}
		else
		{
			return SEGMENTATION_FAULT;
		}
	}
	else
	{
		return PROGRAMA_INVALIDO;
	}
	if (sock != 0)
	{
		msg.tipo = ENVIOBYTES;
		send(sock, &msg, sizeof(t_mensaje), 0);
		send(sock, &buf, sizeof(buf), 0);
	}
	else
	{
		memcpy(*buffer, &memoria[seg->dirFisica + offset], tam);
		return (seg->dirFisica + offset);
	}
	return 0;
}

int atender_solicitud_bytes_int(int base, int offset, int tam, int sock, int **buffer)
{
	t_mensaje msg;
	char buf[tam];
	int i;
	t_info_programa *prog;
	t_info_segmento *seg;
	int encontre_programa = 0;
	int encontre_segmento = 0;

	for (i = 0; i < list_size(list_programas); i++)
	{
		prog = list_get(list_programas, i);
		if (prog->programa == proceso_activo)
		{
			encontre_programa = 1;
			break;
		}
	}
	if (encontre_programa == 1)
	{
		for (i = 0; i < list_size(prog->segmentos); i++)
		{
			seg = list_get(prog->segmentos, i);
			if (seg->dirLogica == base)
			{
				encontre_segmento = 1;
				break;
			}
		}
		if (encontre_segmento == 1)
		{
			if (tam > seg->tamanio || offset > seg->tamanio || (seg->dirFisica+offset+tam) > (seg->dirFisica+seg->tamanio))
			{
				// Devuelvo segmentation fault
				return SEGMENTATION_FAULT;
			}
			else
			{
				// La posicion de memoria es válida
				memcpy(&buf, &memoria[seg->dirFisica + offset], sizeof(buf));
			}
		}
		else
		{
			return SEGMENTATION_FAULT;
		}
	}
	else
	{
		return PROGRAMA_INVALIDO;
	}
	if (sock != 0)
	{
		msg.tipo = ENVIOBYTES;
		send(sock, &msg, sizeof(t_mensaje), 0);
		send(sock, &buf, sizeof(buf), 0);
	}
	else
	{
		memcpy(*buffer, &memoria[seg->dirFisica + offset], tam);
		return (seg->dirFisica + offset);
	}
	return 0;
}

void atender_kernel(int sock)
{
	t_mensaje mensaje;
	if (recv(sock, &mensaje, sizeof(t_mensaje), 0) == 0)
	{
		log_error(logger, "Kernel desconectado.");
		depuracion(SIGINT);
	}
	t_msg_destruir_segmentos msg;
	t_msg_crear_segmento msg2;
	t_msg_envio_bytes msg3;
	int respuesta;
	switch(mensaje.tipo)
	{
	case DESTRUIRSEGMENTOS:
		pthread_mutex_lock(&semRetardo);
		usleep(retardo);
		pthread_mutex_unlock(&semRetardo);
		pthread_mutex_lock(&semCompactacion);
		if (recv(sock, &msg, sizeof(t_msg_destruir_segmentos), 0) == 0)
		{
			log_error(logger, "Kernel desconectado.");
			depuracion(SIGINT);
		}
		log_info(logger, "Se recibio peticion para destruir segmentos de kernel.");
		mensaje.datosNumericos = destruir_segmentos(msg.id_programa);
		pthread_mutex_unlock(&semCompactacion);
		send(sock, &mensaje, sizeof(t_mensaje), 0);
		log_info(logger, "Se eliminaron segmentos de programa %d", msg.id_programa);
		break;
	case CREARSEGMENTO:
		pthread_mutex_lock(&semRetardo);
		usleep(retardo);
		pthread_mutex_unlock(&semRetardo);
		pthread_mutex_lock(&semCompactacion);
		if (recv(sock, &msg2, sizeof(t_msg_crear_segmento), 0) == 0)
		{
			log_error(logger, "Kernel desconectado.");
			depuracion(SIGINT);
		}
		log_info(logger, "Se recibe peticion para crear segmento de kernel.");
		mensaje.datosNumericos = crear_segmento(msg2.id_programa, msg2.tamanio);
		pthread_mutex_unlock(&semCompactacion);
		mensaje.id_proceso = UMV;
		send(sock, &mensaje, sizeof(t_mensaje), 0);
		log_info(logger, "Se creo segmento de programa %d", msg2.id_programa);
		break;
	case ENVIOBYTES:
		pthread_mutex_lock(&semRetardo);
		usleep(retardo);
		pthread_mutex_unlock(&semRetardo);
		pthread_mutex_lock(&semProcesoActivo);
		pthread_mutex_lock(&semCompactacion);
		if (recv(sock, &msg, sizeof(t_msg_cambio_proceso_activo), 0) == 0)
		{
			log_error(logger, "Kernel desconectado.");
			depuracion(SIGINT);
		}
		proceso_activo = msg.id_programa;
		if (recv(sock, &msg3, sizeof(t_msg_envio_bytes), 0) == 0)
		{
			log_error(logger, "Kernel desconectado.");
			depuracion(SIGINT);
		}
		log_info(logger, "Se recibio envio de bytes de kernel.");
		respuesta = atender_envio_bytes(msg3.base, msg3.offset, msg3.tamanio, sock);
		if (respuesta != 0)
		{
			mensaje.tipo = respuesta;
			mensaje.id_proceso = proceso_activo;
			send(sock, &mensaje, sizeof(t_mensaje), 0);
		}
		log_info(logger, "Se recibieron %d bytes de programa %d", msg3.tamanio, msg.id_programa);
		pthread_mutex_unlock(&semCompactacion);
		pthread_mutex_unlock(&semProcesoActivo);
		break;
	}
}

int asignar_direccion_logica(int pid, int tamanio)
{
	t_info_programa *prog;
	t_info_segmento *segm;
	int i;
	int j;
	srand(time(NULL));
	int direccion = rand() % (space - tamanio);
	int salir = 0;
	int direccion_valida;
	for (i = 0; i < list_size(list_programas); i++)
	{
		prog = list_get(list_programas, i);
		if (prog->programa == pid)
		{
			while(salir == 0)
			{
				sleep(1);
				srand(time(NULL));
				direccion = rand() % 10000;
				if((direccion + tamanio) < 10000)
				{
					for(j = 0; j < list_size(prog->segmentos); j++)
					{
						segm = list_get(prog->segmentos, j);
						if( (direccion >= (segm->dirLogica + segm->tamanio)) ||
							((direccion + tamanio) < segm->dirLogica) )
						{
							direccion_valida = 1;
						}
						else
						{
							direccion_valida = 0;
							break;
						}
					}
					if (direccion_valida == 1)
					{
						salir = 1;
					}
					else
					{
						direccion_valida = 0;
					}
				}
			}
		}
	}
	return direccion;
}

int asignar_direccion_en_memoria(int tamanio)
{
	pthread_mutex_lock(&semAlgoritmo);
	int dir;
	if (obtener_cant_segmentos() == 0)
	{
		pthread_mutex_unlock(&semAlgoritmo);
		return 0;
	}
	else
	{
		if (!strcmp(algoritmo, "WF") ||
			!strcmp(algoritmo, "Wf") ||
			!strcmp(algoritmo, "wf") ||
			!strcmp(algoritmo, "Worst-Fit") ||
			!strcmp(algoritmo, "Worst-fit") ||
			!strcmp(algoritmo, "worst-fit"))
		{
			// El algoritmo es worst-fit
			pthread_mutex_unlock(&semAlgoritmo);
			dir = asignar_direccion_wf(tamanio);
			return dir;
		}
		else
		{
			// El algoritmo es first-fit
			pthread_mutex_unlock(&semAlgoritmo);
			dir = asignar_direccion_ff(tamanio);
			return dir;
		}
	}
}

int asignar_direccion_wf(int tamanio)
{
	int espacio_mas_grande = 0;
	int nuevo_espacio;
	int ultimo_limite = 0;
	int ultima_base;
	int primer_direccion = 0;
	int i;
	int direccion_retorno = -1;
	for (i = 0; i < obtener_cant_segmentos(); i++)
	{
		ultima_base = obtener_direccion_segmento(primer_direccion);
		ultimo_limite = obtener_direccion_mas_offset_segmento(primer_direccion);
		nuevo_espacio = ultima_base - primer_direccion;
		if (nuevo_espacio > espacio_mas_grande)
		{
			espacio_mas_grande = nuevo_espacio;
			direccion_retorno = primer_direccion;
		}
		primer_direccion = ultimo_limite + 1;
	}
	ultima_base = space;
	nuevo_espacio = ultima_base - ultimo_limite;
	if (nuevo_espacio > espacio_mas_grande)
	{
		espacio_mas_grande = nuevo_espacio;
		direccion_retorno = primer_direccion;
	}
	if (espacio_mas_grande == 0)
	{
		return -1;
	}
	else
	{
		if (espacio_mas_grande >= tamanio)
		{
			return direccion_retorno;
		}
		else
		{
			return -1;
		}
	}
}

int obtener_direccion_segmento(int arranque)
{
	int i;
	int j;
	t_info_programa *prog;
	t_info_segmento *segm;
	int primer_direccion = space;
	for (i = 0; i < list_size(list_programas); i++)
	{
		prog = list_get(list_programas, i);
		for (j = 0; j < list_size(prog->segmentos); j++)
		{
			segm = list_get(prog->segmentos, j);
			if (segm->dirFisica < primer_direccion && segm->dirFisica >= arranque)
			{
				primer_direccion = segm->dirFisica;
			}
		}
	}
	return primer_direccion;
}

int obtener_direccion_mas_offset_segmento(int arranque)
{
	int i;
	int j;
	t_info_programa *prog;
	t_info_segmento *segm;
	int primer_direccion = space;
	int direccion_mas_offset;
	for (i = 0; i < list_size(list_programas); i++)
	{
		prog = list_get(list_programas, i);
		for (j = 0; j < list_size(prog->segmentos); j++)
		{
			segm = list_get(prog->segmentos, j);
			if (segm->dirFisica < primer_direccion && segm->dirFisica >= arranque)
			{
				primer_direccion = segm->dirFisica;
				direccion_mas_offset = segm->dirFisica + segm->tamanio - 1;
			}
		}
	}
	return direccion_mas_offset;
}

int asignar_direccion_ff(int tamanio)
{
	int dir;
	int primer_direccion = space;
	int ultima_direccion = 0;
	int direccion_arranque = 0;
	int salir = 0;
	int i;
	int j;
	t_info_programa *prog;
	t_info_segmento *segm;
	while (salir == 0)
	{
		for (i = 0; i < list_size(list_programas); i++)
		{
			prog = list_get(list_programas, i);
			for (j = 0; j < list_size(prog->segmentos); j++)
			{
				segm = list_get(prog->segmentos, j);
				if (segm->dirFisica < primer_direccion && segm->dirFisica >= direccion_arranque)
				{
					primer_direccion = segm->dirFisica;
					ultima_direccion = segm->dirFisica + segm->tamanio - 1;
				}
			}
		}
		if (direccion_arranque == 0)
		{
			if (primer_direccion >= tamanio)
			{
				dir = direccion_arranque;
				salir = 1;
			}
			else
			{
				direccion_arranque = ultima_direccion + 1;
				primer_direccion = space;
			}
		}
		else
		{
			if (primer_direccion - direccion_arranque >= tamanio)
			{
				dir = direccion_arranque;
				salir = 1;
			}
			else
			{
				direccion_arranque = ultima_direccion + 1;
				primer_direccion = space;
			}
		}
	}
	return dir;
}

void consola (void* param)
{
	cantidad_dumps = 0;
	char comando[50];
	char comando2[50];
	int flag_comandoOK;
	int flag_comandoOK2;
	int nuevo_valor;
	int valor_numerico;
	int valor_numerico2;
	int valor_numerico3;
	int proc_id = -1;
	int respuesta;
	int dir_fisica;
	char *buffer;
	log_info(logger, "Se lanzo el hilo de consola");
	printw("-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-\n");
	printw("         Bienvenido a la consola de UMV           \n");
	printw("-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-\n");
	refresh();
	int i;
	int cant_num_leidos;
	int *buffer_int;
	int tecla_ingresada;
	int tecla_ingresada2;
	// Bucle principal esperando peticiones del usuario
	for(;;)
	{
		flag_comandoOK = 0;
		flag_comandoOK2 = 0;
		fflush(stdin);
    	printw("Ingrese el comando: \n");
    	printw(">");
    	refresh();
    	getstr(comando);
   		if (flag_comandoOK == 0 && strcmp(comando, "operacion") == 0)
   		{
   			printw("Operaciones disponibles: \n");
   			printw("\tcrear segmento\n");
   			printw("\tsolicitar memoria\n");
   			printw("\tescribir memoria\n");
   			printw("\tdestruir segmentos\n");
   			printw("Ingrese la operacion: \n");
   			printw(">");
   			refresh();
   			getstr(comando2);
   			if (flag_comandoOK2 == 0 && strcmp(comando2, "crear segmento") == 0)
   			{
   				printw("Ingrese proceso: \n");
   				printw(">");
   				refresh();
   				scanw("%d", &proc_id);
   				respuesta = verificar_proc_id(proc_id);
   				if (respuesta == 0)
   				{
   					printw("El proceso ingresado no existe en memoria.\n");
   					printw("Ingrese un proceso que exista en memoria para crear un segmento.\n");
   					refresh();
   				}
   				else
   				{
   					printw("Ingrese tamanio del segmento: \n");
   					printw(">");
   					refresh();
   					scanw("%d", &valor_numerico);
   					pthread_mutex_lock(&semCompactacion);
   					respuesta = crear_segmento(proc_id, valor_numerico);
   					pthread_mutex_unlock(&semCompactacion);
   					switch(respuesta)
   					{
   					case -1:
   						printw("No hay memoria disponible para ese tamanio de segmento.\n");
   						printw("El segmento no fue creado.\n");
   						refresh();
   						break;
   					default:
   						log_info(logger, "Segmento de proceso %d creado.", proc_id);
   						printw("El segmento fue creado.\n");
   						dir_fisica = transformar_direccion_en_fisica(respuesta, proc_id);
   						printw("Posicion en memoria del segmento: %d\n", dir_fisica);
   						refresh();
   						break;
   					}
   				}
   				flag_comandoOK2 = 1;
   			}
   			if (flag_comandoOK2 == 0 && strcmp(comando2, "solicitar memoria") == 0)
   			{
   				printw("Ingrese proceso: \n");
   				printw(">");
   				refresh();
   				scanw("%d", &proc_id);
   				respuesta = verificar_proc_id(proc_id);
   				if (respuesta == 0)
   				{
   					printw("El proceso ingresado no existe en memoria.\n");
   					printw("Ingrese un proceso que exista en memoria.\n");
   					refresh();
   				}
   				else
   				{
   					printw("Ingrese base: \n");
   					printw(">");
   					refresh();
   					scanw("%d", &valor_numerico);
   					printw("Ingrese offset: \n");
   					printw(">");
   					refresh();
   					scanw("%d", &valor_numerico2);
   					printw("Ingrese cantidad de bytes a leer: \n");
   					printw(">");
   					refresh();
   					scanw("%d", &valor_numerico3);
   					buffer = (char *)malloc(valor_numerico3);
   					valor_numerico = transformar_direccion_en_logica(valor_numerico, proc_id);
   					pthread_mutex_lock(&semProcesoActivo);
   					pthread_mutex_lock(&semCompactacion);
   					respuesta = atender_solicitud_bytes(valor_numerico, valor_numerico2, valor_numerico3, 0, &buffer);
   					pthread_mutex_unlock(&semCompactacion);
   					pthread_mutex_unlock(&semProcesoActivo);
   					switch(respuesta)
   					{
   					case PROGRAMA_INVALIDO:
   						printw("El proceso es invalido.\n");
   						refresh();
   						break;
   					case SEGMENTATION_FAULT:
   						printw("Segmentation fault.\n");
   						refresh();
   						break;
   					default:
   						printw("Posicion de memoria solicitada: %d\n", respuesta);
   						printw("Contenido: ");
   						for (i = 0; i < valor_numerico3; i++)
   						{
   							printw("%c", buffer[i]);
   						}
   						printw("\n");
   						refresh();
   						free(buffer);
   						break;
   					}
   				}
   				flag_comandoOK2 = 1;
   			}
   			if (flag_comandoOK2 == 0 && strcmp(comando2, "solicitar memoria int") == 0)
			{
				printw("Ingrese proceso: \n");
				printw(">");
				refresh();
				scanw("%d", &proc_id);
				respuesta = verificar_proc_id(proc_id);
				if (respuesta == 0)
				{
					printw("El proceso ingresado no existe en memoria.\n");
					printw("Ingrese un proceso que exista en memoria.\n");
					refresh();
				}
				else
				{
					printw("Ingrese base: \n");
					printw(">");
					refresh();
					scanw("%d", &valor_numerico);
					printw("Ingrese offset: \n");
					printw(">");
					refresh();
					scanw("%d", &valor_numerico2);
					printw("Ingrese cantidad de bytes a leer: \n");
					printw(">");
					refresh();
					scanw("%d", &valor_numerico3);
					buffer_int = (int *)malloc(valor_numerico3);
					valor_numerico = transformar_direccion_en_logica(valor_numerico, proc_id);
					pthread_mutex_lock(&semProcesoActivo);
					pthread_mutex_lock(&semCompactacion);
					respuesta = atender_solicitud_bytes_int(valor_numerico, valor_numerico2, valor_numerico3, 0, &buffer_int);
					pthread_mutex_unlock(&semCompactacion);
					pthread_mutex_unlock(&semProcesoActivo);
					switch(respuesta)
					{
					case PROGRAMA_INVALIDO:
						printw("El proceso es invalido.\n");
						refresh();
						break;
					case SEGMENTATION_FAULT:
						printw("Segmentation fault.\n");
						refresh();
						break;
					default:
						printw("Posicion de memoria solicitada: %d\n", respuesta);
						printw("Contenido: \n");
						cant_num_leidos = (int)valor_numerico3 / 4;
						for (i = 0; i < cant_num_leidos; i++)
						{
							printw("%d\n", buffer_int[i]);
						}
						refresh();
						free(buffer);
						break;
					}
				}
				flag_comandoOK2 = 1;
			}
   			if (flag_comandoOK2 == 0 && strcmp(comando2, "escribir memoria") == 0)
   			{
   				printw("Ingrese proceso: \n");
   				printw(">");
   				refresh();
   				scanw("%d", &proc_id);
   				respuesta = verificar_proc_id(proc_id);
   				if (respuesta == 0)
   				{
   					printw("El proceso ingresado no existe en memoria.\n");
   					printw("Ingrese un proceso que exista en memoria.\n");
   					refresh();
   				}
   				else
   				{
   					printw("Ingrese base: \n");
   					printw(">");
   					refresh();
   					scanw("%d", &valor_numerico);
   					printw("Ingrese offset: \n");
   					printw(">");
   					refresh();
   					scanw("%d", &valor_numerico2);
   					printw("Ingrese cantidad de bytes a escribir: \n");
   					printw(">");
   					refresh();
   					scanw("%d", &valor_numerico3);
   					valor_numerico = transformar_direccion_en_logica(valor_numerico, proc_id);
   					pthread_mutex_lock(&semProcesoActivo);
   					pthread_mutex_lock(&semCompactacion);
   					respuesta = atender_envio_bytes(valor_numerico, valor_numerico2, valor_numerico3, 0);
   					pthread_mutex_unlock(&semCompactacion);
   					pthread_mutex_unlock(&semProcesoActivo);
   					switch(respuesta)
   					{
   					case 0:
   						printw("Se escribio la memoria satisfactoriamente.\n");
   						refresh();
   						break;
   					case PROGRAMA_INVALIDO:
   						printw("El proceso es invalido.\n");
   						refresh();
   						break;
   					case SEGMENTATION_FAULT:
   						printw("Segmentation fault.\n");
   						refresh();
   						break;
   					}
   				}
   				flag_comandoOK2 = 1;
   			}
   			if (flag_comandoOK2 == 0 && strcmp(comando2, "destruir segmentos") == 0)
   			{
   				printw("Ingrese proceso: \n");
   				printw(">");
   				refresh();
   				scanw("%d", &proc_id);
   				pthread_mutex_lock(&semCompactacion);
   				respuesta = destruir_segmentos(proc_id);
   				pthread_mutex_unlock(&semCompactacion);
   				if (respuesta == 1)
   				{
   					log_info(logger, "Segmentos de proceso %d destruidos satisfactoriamente.", proc_id);
   					printw("Los segmentos del programa fueron destruidos satisfactoriamente.\n");
   					refresh();
   				}
   				else
   				{
   					printw("El programa ingresado no es valido.\n");
   					refresh();
   				}
   				flag_comandoOK2 = 1;
   			}
   			flag_comandoOK = 1;
   		}
   		if (flag_comandoOK == 0 && strcmp(comando, "retardo") == 0)
   		{
   			log_info(logger, "Se cambio el valor de retardo por consola.");
   			printw("Nuevo valor de retardo: ");
   			refresh();
   			scanw("%d", &nuevo_valor);
   			printw("Valor anterior de retardo: %d\n", retardo);
   			cambiar_retardo(nuevo_valor);
   			printw("Valor actual de retardo: %d\n", retardo);
   			refresh();
   			flag_comandoOK = 1;
   		}
   		if (flag_comandoOK == 0 && strcmp(comando, "algoritmo") == 0)
   		{
   			log_info(logger, "Se cambio el algoritmo por consola.");
   			printw("Se cambio el algoritmo.\n");
   			printw("Algoritmo anterior: %s\n", algoritmo);
   			cambiar_algoritmo();
   			printw("Algoritmo actual: %s\n", algoritmo);
   			refresh();
   		   	flag_comandoOK = 1;
   		}
   		if (flag_comandoOK == 0 && strcmp(comando, "compactacion") == 0)
   		{
   			log_info(logger, "Se pidio compactar memoria por consola.");
   			pthread_mutex_lock(&semCompactacion);
   			compactar_memoria();
   			pthread_mutex_unlock(&semCompactacion);
   		   	flag_comandoOK = 1;
   		}
   		if (flag_comandoOK == 0 && strcmp(comando, "dump procesos") == 0)
   		{
   			log_info(logger, "Se pidio dump de procesos.");
   			printw("¿Desea mostrar la informacion de todos los procesos? (s/n): ");
   			tecla_ingresada = getch();
   			printw("\n");
   			refresh();
   			printw("¿Desea guardar los datos del dump en un archivo en disco? (s/n): ");
			tecla_ingresada2 = getch();
			printw("\n");
			refresh();
   			if (tecla_ingresada != 115)
   			{
   				printw("Ingrese el id del proceso que desea mostrar informacion: ");
   				refresh();
   				scanw("%d", &proc_id);
   				respuesta = verificar_proc_id(proc_id);
   				if (respuesta == 0)
   				{
   					printw("El proceso ingresado no existe en memoria.\n");
   					printw("Ingrese un proceso que exista en memoria.\n");
   					refresh();
   				}
   				else
   				{
   					pthread_mutex_lock(&semCompactacion);
   					dump_proceso(proc_id, tecla_ingresada2);
   					pthread_mutex_unlock(&semCompactacion);
   				}
   			}
   			else
   			{
   				pthread_mutex_lock(&semCompactacion);
   				dump_memoria(tecla_ingresada2);
   				pthread_mutex_unlock(&semCompactacion);
   			}
   			flag_comandoOK = 1;
   		}
   		if (flag_comandoOK == 0 && strcmp(comando, "dump memoria") == 0)
   		{
   			printw("¿Desea guardar los datos del dump en un archivo en disco? (s/n): ");
   			tecla_ingresada = getch();
   			printw("\n");
   			printw("Ingrese direccion de memoria de comienzo: \n");
			printw(">");
			refresh();
			scanw("%d", &valor_numerico);
			printw("Ingrese cantidad de bytes a leer: \n");
			printw(">");
			refresh();
			scanw("%d", &valor_numerico2);
			if (valor_numerico + valor_numerico2 <= space && valor_numerico >= 0 && valor_numerico2 >= 0)
			{
				pthread_mutex_lock(&semCompactacion);
				mostrar_memoria(valor_numerico, valor_numerico2, tecla_ingresada);
				pthread_mutex_unlock(&semCompactacion);
			}
			printw("\n");
   			flag_comandoOK = 1;
   		}
   		if (flag_comandoOK == 0 && strcmp(comando, "help") == 0)
   		{
   			printw("Los unicos comandos habilitados son: \n");
   			printw("\toperacion\n");
   			printw("\tretardo\n");
   			printw("\talgoritmo\n");
   			printw("\tcompactacion\n");
   			printw("\tdump memoria\n");
   			printw("\tdump procesos\n");
   			refresh();
   			flag_comandoOK = 1;
   		}
   		if(flag_comandoOK == 0)
   		{
   			printw("Por favor verifique la sintaxis de los comandos utilizados.\n");
   			printw("Los unicos comandos habilitados son: \n");
   			printw("\toperacion\n");
   			printw("\tretardo\n");
   			printw("\talgoritmo\n");
   			printw("\tcompactacion\n");
   			printw("\tdump memoria\n");
   			printw("\tdump procesos\n");
   			refresh();
   		}
   	}
}

void dump_proceso(int proc, int opcion)
{
	int cant_espacios;
	char buffer_aux[50];
	int cont_aux;
	cantidad_dumps++;
	if (opcion == 115)
	{
		strcpy(nombre_archivo_dump, "dump");
		sprintf(buff_dump, "%d", cantidad_dumps);
		strcat(nombre_archivo_dump, buff_dump);
		archivo_dump = fopen(nombre_archivo_dump, "w+");
		tiempo_dump = time(NULL);
		ptr_tiempo_dump = gmtime(&tiempo_dump);
		tiempo_dump_archivo = asctime(ptr_tiempo_dump);
		fprintf(archivo_dump, "%s", tiempo_dump_archivo);
		fprintf(archivo_dump, "\n");
	}
	clear();
	refresh();
	int x = 0;
	int y = 0;
	move(y, x);
	printw("Id Proceso\n");
	x += 15;
	if (opcion == 115)
	{
		fprintf(archivo_dump, "Id Proceso");
		fprintf(archivo_dump, "     ");
	}
	move(y, x);
	printw("Dir. Fisica Segmento\n");
	x += 25;
	if (opcion == 115)
	{
		fprintf(archivo_dump, "Dir. Fisica Segmento");
		fprintf(archivo_dump, "     ");
	}
	move(y, x);
	printw("Dir. Logica Segmento\n");
	x += 30;
	if (opcion == 115)
	{
		fprintf(archivo_dump, "Dir. Logica Segmento");
		fprintf(archivo_dump, "          ");
	}
	move(y, x);
	printw("Tam. Segm.\n");
	if (opcion == 115)
	{
		fprintf(archivo_dump, "Tam. Segm.");
		fprintf(archivo_dump, "\n");
	}
	refresh();
	t_info_programa *prog;
	t_info_segmento *seg;
	int i;
	int j;
	x = 0;
	y = 1;
	for (i = 0; i < list_size(list_programas); i++)
	{
		prog = list_get(list_programas, i);
		if (prog->programa == proc)
		{
			break;
		}
	}
	for (j = 0; j < list_size(prog->segmentos); j++)
	{
		seg = list_get(prog->segmentos, j);
		move(y, x);
		printw("%d", seg->id);
		x += 15;
		if (opcion == 115)
		{
			fprintf(archivo_dump, "%d", seg->id);
			sprintf(buffer_aux, "%d", seg->id);
			cant_espacios = 5 + 10 - strlen(buffer_aux);
			for (cont_aux = 0; cont_aux < cant_espacios; cont_aux++)
			{
				buffer_aux[cont_aux] = ' ';
			}
			buffer_aux[cont_aux] = '\0';
			fprintf(archivo_dump, "%s", buffer_aux);
		}
		move(y, x);
		printw("%d", seg->dirFisica);
		x += 25;
		if (opcion == 115)
		{
			fprintf(archivo_dump, "%d", seg->dirFisica);
			sprintf(buffer_aux, "%d", seg->dirFisica);
			cant_espacios = 5 + 20 - strlen(buffer_aux);
			for (cont_aux = 0; cont_aux < cant_espacios; cont_aux++)
			{
				buffer_aux[cont_aux] = ' ';
			}
			buffer_aux[cont_aux] = '\0';
			fprintf(archivo_dump, "%s", buffer_aux);
		}
		move(y, x);
		printw("%d", seg->dirLogica);
		x += 30;
		if (opcion == 115)
		{
			fprintf(archivo_dump, "%d", seg->dirLogica);
			sprintf(buffer_aux, "%d", seg->dirLogica);
			cant_espacios = 5 + 25 - strlen(buffer_aux);
			for (cont_aux = 0; cont_aux < cant_espacios; cont_aux++)
			{
				buffer_aux[cont_aux] = ' ';
			}
			buffer_aux[cont_aux] = '\0';
			fprintf(archivo_dump, "%s", buffer_aux);
		}
		move(y, x);
		printw("%d", seg->tamanio);
		if (opcion == 115)
		{
			fprintf(archivo_dump, "%d", seg->tamanio);
			fprintf(archivo_dump, "\n");
		}
		y++;
		x = 0;
		refresh();
	}
	printw("\n");
	refresh();
}

void mostrar_memoria(int base, int tam, int opcion)
{
	cantidad_dumps++;
	if (opcion == 115)
	{
		strcpy(nombre_archivo_dump, "dump");
		sprintf(buff_dump, "%d", cantidad_dumps);
		strcat(nombre_archivo_dump, buff_dump);
		archivo_dump = fopen(nombre_archivo_dump, "w+");
		tiempo_dump = time(NULL);
		ptr_tiempo_dump = gmtime(&tiempo_dump);
		tiempo_dump_archivo = asctime(ptr_tiempo_dump);
		fprintf(archivo_dump, "%s", tiempo_dump_archivo);
		fprintf(archivo_dump, "\n");
	}
	clear();
	refresh();
	int i = 0;
	for (i = base; i < (base + tam); i++)
	{
		printw("%c", memoria[i]);
		refresh();
		if (opcion == 115)
		{
			fprintf(archivo_dump, "%c", memoria[i]);
		}
	}
	if (opcion == 115)
	{
		fclose(archivo_dump);
	}
	refresh();
}

void dump_memoria(int opcion)
{
	int cant_espacios;
	char buffer_aux[50];
	int cont_aux;
	cantidad_dumps++;
	if (opcion == 115)
	{
		strcpy(nombre_archivo_dump, "dump");
		sprintf(buff_dump, "%d", cantidad_dumps);
		strcat(nombre_archivo_dump, buff_dump);
		archivo_dump = fopen(nombre_archivo_dump, "w+");
		tiempo_dump = time(NULL);
		ptr_tiempo_dump = gmtime(&tiempo_dump);
		tiempo_dump_archivo = asctime(ptr_tiempo_dump);
		fprintf(archivo_dump, "%s", tiempo_dump_archivo);
		fprintf(archivo_dump, "\n");
	}
	int mem_utilizada = 0;
	clear();
	refresh();
	int x = 0;
	int y = 0;
	move(y, x);
	printw("Id Proceso\n");
	x += 15;
	if (opcion == 115)
	{
		fprintf(archivo_dump, "Id Proceso");
		fprintf(archivo_dump, "     ");
	}
	move(y, x);
	printw("Dir. Fisica Segmento\n");
	x += 25;
	if (opcion == 115)
	{
		fprintf(archivo_dump, "Dir. Fisica Segmento");
		fprintf(archivo_dump, "     ");
	}
	move(y, x);
	printw("Dir. Logica Segmento\n");
	x += 30;
	if (opcion == 115)
	{
		fprintf(archivo_dump, "Dir. Logica Segmento");
		fprintf(archivo_dump, "          ");
	}
	move(y, x);
	printw("Tam. Segm.\n");
	if (opcion == 115)
	{
		fprintf(archivo_dump, "Tam. Segm.");
		fprintf(archivo_dump, "\n");
	}
	refresh();
	t_info_programa *prog;
	t_info_segmento *seg;
	int i;
	int j;
	x = 0;
	y = 1;
	for (i = 0; i < list_size(list_programas); i++)
	{
		prog = list_get(list_programas, i);
		for (j = 0; j < list_size(prog->segmentos); j++)
		{
			seg = list_get(prog->segmentos, j);
			move(y, x);
			printw("%d", seg->id);
			x += 15;
			if (opcion == 115)
			{
				fprintf(archivo_dump, "%d", seg->id);
				sprintf(buffer_aux, "%d", seg->id);
				cant_espacios = 5 + 10 - strlen(buffer_aux);
				for (cont_aux = 0; cont_aux < cant_espacios; cont_aux++)
				{
					buffer_aux[cont_aux] = ' ';
				}
				buffer_aux[cont_aux] = '\0';
				fprintf(archivo_dump, "%s", buffer_aux);
			}
			move(y, x);
			printw("%d", seg->dirFisica);
			x += 25;
			if (opcion == 115)
			{
				fprintf(archivo_dump, "%d", seg->dirFisica);
				sprintf(buffer_aux, "%d", seg->dirFisica);
				cant_espacios = 5 + 20 - strlen(buffer_aux);
				for (cont_aux = 0; cont_aux < cant_espacios; cont_aux++)
				{
					buffer_aux[cont_aux] = ' ';
				}
				buffer_aux[cont_aux] = '\0';
				fprintf(archivo_dump, "%s", buffer_aux);
			}
			move(y, x);
			printw("%d", seg->dirLogica);
			x += 30;
			if (opcion == 115)
			{
				fprintf(archivo_dump, "%d", seg->dirLogica);
				sprintf(buffer_aux, "%d", seg->dirLogica);
				cant_espacios = 5 + 25 - strlen(buffer_aux);
				for (cont_aux = 0; cont_aux < cant_espacios; cont_aux++)
				{
					buffer_aux[cont_aux] = ' ';
				}
				buffer_aux[cont_aux] = '\0';
				fprintf(archivo_dump, "%s", buffer_aux);
			}
			move(y, x);
			printw("%d", seg->tamanio);
			if (opcion == 115)
			{
				fprintf(archivo_dump, "%d", seg->tamanio);
				fprintf(archivo_dump, "\n");
			}
			y++;
			x = 0;
			refresh();
			mem_utilizada += seg->tamanio;
		}
	}
	y++;
	move(y, x);
	printw("Memoria total: %d\n", space);
	if (opcion == 115)
	{
		fprintf(archivo_dump, "Memoria total: %d\n", space);
	}
	printw("Memoria utilizada: %d\n", mem_utilizada);
	if (opcion == 115)
	{
		fprintf(archivo_dump, "Memoria utilizada: %d\n", mem_utilizada);
		fclose(archivo_dump);
	}
	refresh();
}

int verificar_proc_id(int pid)
{
	int i;
	t_info_programa *prog;
	for (i = 0; i < list_size(list_programas); i++)
	{
		prog = list_get(list_programas, i);
		if (prog->programa == pid)
		{
			return 1;
		}
	}
	return 0;
}

int transformar_direccion_en_fisica(int direccion, int pid)
{
	t_info_programa *prog;
	t_info_segmento *segm;
	int i;
	int j;
	for (i = 0; i < list_size(list_programas); i++)
	{
		prog = list_get(list_programas, i);
		if (pid == prog->programa)
		{
			for (j = 0; j < list_size(prog->segmentos); j++)
			{
				segm = list_get(prog->segmentos, j);
				if (segm->dirLogica == direccion)
				{
					return segm->dirFisica;
				}
			}
		}
	}
	return -1;
}

int transformar_direccion_en_logica(int direccion, int pid)
{
	t_info_programa *prog;
	t_info_segmento *segm;
	int i;
	int j;
	for (i = 0; i < list_size(list_programas); i++)
	{
		prog = list_get(list_programas, i);
		if (pid == prog->programa)
		{
			for (j = 0; j < list_size(prog->segmentos); j++)
			{
				segm = list_get(prog->segmentos, j);
				if (segm->dirFisica == direccion)
				{
					return segm->dirLogica;
				}
			}
		}
	}
	return -1;
}

int crear_segmento(int idproc, int tamanio)
{
	int tamanio_lista = list_size(list_programas);
	t_info_programa *prog;
	t_info_segmento *seg;
	int i;
	int encontre_programa = 0;
	int hay_espacio_en_memoria;
	int respuesta;
	if (tamanio <= memoria_libre)
	{
		hay_espacio_en_memoria = 1;
	}
	else
	{
		hay_espacio_en_memoria = 0;
	}
	if (hay_espacio_en_memoria == 1)
	{
		respuesta = asignar_direccion_en_memoria(tamanio);
		if (respuesta == -1)
		{
			compactar_memoria();
		}
		for (i = 0; i < tamanio_lista; i++)
		{
			prog = list_get(list_programas, i);
			if (prog->programa == idproc)
			{
				encontre_programa = 1;
				break;
			}
		}
		if (encontre_programa == 1)
		{
			seg = (t_info_segmento *)malloc(sizeof(t_info_segmento));
			seg->id = idproc;
			seg->tamanio = tamanio;
			seg->dirFisica = asignar_direccion_en_memoria(tamanio);
			seg->dirLogica = asignar_direccion_logica(idproc, tamanio);
			list_add(prog->segmentos, seg);
			memoria_libre -= tamanio;
			return seg->dirLogica;
		}
		else
		{
			prog = (t_info_programa *)malloc(sizeof(t_info_programa));
			prog->programa = idproc;
			prog->segmentos = list_create();
			seg = (t_info_segmento *)malloc(sizeof(t_info_segmento));
			seg->id = idproc;
			seg->tamanio = tamanio;
			seg->dirFisica = asignar_direccion_en_memoria(tamanio);
			seg->dirLogica = asignar_direccion_logica(idproc, tamanio);
			list_add(prog->segmentos, seg);
			list_add(list_programas, prog);
			memoria_libre -= tamanio;
			return seg->dirLogica;
		}
	}
	else
	{
		return -1;
	}
}

int destruir_segmentos(int idproc)
{
	int tamanio_lista = list_size(list_programas);
	int i;
	int indice;
	int encontre_programa = 0;
	t_info_programa *prog;
	t_info_segmento *segm;
	// Busco al programa en mi lista de programas
	for (i = 0; i < tamanio_lista; i++)
	{
		prog = list_get(list_programas, i);
		if (prog->programa == idproc)
		{
			indice = i;
			encontre_programa = 1;
			break;
		}
	}
	if (encontre_programa == 1)
	{
		for (i = list_size(prog->segmentos) - 1; i >= 0; i--)
		{
			segm = list_get(prog->segmentos, i);
			memoria_libre += segm->tamanio;
			list_remove(prog->segmentos, i);
		}
		list_destroy(prog->segmentos);
		list_remove(list_programas, indice);
		return 1;
	}
	else
	{
		return 0;
	}
}

void cambiar_algoritmo()
{
	pthread_mutex_lock(&semAlgoritmo);
	if (!strcmp(algoritmo, "WF") ||
		!strcmp(algoritmo, "Wf") ||
		!strcmp(algoritmo, "wf") ||
		!strcmp(algoritmo, "Worst-Fit") ||
		!strcmp(algoritmo, "Worst-fit") ||
		!strcmp(algoritmo, "worst-fit"))
	{
		strcpy(algoritmo, "First-Fit");
	}
	else
	{
		strcpy(algoritmo, "Worst-Fit");
	}
	pthread_mutex_unlock(&semAlgoritmo);
}

void cambiar_retardo(int valor)
{
	pthread_mutex_lock(&semRetardo);
	retardo = valor;
	pthread_mutex_unlock(&semRetardo);
}

void compactar_memoria()
{
	int primer_direccion = space;
	int primer_programa;
	int i;
	int j;
	int nueva_direccion = 0;
	int arranque = 0;
	t_info_programa *prog;
	t_info_segmento *segm;
	int cant_segmentos = obtener_cant_segmentos();
	int cont;
	if (cant_segmentos != 0)
	{
		printw("Compactando...\n");
		refresh();
		for (cont = 0; cont < cant_segmentos; cont++)
		{
			// Busco la menor direccion en mi lista de segmentos utilizados
			for (i = 0; i < list_size(list_programas); i++)
			{
				prog = list_get(list_programas, i);
				for (j = 0; j < list_size(prog->segmentos); j++)
				{
					segm = list_get(prog->segmentos, j);
					if (segm->dirFisica < primer_direccion && segm->dirFisica >= arranque)
					{
						primer_direccion = segm->dirFisica;
						primer_programa = segm->id;
					}
				}
			}
			for (i = 0; i < list_size(list_programas); i++)
			{
				prog = list_get(list_programas, i);
				if (prog->programa == primer_programa)
				{
					break;
				}
			}
			for (j = 0; j < list_size(prog->segmentos); j++)
			{
				segm = list_get(prog->segmentos, j);
				if (segm->dirFisica == primer_direccion)
				{
					break;
				}
			}
			memcpy(&memoria[nueva_direccion], &memoria[segm->dirFisica], segm->tamanio);
			segm->dirFisica = nueva_direccion;
			nueva_direccion = segm->dirFisica + segm->tamanio;
			primer_direccion = space;
			arranque = nueva_direccion;
		}
	}
	else
	{
		printw("Todavia no hay segmentos a compactar.\n");
		refresh();
	}
}

int obtener_cant_segmentos()
{
	int cant = 0;
	int i;
	t_info_programa *prog;
	for (i = 0; i < list_size(list_programas); i++)
	{
		prog = list_get(list_programas, i);
		cant += list_size(prog->segmentos);
	}
	return cant;
}

void GetInfoConfFile(void)
{
	t_config* config;
	config = config_create(PATH_CONFIG);
	if (config_has_property(config, "IP"))
	{
		myip = (char*) malloc(strlen(config_get_string_value(config, "IP")) + 1);
		strcpy(myip, config_get_string_value(config, "IP"));
	}
	strcpy(algoritmo,config_get_string_value(config, "ALGORITMO"));
	port = config_get_int_value(config, "PORT");
	space = config_get_int_value(config, "DISK_SPACE");
	retardo = config_get_int_value(config, "RETARDO");
	return;
}

void depuracion(int senial)
{
	int i;
	t_info_programa *prog;
	switch(senial)
	{
	case SIGINT:
		endwin();
		log_destroy(logger);
		exit(0);
		break;
	}
}

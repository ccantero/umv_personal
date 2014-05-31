/*
 * umv.c
 *
 *  Created on: 13/05/2014
 *      Author: utnso
 */


#include "umv.h"

int main(int argc, char *argv[])
{
	logger = log_create("Log.txt", "UMV", false, LOG_LEVEL_INFO);
	GetInfoConfFile();
	pthread_t th1;
	pthread_t conexiones[MAXCONEXIONES];
	int cant_conexiones = 0;
	pthread_create(&th1, NULL, (void *)consola, NULL);
	memoria = malloc(space);
	list_programas = list_create();

	int sockfd;
	int newfd;
	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr;
	int sin_size;
	int yes = 1;
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket");
		exit(1);
	}
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
	{
		perror("setsockopt");
		exit(1);
	}
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0', 8);
	if(bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
	{
		perror("bind");
		exit(1);
	}
	if(listen(sockfd, MAXCONEXIONES) == -1)
	{
		perror("listen");
		exit(1);
	}
	while(1)
	{
		sin_size = sizeof(struct sockaddr_in);
		if((newfd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1)
		{
			perror("accept");
			continue;
		}
		printf("Se recibio una conexion desde %s\n", inet_ntoa(their_addr.sin_addr));
		printf("Contenido de newfd: %d\n", newfd);
		pthread_create(&conexiones[cant_conexiones], NULL, (void *)conexion_nueva, (void *)&newfd);
		cant_conexiones++;
	}
	pthread_join(th1, NULL);
	return 0;
}

void conexion_nueva(void *param)
{
	log_info(logger, "Se lanzo un hilo por conexion nueva.");
	int conexion;
	conexion = *((int *)param);
	int tipo_conexion;
	t_msg_handshake msg;
	recv(conexion, &msg, sizeof(t_msg_handshake), 0);
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
	recv(sock, &mensaje, sizeof(t_mensaje), 0);
	t_msg_cambio_proceso_activo msg;
	t_msg_solicitud_bytes msg2;
	t_msg_envio_bytes msg3;
	int respuesta;
	switch(mensaje.tipo)
	{
	case SOLICITUDBYTES:
		pthread_mutex_lock(&semProcesoActivo);
		recv(sock, &msg, sizeof(t_msg_cambio_proceso_activo), 0);
		proceso_activo = msg.id_programa;
		recv(sock, &msg2, sizeof(t_msg_solicitud_bytes), 0);
		respuesta = atender_solicitud_bytes(msg2.base, msg2.offset, msg2.tamanio, sock, NULL);
		if (respuesta != 0)
		{
			mensaje.tipo = respuesta;
			mensaje.id_proceso = proceso_activo;
			send(sock, &mensaje, sizeof(t_mensaje), 0);
		}
		pthread_mutex_unlock(&semProcesoActivo);
		break;
	case ENVIOBYTES:
		pthread_mutex_lock(&semProcesoActivo);
		recv(sock, &msg, sizeof(t_msg_cambio_proceso_activo), 0);
		proceso_activo = msg.id_programa;
		recv(sock, &msg3, sizeof(t_msg_envio_bytes), 0);
		respuesta = atender_envio_bytes(msg3.base, msg3.offset, msg3.tamanio, sock);
		if (respuesta != 0)
		{
			mensaje.tipo = respuesta;
			mensaje.id_proceso = proceso_activo;
			send(sock, &mensaje, sizeof(t_mensaje), 0);
		}
		pthread_mutex_unlock(&semProcesoActivo);
		break;
	}
}

int atender_envio_bytes(int base, int offset, int tam, int sock)
{
	char buffer[tam];
	if (sock != 0)
	{
		recv(sock, &buffer, sizeof(buffer), 0);
	}
	else
	{
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
			return SEGMENTO_INVALIDO;
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
			if (offset > seg->tamanio && ((seg->tamanio - offset) >= tam))
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
			return SEGMENTO_INVALIDO;
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
		*buffer = (char *)malloc(tam + 1);
		memcpy(*buffer, &memoria[seg->dirFisica + offset], tam);
		return (seg->dirFisica + offset);
	}
	return 0;
}

void atender_kernel(int sock)
{
	t_mensaje mensaje;
	recv(sock, &mensaje, sizeof(t_mensaje), 0);
	t_msg_destruir_segmentos msg;
	t_msg_crear_segmento msg2;
	t_msg_envio_bytes msg3;
	int respuesta;
	switch(mensaje.tipo)
	{
	case DESTRUIRSEGMENTOS:
		pthread_mutex_lock(&semCompactacion);
		recv(sock, &msg, sizeof(t_msg_destruir_segmentos), 0);
		mensaje.datosNumericos = destruir_segmentos(msg.id_programa);
		pthread_mutex_unlock(&semCompactacion);
		send(sock, &mensaje, sizeof(t_mensaje), 0);
		break;
	case CREARSEGMENTO:
		pthread_mutex_lock(&semCompactacion);
		recv(sock, &msg2, sizeof(t_msg_crear_segmento), 0);
		mensaje.datosNumericos = crear_segmento(msg2.id_programa, msg2.tamanio);
		pthread_mutex_unlock(&semCompactacion);
		mensaje.id_proceso = UMV;
		send(sock, &mensaje, sizeof(t_mensaje), 0);
		break;
	case ENVIOBYTES:
		pthread_mutex_lock(&semProcesoActivo);
		recv(sock, &msg, sizeof(t_msg_cambio_proceso_activo), 0);
		proceso_activo = msg.id_programa;
		recv(sock, &msg3, sizeof(t_msg_envio_bytes), 0);
		respuesta = atender_envio_bytes(msg3.base, msg3.offset, msg3.tamanio, sock);
		if (respuesta != 0)
		{
			mensaje.tipo = respuesta;
			mensaje.id_proceso = proceso_activo;
			send(sock, &mensaje, sizeof(t_mensaje), 0);
		}
		pthread_mutex_unlock(&semProcesoActivo);
		break;
	}
}

int hay_espacio_en_memoria(int tam)
{
	int mem[space];
	int i;
	int j;
	int k;
	t_info_programa *prog;
	t_info_segmento *seg;
	int espacio_libre = 0;
	for (i = 0; i < space; i++)
	{
		mem[i] = 0;
	}
	for (i = 0; i < list_size(list_programas); i++)
	{
		prog = list_get(list_programas, i);
		for (j = 0; j < list_size(prog->segmentos); j++)
		{
			seg = list_get(prog->segmentos, j);
			for (k = 0; k < seg->tamanio; k++)
			{
				mem[k + seg->dirFisica] = 1;
			}
		}
	}
	for (i = 0; i < space; i++)
	{
		if (mem[i] == 0)
		{
			espacio_libre++;
			if (espacio_libre == tam)
			{
				return 1;
			}
		}
		else
		{
			espacio_libre = 0;
		}
	}
	return 0;
}

int asignar_direccion_logica(int tamanio)
{

	return 0;
}

int asignar_direccion_en_memoria(int tamanio)
{
	int dir;
	if (obtener_cant_segmentos() == 0)
	{
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
			dir = asignar_direccion_wf(tamanio);
			return dir;
		}
		else
		{
			// El algoritmo es first-fit
			dir = asignar_direccion_ff(tamanio);
			return dir;
		}
	}
}

int asignar_direccion_wf(int tamanio)
{
	int espacio_mas_grande = 0;
	int base_segmento;
	int limite_segmento;
	int ultimo_limite;
	int ultima_direccion = 0;
	int primer_direccion;
	int i;
	for (i = 0; i < obtener_cant_segmentos(); i++)
	{
		base_segmento = obtener_base_segmento(i, 0);
		limite_segmento = obtener_limite_segmento(i, 0);
		if (ultima_direccion == 0)
		{
			if (base_segmento - ultima_direccion > espacio_mas_grande)
			{
				espacio_mas_grande = base_segmento - ultima_direccion;
				primer_direccion = ultima_direccion;
				ultima_direccion = base_segmento - 1;
				ultimo_limite = limite_segmento;
			}
		}
		else
		{
			if (base_segmento - ultimo_limite - 1 > espacio_mas_grande)
			{
				espacio_mas_grande = base_segmento - ultimo_limite - 1;
				primer_direccion = ultimo_limite + 1;
				ultima_direccion = base_segmento - 1;
				ultimo_limite = limite_segmento;
			}
		}
	}
	if (space - 1 - ultimo_limite - 1 > espacio_mas_grande)
	{
		espacio_mas_grande = space - 1 - ultimo_limite - 1;
		primer_direccion = ultimo_limite + 1;
		ultima_direccion = space - 1;
	}
	return primer_direccion;
}

int obtener_base_segmento(int index, int arranque)
{
	int ultima_direccion = obtener_direccion_segmento(arranque);
	if (index == 1)
	{
		return ultima_direccion;
	}
	else
	{
		ultima_direccion = obtener_base_segmento(index - 1, ultima_direccion + 1);
	}
	return ultima_direccion;
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

int obtener_limite_segmento(int index, int arranque)
{
	int ultima_direccion = obtener_direccion_mas_offset_segmento(arranque);
	if (index == 1)
	{
		return ultima_direccion;
	}
	else
	{
		ultima_direccion = obtener_limite_segmento(index - 1, ultima_direccion + 1);
	}
	return ultima_direccion;
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
				direccion_mas_offset = segm->dirFisica + segm->tamanio;
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
	while (salir != 0)
	{
		for (i = 0; i < list_size(list_programas); i++)
		{
			prog = list_get(list_programas, i);
			for (j = 0; j < list_size(prog->segmentos); j++)
			{
				segm = list_get(prog->segmentos, j);
				if (segm->dirFisica < primer_direccion && segm->dirFisica > direccion_arranque)
				{
					primer_direccion = segm->dirFisica;
					ultima_direccion = segm->dirFisica + segm->tamanio;
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
			if (primer_direccion - direccion_arranque > tamanio)
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
	initscr();
	echo();
	scrollok(stdscr, TRUE);
	char comando[100];
	char comando2[100];
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
   					respuesta = atender_solicitud_bytes(valor_numerico, valor_numerico2, valor_numerico3, 0, &buffer);
   					switch(respuesta)
   					{
   					case PROGRAMA_INVALIDO:
   						printw("El proceso es invalido.\n");
   						refresh();
   						break;
   					case SEGMENTO_INVALIDO:
   						printw("El segmento es invalido.\n");
   						refresh();
   						break;
   					case SEGMENTATION_FAULT:
   						printw("Segmentation fault.\n");
   						refresh();
   						break;
   					default:
   						printw("Posicion de memoria solicitada: %d\n", respuesta);
   						printw("Contenido: %s\n", buffer);
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
   					respuesta = atender_envio_bytes(valor_numerico, valor_numerico2, valor_numerico3, 0);
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
   					case SEGMENTO_INVALIDO:
   						printw("El segmento es invalido.\n");
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
   			compactar_memoria();
   		   	flag_comandoOK = 1;
   		}
   		if (flag_comandoOK == 0 && strcmp(comando, "dump") == 0)
   		{
   			pthread_mutex_lock(&semCompactacion);
   			dump_memoria();
   			pthread_mutex_unlock(&semCompactacion);
   		   	flag_comandoOK = 1;
   		}
   		if (flag_comandoOK == 0 && strcmp(comando, "help") == 0)
   		{
   			printw("Los unicos comandos habilitados son: \n");
   			printw("\toperacion\n");
   			printw("\tretardo\n");
   			printw("\talgoritmo\n");
   			printw("\tcompactacion\n");
   			printw("\tdump\n");
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
   			printw("\tdump\n");
   			refresh();
   		}
   	}
}

void dump_memoria()
{
	int mem_utilizada = 0;
	clear();
	refresh();
	int x = 0;
	int y = 0;
	move(y, x);
	printw("Id Proceso\n");
	x += 15;
	move(y, x);
	printw("Dir. Fisica Segmento\n");
	x += 25;
	move(y, x);
	printw("Tamanio Segmento\n");
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
			move(y, x);
			printw("%d", seg->dirFisica);
			x += 25;
			move(y, x);
			printw("%d", seg->tamanio);
			y++;
			x = 0;
			refresh();
			mem_utilizada += seg->tamanio;
		}
	}
	y++;
	move(y, x);
	printw("Memoria total: %d\n", space);
	printw("Memoria utilizada: %d\n", mem_utilizada);
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

	return 0;
}

int transformar_direccion_en_logica(int direccion, int pid)
{

	return 0;
}

int crear_segmento(int idproc, int tamanio)
{
	int tamanio_lista = list_size(list_programas);
	t_info_programa *prog;
	t_info_segmento *seg;
	int i;
	if (tamanio_lista != 0)
	{
		// Busco al programa en mi lista de programas
		for (i = 0; i < tamanio_lista; i++)
		{
			prog = list_get(list_programas, i);
			if (prog->programa == idproc)
			{
				break;
			}
		}
		seg = (t_info_segmento *)malloc(sizeof(t_info_segmento));
		seg->id = idproc;
		seg->tamanio = tamanio;
		seg->dirFisica = asignar_direccion_en_memoria(tamanio);
		seg->dirLogica = asignar_direccion_logica(tamanio);
		list_add(prog->segmentos, seg);
		return seg->dirLogica;
	}
	else
	{
		if (hay_espacio_en_memoria(tamanio))
		{
			prog = (t_info_programa *)malloc(sizeof(t_info_programa));
			prog->programa = idproc;
			prog->segmentos = list_create();
			seg = (t_info_segmento *)malloc(sizeof(t_info_segmento));
			seg->id = idproc;
			seg->tamanio = tamanio;
			seg->dirFisica = asignar_direccion_en_memoria(tamanio);
			seg->dirLogica = asignar_direccion_logica(tamanio);
			list_add(prog->segmentos, seg);
			list_add(list_programas, prog);
			return seg->dirLogica;
		}
		else
		{
			return -1;
		}
	}
}

int destruir_segmentos(int idproc)
{
	int tamanio_lista = list_size(list_programas);
	int i;
	int indice;
	int encontre_programa = 0;
	t_info_programa *prog;
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
	pthread_mutex_lock(&semCompactacion);
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
					else
					{
						// TODO
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
			nueva_direccion = segm->tamanio + 1;
			primer_direccion = space;
			arranque = nueva_direccion;
		}
	}
	else
	{
		printw("Todavia no hay segmentos a compactar.\n");
		refresh();
	}
	pthread_mutex_unlock(&semCompactacion);
}

int obtener_cant_segmentos()
{
	int cant = 0;
	int i, j;
	t_info_programa *prog;
	t_info_segmento *segm;
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

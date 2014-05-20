/*
 * umv.c
 *
 *  Created on: 13/05/2014
 *      Author: utnso
 */


#include "umv.h"

int main(int argc, char *argv[])
{
	logger = log_create("Log.txt", "UMV",false, LOG_LEVEL_INFO);
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
	printf("Conexion: %d\n", conexion);
	int tipo_conexion;
	t_msg_handshake msg;
	recv(conexion, &msg, sizeof(t_msg_handshake), 0);
	log_info(logger, "Se recibio handshake de conexion nueva.");
	tipo_conexion = msg.tipo;
	msg.tipo = UMV;
	send(conexion, &msg, sizeof(t_msg_handshake), 0);
	log_info(logger, "Se envio handshake a conexion nueva.");
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
	char buffer[128];
	char aux[128];
	memset(buffer, '\0', 128);
	recv(sock, aux, sizeof(aux), 0);
	memcpy(buffer, aux, 128);
	int tamanio;
	tamanio = strlen(buffer);
	t_msg_cambio_proceso_activo msg;
	t_msg_solicitud_bytes msg2;
	t_msg_envio_bytes msg3;
	switch(tamanio)
	{
	case (sizeof(t_msg_cambio_proceso_activo)):
		// Recibí un tipo de mensaje cambio de proceso activo
		memcpy(&msg, buffer, sizeof(t_msg_cambio_proceso_activo));
		// Comportamiento para cambiar proceso activo
		proceso_activo = msg.id_programa;
		break;
	case (sizeof(t_msg_solicitud_bytes)):
		// Recibí un tipo de mensaje solicitud de bytes
		memcpy(&msg2, buffer, sizeof(t_msg_solicitud_bytes));
		// TODO Comportamiento cuando se recibe una solicitud de bytes

		break;
	case (sizeof(t_msg_envio_bytes) + 1):
		// Recibí un tipo de mensaje envío de bytes
		memcpy(&msg3, buffer, sizeof(t_msg_envio_bytes));
		// TODO Comportamiento cuando se recibe un envio de bytes de un cpu

		break;
	}
}

void atender_kernel(int sock)
{
	log_info(logger, "Se conecto el kernel.");
	t_mensaje mensaje;
	recv(sock, &mensaje, sizeof(t_mensaje), 0);
	/*
	char buffer[128];
	char aux[128];
	memset(buffer, '\0', 128);
	recv(sock, aux, sizeof(aux), 0);
	memcpy(buffer, aux, 128);
	int tamanio;
	tamanio = strlen(buffer);
	*/
	t_msg_destruir_segmentos msg;
	t_msg_crear_segmento msg2;
	t_msg_envio_bytes msg3;
	t_msg_cambio_proceso_activo msg4;
	t_info_programa *programa_nuevo;
	t_info_segmento *segmento_nuevo;
	t_info_programa *prog_aux;
	t_info_segmento *seg_aux;
	int tamanio_lista = list_size(list_programas);
	int i;
	int indice;
	switch(mensaje.tipo)
	{
	case DESTRUIRSEGMENTOS:
		pthread_mutex_lock(&semCompactacion);
		recv(sock, &msg, sizeof(t_msg_destruir_segmentos), 0);
		// Busco al programa en mi lista de programas
		for (i = 0; i < tamanio_lista; i++)
		{
			prog_aux = list_get(list_programas, i);
			if (prog_aux->programa == msg.id_programa)
			{
				indice = i;
				break;
			}
		}
		for (i = list_size(prog_aux->segmentos) - 1; i >= 0; i--)
		{
			list_remove(prog_aux->segmentos, i);
		}
		list_destroy(prog_aux->segmentos);
		list_remove(list_programas, indice);
		pthread_mutex_unlock(&semCompactacion);
		send(sock, &mensaje, sizeof(t_mensaje), 0);
		break;
	case CREARSEGMENTO:
		pthread_mutex_lock(&semCompactacion);
		recv(sock, &msg2, sizeof(t_msg_crear_segmento), 0);
		if (tamanio_lista != 0)
		{
			// Busco al programa en mi lista de programas
			for (i = 0; i < tamanio_lista; i++)
			{
				prog_aux = list_get(list_programas, i);
				if (prog_aux->programa == msg2.id_programa)
				{
					break;
				}
			}
			seg_aux = (t_info_segmento *)malloc(sizeof(t_info_segmento));
			seg_aux->id = msg2.id_programa;
			seg_aux->tamanio = msg2.tamanio;
			seg_aux->dirFisica = asignar_direccion_en_memoria();
			seg_aux->dirLogica = asignar_direccion_logica();
			list_add(prog_aux->segmentos, seg_aux);
		}
		else
		{
			programa_nuevo = (t_info_programa *)malloc(sizeof(t_info_programa));
			programa_nuevo->programa = msg2.id_programa;
			programa_nuevo->segmentos = list_create();
			segmento_nuevo = (t_info_segmento *)malloc(sizeof(t_info_segmento));
			segmento_nuevo->id = msg2.id_programa;
			segmento_nuevo->tamanio = msg2.tamanio;
			segmento_nuevo->dirFisica = asignar_direccion_en_memoria();
			segmento_nuevo->dirLogica = asignar_direccion_logica();
			list_add(programa_nuevo->segmentos, segmento_nuevo);
			list_add(list_programas, programa_nuevo);
		}
		pthread_mutex_unlock(&semCompactacion);
		// TODO verificar espacio en memoria y responder al kernel con la direccion logica
		// si no hay espacio devuelvo -1
		send(sock, &mensaje, sizeof(t_mensaje), 0);
		break;
	case ENVIOBYTES:
		recv(sock, &msg4, sizeof(t_msg_cambio_proceso_activo), 0);
		proceso_activo = msg4.id_programa;
		recv(sock, &msg3, sizeof(t_msg_envio_bytes), 0);
		// TODO Comportamiento cuando se reciben bytes del kernel
		send(sock, &mensaje, sizeof(t_mensaje), 0);
		break;
	}
}

int asignar_direccion_logica()
{

	return 0;
}

int asignar_direccion_en_memoria()
{
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
		}
		else
		{
			// El algoritmo es first-fit
		}
	}
	return 0;
}

void consola (void* param)
{
	char comando[100];
	char *c;
	int flag_comandoOK;
	int nuevo_valor;
	log_info(logger, "Se lanzo el hilo de consola");
	// Bucle principal esperando peticiones del usuario
	for(;;)
	{
		flag_comandoOK = 0;
    	printf("-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-\n");
    	printf("         Bienvenido a la consola de UMV           \n");
    	printf("-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-\n");
    	printf("Ingrese el comando: \n");
    	printf(">");
    	scanf("%s",comando);
    	c = strtok(comando,"\n");
   		if (flag_comandoOK == 0 && strncmp(c,"operacion",strlen("operacion"))==0 )
   		{
   			printf("Llamar a funcion operacion\n");
   			flag_comandoOK = 1;
   		}
   		if (flag_comandoOK == 0 && strncmp(c,"retardo",strlen("retardo"))==0)
   		{
   			log_info(logger, "Se cambio el valor de retardo por consola.");
   			printf("Nuevo valor de retardo: ");
   			scanf("%d", &nuevo_valor);
   			printf("Valor anterior de retardo: %d\n", retardo);
   			cambiar_retardo(nuevo_valor);
   			printf("Valor actual de retardo: %d\n", retardo);
   			flag_comandoOK = 1;
   		}
   		if (flag_comandoOK == 0 && strncmp(c,"algoritmo",strlen("algoritmo"))==0)
   		{
   			log_info(logger, "Se cambio el algoritmo por consola.");
   			printf("Se cambio el algoritmo.\n");
   			printf("Algoritmo anterior: %s\n", algoritmo);
   			cambiar_algoritmo();
   			printf("Algoritmo actual: %s\n", algoritmo);
   		   	flag_comandoOK = 1;
   		}
   		if (flag_comandoOK == 0 && strncmp(c,"compactacion",strlen("compactacion"))==0)
   		{
   			log_info(logger, "Se pidio compactar memoria por consola.");
   			compactar_memoria();
   		   	flag_comandoOK = 1;
   		}
   		if (flag_comandoOK == 0 && strncmp(c,"dump",strlen("dump"))==0)
   		{
   			printf("Llamar a funcion dump\n");
   		   	flag_comandoOK = 1;
   		}
   		if(flag_comandoOK == 0)
   		{
   			printf("Por favor verifique la sintaxis de los comandos utilizados\n");
   			printf("Los unicos comandos habilitados son: \n");
   			printf("\toperacion\n");
   			printf("\tretardo\n");
   			printf("\talgoritmo\n");
   			printf("\tcompactacion\n");
   			printf("\tdump\n");
   		}
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
		// TODO Reflejar cambios en "memoria"
		printf("Compactando...\n");
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
			segm->dirFisica = nueva_direccion;
			nueva_direccion = segm->tamanio + 1;
			primer_direccion = space;
			arranque = nueva_direccion;
		}
	}
	else
	{
		printf("Todavia no hay segmentos a compactar.\n");
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

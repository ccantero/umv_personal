/*
 * umv.h
 *
 *  Created on: 13/05/2014
 *      Author: utnso
 */

#ifndef UMV_H_
#define UMV_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include <src/silverstack.h>

#define MAXCONEXIONES 25
#define PATH_CONFIG "../conf"

typedef struct {
	int id;
	int dirLogica;
	int dirFisica;
	int tamanio;
} t_info_segmento;

typedef struct {
	int programa;
	t_list *segmentos;
} t_info_programa;

t_log *logger;
t_list *list_programas;
t_dictionary *dic_cpus;

pthread_mutex_t semRetardo = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t semAlgoritmo = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t semCompactacion = PTHREAD_MUTEX_INITIALIZER;

int sockPrin;
int space;
int socketKernel;
char hostip[16];
char algoritmo[16];
int *memoria;
char *myip;
int port;
int proceso_activo;
int retardo;

void consola (void* param);
void GetInfoConfFile(void);
void conexion_nueva(void *param);
void atender_kernel(int sock);
void atender_cpu(int sock);
int asignar_direccion_en_memoria();
int asignar_direccion_logica();
void compactar_memoria();
int obtener_cant_segmentos();
void cambiar_retardo(int valor);
void cambiar_algoritmo();

#endif /* UMV_H_ */

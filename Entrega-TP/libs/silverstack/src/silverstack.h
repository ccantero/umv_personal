/*
 * silverstack.h
 *
 *  Created on: 28/04/2014
 *      Author: utnso
 */

#ifndef SILVERSTACK_H_
#define SILVERSTACK_H_

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>  /* Semaphore */
#include <parser/metadata_program.h>
#include <commons/collections/queue.h>

#define SENDFILE 104
#define HANDSHAKE 100
#define HANDSHAKEOK 101
#define SALIR 110 // Programa
#define CPU 200
#define UMV 201
#define KERNEL 202
#define PROGRAMA 203
#define QUANTUMFINISH 301
#define PROGRAMFINISH 302
#define ASIGNACION 306
#define IMPRIMIR 307
#define IMPRIMIRTEXTO 308
#define VARCOMREQUEST 309
#define ENTRADASALIDA 310
#define SIGNALSEM 311
#define WAITSEM 312
#define NEW_PROGRAM_REQUEST 401
#define NEW_PROGRAMOK 402
#define STACK_SIZE 403
#define LOW_MEMORY 404
#define TAMANIO_REQUEST 405
#define TAMANIOOK 406
#define STACKREQUEST 407
#define STACKOK 408
#define INSTRUCTIONREQUEST 409
#define INSTRUCTIONREQUESTOK 410
#define ETIQUETASREQUEST 409
#define ETIQUETASREQUESTOK 410
#define SOLICITUDBYTES 500
#define ENVIOBYTES 501
#define CAMBIOPROCACT 502
#define CREARSEGMENTO 503
#define DESTRUIRSEGMENTOS 504
#define ERROR 999
#define BLOCK 998
#define SEGMENTATION_FAULT 1001

// Estructura de mensaje global para usar entre kernel, programas y cpu's
typedef struct {
	int tipo;
	int id_proceso;
	int datosNumericos;
	char mensaje[16];
} t_mensaje;


// ############################## INICIO Estructuras de mensajes con la UMV ##############################
typedef struct {
	int base;
	int offset;
	int tamanio;
} t_msg_solicitud_bytes;

typedef struct {
	int base;
	int offset;
	int tamanio;
} t_msg_envio_bytes;

typedef struct {
	int tipo;
} t_msg_handshake;

typedef struct {
	int id_programa;
} t_msg_cambio_proceso_activo;

typedef struct {
	int id_programa;
	int tamanio;
} t_msg_crear_segmento;

typedef struct {
	int id_programa;
} t_msg_destruir_segmentos;
// ############################### FIN Estructuras de mensajes con la UMV ###############################



typedef struct _hdr {
	char desc_id[16];
	unsigned char pay_desc;
	int pay_len;
} thdr;

#define SIZE_HDR sizeof(thdr)

typedef struct _io {
	char* name;
	int retardo;
	t_queue *io_queue;
	sem_t io_sem;
	pthread_t* th_io;
} t_io;

typedef struct _t_semaphore {
	char* identifier;
	int	value;
	t_queue* queue;
} t_semaphore;

typedef struct _instruction_index {
	t_size size;
	t_intructions* index;
} t_instruction_index;

typedef struct _etiquetas_index {
	t_size size;
	char* etiquetas;
} t_etiquetas_index;

typedef struct _t_segment {
	unsigned char code_identifier;
	int start;
	size_t offset;
} t_segment;

typedef struct _t_nodo_segment {
	int start;
	size_t offset;
} t_nodo_segment;

typedef struct _t_global {
	char* identifier;
	int	value;
} t_global;

typedef struct _t_nodo_cpu {
	int	socket;
	unsigned char status;
}t_nodo_cpu;

typedef struct _pcb {
	unsigned int unique_id; 		/* Identificador Único */
	int code_segment; 				/* Direccion Logica del comienzo del Segmento Código Ansisop */
	int stack_segment; 				/* Direccion Logica del comienzo del Segmento Stack */
	int stack_pointer; 				/* Direccion Logica del puntero actual dentro del Segmento Stack */
	int instruction_index; 			/* Direccion Logica del comienzo del Segmento Indice de Instrucciones */
	int etiquetas_index; 			/* Direccion Logica del comienzo del Segmento Indice de Etiquetas */
	int size_etiquetas_index; 	/* Tamaño del Indice de Etiquetas */
	int program_counter; 			/* Número de la próxima instrucción */
	int context_actual;				/* Tamaño del Contexto Actual */
	int peso;
	int instruction_size;
} t_pcb;


#endif /* SILVERSTACK_H_ */

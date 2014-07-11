/*
 * protocol.h
 *
 *  Created on: 26/04/2014
 *      Author: utnso
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include <stdio.h> // printf
#include <stdlib.h>  // atoi
#include <string.h> //strcpy, strtok, strcmp
#include <sys/types.h> // read
#include <unistd.h> // pause
#include <pthread.h> //
#include <fcntl.h> // O_RDONLY
#include <sys/socket.h> // socket, connect
#include <arpa/inet.h> // struct sockaddr_in>
#include <time.h> // time()
#include <signal.h> // SIGINT
#include <semaphore.h>  /* Semaphore */
#include <commons/log.h> // log_create, log_info, log_error
#include <commons/config.h> // config_get_int_value
#include <commons/collections/queue.h> // queue_create
#include <parser/metadata_program.h>
#include <src/silverstack.h>

typedef struct _t_process {
	unsigned int pid;
	int program_socket;
	int current_cpu_socket;
	unsigned char status;
	unsigned char blocked_status;
	int error_status;
	time_t t_inicial;
	time_t t_final;
} t_process;

typedef struct _t_io_queue_nodo {
	unsigned int pcb;
	int retardo;
} t_io_queue_nodo;

typedef struct _t_nodo_queue_semaphore {
	int process_id;
}t_nodo_queue_semaphore;

typedef struct _t_pedido {
	int process_id;
	unsigned char previous_status;
	unsigned char new_status;
}t_pedido;

typedef struct _t_nodo_blocked_io {
	sem_t* semaforo;
} t_nodo_blocked_io;

#define MAXDATASIZE 1024
#define SIZE_MSG sizeof(t_mensaje)
#define MSG_CON_UMV 0x10
#define MSG_CON_UMV_OK 0x11
#define MSG_CON_UMV_FAIL 0x12
#define CODE_SEGMENT 0x20
#define STACK_SEGMENT 0x21

#define backlog 10
#define CPU_AVAILABLE 0x30 // CPU Node Status // Ready
#define CPU_IDLE 0x31 // CPU Node Status // Not working because multiprogramacion
#define CPU_WORKING 0x32 // CPU Node Status
#define PROCESS_NEW 0x40 // Process Node Status
#define PROCESS_READY 0x41 // Process Node Status
#define PROCESS_EXECUTE 0x42 // Process Node Status
#define PROCESS_BLOCKED 0x43 // Process Node Status
#define PROCESS_EXIT 0x44 // Process Node Status
#define PROCESS_ERROR 0x45 // Process Node Status

#define NOT_BLOCKED 0x50 // Process Node Status
#define BLOCKED_IO 0x51 // Process Node Status
#define BLOCKED_SEM 0x52 // Process Node Status

#define ERROR_WRONG_VARCOM 2001
#define ERROR_WRONG_IO 2002
#define ERROR_PROGRAM_ABORT 2003

t_log *logger;
t_list *list_io;
t_list *list_pcb_new;
t_list *list_pcb_ready;
t_list *list_pcb_execute;
t_list *list_pcb_blocked;
t_list *list_pcb_exit;
t_list *list_segment;
t_list *list_semaphores;
t_list *list_globales;
t_list *list_cpu;
t_list *list_process;

t_queue* queue_rr;
t_queue* queue_blocked;

int port_cpu,port_program,port_umv,sockPrin,multiprogramacion,quantum,retardo,stack_tamanio;
int sock_umv, process_Id, cantidad_cpu, cantidad_procesos_sistema, stack_size;
char myip[16],umv_ip[16];
sem_t free_io_queue;

pthread_mutex_t mutex_pedidos;
pthread_mutex_t mutex_new_queue;
pthread_mutex_t mutex_ready_queue;
pthread_mutex_t mutex_execute_queue;
pthread_mutex_t mutex_block_queue;
pthread_mutex_t mutex_exit_queue;
pthread_mutex_t mutex_semaphores_list;
pthread_mutex_t mutex_process_list;
pthread_mutex_t mutex_blocked_queue;

sem_t sem_consola;
sem_t sem_consola_ready;
sem_t sem_plp;
sem_t sem_pcp;
sem_t mutex_cpu_list;
sem_t sem_cpu_list;

int exit_status;

void GetInfoConfFile(char* PATH_CONFIG);
int umv_connect(void);
t_global* global_create(char *global_name);
void global_update_value(int sock_cpu, char* global_name, int value);
void global_get_value(int sock_cpu, char* global_name);
t_io* io_create(char *io_name, int io_retardo);
t_semaphore* semaphore_create(char* sem_name, int value);
void semaphore_wait(int sock_cpu, char* sem_name);
void semaphore_signal(int sock_cpu, char* sem_name);
int escuchar_Nuevo_Programa(int sock_program);
void pcb_create(char* buffer, int tamanio_buffer, int sock_program);
int umv_create_segment(int process_id, int tamanio);
int umv_change_process(int process_id);
int umv_send_bytes(int base, int offset, int tamanio);
void finalizo_Quantum(int sock_cpu);
void pcb_update(t_pcb* pcb,unsigned char actual_state);
void sort_plp(void);
void planificador_sjn(void);
void mostrar_consola(void);
void mostrar_procesos(void);
int is_Connected_CPU(int socket);
int escuchar_Nuevo_cpu(int sock_cpu);
int escuchar_cpu(int sock_cpu);
t_nodo_cpu* cpu_create(int socket);
void cpu_remove(int socket);
t_process* process_create(unsigned int pid, int socket);
void process_update(int pid, unsigned char previous_status, unsigned char next_status);
void process_destroy(t_process *p);
void pcb_move(unsigned int pid,t_list* from, t_list* to);
void io_wait(int sock_cpu, char* io_name, int amount);
t_io_queue_nodo* io_queue_create(unsigned int process_id, int retardo);
void retardo_io(void *ptr);
void found_cpus_available(void);
void depurar(int signum);
int umv_send_segment(int pid, char* buffer, int tamanio);
int servidor_Programa(void);
int servidor_CPU(void);
int buscar_Mayor(int a, int b, int c);
int is_Connected_Program(int sock_program);
void planificador_rr(void);
void process_execute(int unique_id, int cpu_socket);
void pcb_destroy(t_pcb* self);
void umv_destroy_segment(int process_id);
int get_process_id_by_sock_cpu(int sock_cpu);
void program_exit(int pid);
t_pedido* pedido_create(int pid, unsigned char previous_status, unsigned char next_status);
void process_finish(int sock_cpu);
void imprimirTexto(int sock_cpu,int valor);
void imprimir(int sock_cpu,int valor);
int get_sock_prog_by_sock_cpu(int sock_cpu);
void cpu_set_status(int socket, unsigned char status);
unsigned char process_get_status(int process_id);
void process_set_status(int process_id, unsigned char status);
t_process* process_get(int pid);
t_nodo_cpu* cpu_get_next_available(int pid);
void process_segmentation_fault(int sock_cpu);
void program_error(int sock_program);
t_nodo_blocked_io* nodo_blocked_io_create(sem_t* semaforo);

int get_Segment_Start(int offset);					// A revisar si va o no va
void io_destroy(t_io*); 							// A revisar si va o no va
void semaphore_destroy(t_semaphore *self);			// A revisar si va o no va
int send_umv_stack(int process_id);					// A revisar si va o no va
void fd_set_cpu_sockets(fd_set* descriptores);		// A revisar si va o no va
void fd_set_program_sockets(fd_set* descriptores);	// A revisar si va o no va
void test_pcb(int pid, unsigned char previous_status);
void asignar_valor_VariableCompartida(int sock_cpu, char* global_name, int value);
void calcularTiempo(void);
struct tm* timeConvert(double seconds);

#endif /* PROTOCOL_H_ */

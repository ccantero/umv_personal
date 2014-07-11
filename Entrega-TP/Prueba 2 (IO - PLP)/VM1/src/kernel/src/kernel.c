/*
 * kernel.c
 *
 *  Created on: 26/04/2014
 *  Author: SilverStack
 *	Compiler Include: Path to commons lib
 *  Linker Command: gcc -lpthread -pthread
 *  Linker Library: -commons
 */

/* Header File */

#include "protocol.h"
#include <errno.h>

int main(int argc, char *argv[])
{
	int i,fdmax; /* for aux */
	//TODO: Si no existe variable compartida no morir
	int sock_program = 0, sock_cpu = 0;
	int new_socket;
	fd_set read_fds;
	fd_set master;

	pthread_t th_plp;
	pthread_t th_pcp;
	pthread_t th_consola;

	if(argc != 2)
	{
		printf("ERROR, la sintaxis de kernel es: ./kernel archivo_configuracion \n");
		return -1;
	}

	//logger = log_create("Log.txt", "Kernel",false, LOG_LEVEL_INFO);
	logger = log_create("Log.txt", "Kernel",false, LOG_LEVEL_DEBUG);

	list_io = list_create();
	list_segment = list_create();
	list_semaphores = list_create();
	list_pcb_new = list_create();
	list_pcb_ready = list_create();
	list_pcb_execute = list_create();
	list_pcb_blocked = list_create();
	list_globales = list_create();
	list_pcb_exit = list_create();
	list_cpu = list_create();
	list_process = list_create();

	queue_rr = queue_create();
	queue_blocked = queue_create();

	process_Id = 10000;
	cantidad_cpu = 0;
	cantidad_procesos_sistema = 0;

	sem_init(&free_io_queue, 0, 1);

	sem_init(&sem_plp, 0, 0); // Empieza en cero porque tiene que bloquearse hasta que aparezca algo
	sem_init(&sem_pcp, 0, 0); // Empieza en cero porque tiene que bloquearse hasta que aparezca algo
	sem_init(&mutex_cpu_list, 0, 1);

	sem_init(&sem_consola,0,0); //arranca en 0 hasta que haya algun proceso para mostrar
	sem_init(&sem_consola_ready,0,0); //arranca en 0 para que muestre el new antes de hacer process update

	if (pthread_mutex_init(&mutex_pedidos, NULL) != 0)
	{
		printf("ERROR - No se pudo inicializar mutex mutex_pedidos\n");
		return 1;
	}

	if (pthread_mutex_init(&mutex_new_queue, NULL) != 0)
	{
		printf("ERROR - No se pudo inicializar mutex mutex_new_queue\n");
		return 1;
	}

	if (pthread_mutex_init(&mutex_ready_queue, NULL) != 0)
	{
		printf("ERROR - No se pudo inicializar mutex mutex_ready_queue\n");
		return 1;
	}

	if (pthread_mutex_init(&mutex_execute_queue, NULL) != 0)
	{
		printf("ERROR - No se pudo inicializar mutex mutex_execute_queue\n");
		return 1;
	}

	if (pthread_mutex_init(&mutex_block_queue, NULL) != 0)
	{
		printf("ERROR - No se pudo inicializar mutex mutex_block_queue\n");
		return 1;
	}

	if (pthread_mutex_init(&mutex_exit_queue, NULL) != 0)
	{
		printf("ERROR - No se pudo inicializar mutex mutex_exit_queue\n");
		return 1;
	}

	if (pthread_mutex_init(&mutex_semaphores_list, NULL) != 0)
	{
		printf("ERROR - No se pudo inicializar mutex mutex_semaphores_list\n");
		return 1;
	}

	if (pthread_mutex_init(&mutex_process_list, NULL) != 0)
	{
		printf("ERROR - No se pudo inicializar mutex mutex_process_list\n");
		return 1;
	}

	if (pthread_mutex_init(&mutex_blocked_queue, NULL) != 0)
	{
		printf("ERROR - No se pudo inicializar mutex mutex_blocked_queue\n");
		return 1;
	}

	GetInfoConfFile(argv[1]);

	signal(SIGINT,depurar);

	pthread_create(&th_plp,NULL,(void*)planificador_sjn,NULL);
	pthread_create(&th_pcp,NULL,(void*)planificador_rr,NULL);
	pthread_create(&th_consola,NULL,(void*)mostrar_consola,NULL);

	sock_umv = umv_connect();
	sock_cpu = servidor_CPU();
	sock_program = servidor_Programa();

	if(sock_umv == -1 || sock_program == -1 || sock_cpu == -1)
	{
		printf("ERROR, Alguno de los sockets principales no pudo iniciar. \n");
		printf("ERROR, sock_umv = %d \n", sock_umv);
		printf("ERROR, sock_program = %d \n", sock_program);
		printf("ERROR, sock_cpu = %d \n", sock_cpu);
		return -1;
	}

	FD_ZERO (&master);
	FD_ZERO (&read_fds);

	FD_SET (sock_program, &master);
	FD_SET (sock_umv, &master);
	FD_SET (sock_cpu, &master);

	fdmax = buscar_Mayor(sock_program, sock_umv, sock_cpu);

	exit_status = 1;

	while(exit_status == 1)
	{
		read_fds = master;

		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
		{
			log_error(logger, "Error en funcion select");
			log_error(logger,"errno = %d", errno);
			log_error(logger,"strerror = %s", strerror(errno));
			exit(1);
		}

		for (i = 0; i <= fdmax; i++)
		{
			if (FD_ISSET(i, &read_fds))
			{
				if(i == sock_umv)
				{
					log_error(logger,"Se cayo la UMV");
					close(sock_umv);
					//exit_status = 0;
					//break;
				}

				if (i == sock_program)
				{
					new_socket = escuchar_Nuevo_Programa(sock_program);
					if(new_socket == -1)
					{
						continue;
					}

					FD_SET(new_socket, &master);
					if(fdmax < new_socket)
						fdmax = new_socket;

					continue;
				}

				if (i == sock_cpu)
				{
					new_socket = escuchar_Nuevo_cpu(i);

					if(new_socket == -1)
					{
						log_error(logger,"No se pudo agregar una cpu");
						continue;
					}

					FD_SET(new_socket, &master);
					cantidad_cpu++;
					sem_wait(&mutex_cpu_list);
					list_add(list_cpu, cpu_create(new_socket));
					sem_post(&mutex_cpu_list);

					log_info(logger,"[CPU] - Se agrego un cpu a la lista de CPU socket %d", new_socket);

					if(cantidad_cpu < multiprogramacion)
					{
						sem_post(&sem_cpu_list); // Hay un nuevo CPU Disponible
					}

					if(fdmax < new_socket)
						fdmax = new_socket;

					continue;
				}

				if(is_Connected_Program(i) == 0)
				{
					// El programa se cerró.
					program_error(i);
					FD_CLR(i, &master);
					close(i);
					log_info(logger,"Se cerro el programa.");
					continue;
				}

				if(is_Connected_CPU(i) != 0)
				{
					if(escuchar_cpu(i) == -1)
					{
						close(i);
						FD_CLR(i, &master);
						cpu_remove(i);
						sem_wait(&sem_cpu_list); // Hay un CPU Disponible menos
						log_info(logger,"[CPU] - Se removio un cpu de la lista de CPU");
						continue;
					}
					continue;
				}

				close(i);
				FD_CLR(i, &master);
			}
		} /* for (i = 0; i <= fdmax; i++) */
	}/* for(;;) */

	log_destroy(logger);
	exit(0);
	return 1;
}

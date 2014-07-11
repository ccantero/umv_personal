/*
 * programa.c
 *
 *  Created on: 30/04/2014
 *      Author: utnso
 */
#include "programa.h"

int main (int argc, char *argv[])
{
	signal(SIGINT, depuracion);
	// Creo el logger
	char *nombreArchivo;
	char nombre_archivo_log[25];
	strcpy(nombre_archivo_log, argv[1]);
	nombreArchivo = argv[1];
	char *nombre_log = strcat(nombre_archivo_log, ".log");
	logger = log_create(nombre_log, argv[1], true, LOG_LEVEL_INFO);
	// recibo nombre del archivo a procesar
	log_info(logger, "Se abrio el archivo %s", nombreArchivo);
	// leo archivo configuracion
	config = config_create(getenv("ANSISOP_CONFIG"));
	//defino algunas variables
	int puerto;
	char *direccionIp = (char *)malloc(16);
	char buf[256];
	int numBytes;
	//veo ip y puerto
	puerto = config_get_int_value(config, "PUERTO");
	direccionIp = config_get_string_value(config, "IP");
	//creo socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		log_error(logger,"Error cuando se crea el socket.");
		exit(1);
	}
	//asigno el socket al kernel
	struct sockaddr_in their_addr;
	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(puerto);
	their_addr.sin_addr.s_addr = inet_addr(direccionIp);
	memset(&(their_addr.sin_zero), '\0', 8);
	// Conecto el socket y compruebo errores
	if (connect(sockfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1)
	{
		log_error(logger,"Error conectando el socket");
		exit(1);
	}
	t_mensaje msg;
	msg.tipo = HANDSHAKE;
	msg.id_proceso = PROGRAMA;
	strcpy(msg.mensaje,"Hola Kernel!!");
	send(sockfd, &msg, sizeof(msg), 0);
	// Respuesta handshake y compruebo errores
	if ((numBytes = recv(sockfd, &msg, sizeof(msg), 0)) <= 0)
	{
		log_error(logger,"Error recibiendo mensaje.");
		exit(1);
	}
	//imprimo respuesta handshake
	struct stat stat_file;
	//envio codigo ansisop
	stat(nombreArchivo,&stat_file);
	file = fopen (nombreArchivo,"r");
	char* buffer = (char*) malloc(sizeof(char) * (stat_file.st_size +1));
	if (file==NULL)
	{
		log_error(logger,"Error de apertura de archivo");
		exit(1);
	}
	fread(buffer, stat_file.st_size,1, file);
	msg.tipo = SENDFILE;
	msg.id_proceso = PROGRAMA;
	msg.datosNumericos = stat_file.st_size;
	send(sockfd, &msg, sizeof(t_mensaje), 0);
	send(sockfd, buffer, stat_file.st_size, 0);
	while (1)
	{
		// Recibo y me fijo si hay errores
		if ((numBytes = recv(sockfd, &msg, sizeof(t_mensaje), 0)) <= 0)
		{
			log_error(logger,"Error recibiendo mensaje.");
			exit(1);
		}
		switch(msg.tipo)
		{
			case IMPRIMIR: printf("%d\n", msg.datosNumericos);break;
			case IMPRIMIRTEXTO:
			{
				//recibo texto y me fijo si tiene error
				if ((numBytes = recv(sockfd, &buf, msg.datosNumericos, 0)) <= 0)
				{
					log_error(logger,"Error recibiendo mensaje.");
					exit(1);
				}
				buf[msg.datosNumericos] = '\0';
				//imprimo texto
				printf("%s", buf);
				break;
			}
			case SALIR:
			{
				printf("Finalizo Ejecucion de Programa\n");
				printf("%s\n", msg.mensaje);
				exit(1);
			}
		}
	}
	//cierro archivo
	fclose(file);
	// Cierro el descriptor del socket
	close(sockfd);
	// Libero memoria
	free(direccionIp);
	config_destroy(config);
	return 0;
}

// Funciones
void depuracion(int senial)
{
	switch(senial)
	{
	case SIGINT:
		log_info(logger, "Inicia depuracion de variables y memoria.");
		fclose(file);
		close(sockfd);
		config_destroy(config);
		log_destroy(logger);
		exit(0);
		break;
	}
}

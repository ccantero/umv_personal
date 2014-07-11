/*
 * programa.h
 *
 *  Created on: 30/04/2014
 *      Author: utnso
 */

#ifndef PROGRAMA_H_
#define PROGRAMA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <commons/config.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <src/silverstack.h>
#include <signal.h>

#define SALIR 110

t_config *config;
t_log *logger;
int sockfd;
FILE *file;

void depuracion(int senial);

#endif /* PROGRAMA_H_ */

/*
 * cpu.c
 *
 *  Created on: 23/04/2014
 *      Author: utnso
 */


#include "cpu.h"

// Defino las primitivas que vamos a usar, cada una está definida mas abajo con el resto de las funciones
AnSISOP_funciones primitivas = {
		.AnSISOP_definirVariable		 = silverstack_definirVariable,
		.AnSISOP_obtenerPosicionVariable = silverstack_obtenerPosicionVariable,
		.AnSISOP_dereferenciar			 = silverstack_dereferenciar,
		.AnSISOP_asignar				 = silverstack_asignar,
		.AnSISOP_imprimir				 = silverstack_imprimir,
		.AnSISOP_imprimirTexto			 = silverstack_imprimirTexto,
		.AnSISOP_obtenerValorCompartida  = silverstack_obtenerValorCompartida,
		.AnSISOP_entradaSalida           = silverstack_entradaSalida,
		.AnSISOP_finalizar               = silverstack_finalizar,
		.AnSISOP_asignarValorCompartida  = silverstack_asignarValorCompartida,
		.AnSISOP_irAlLabel               = silverstack_irAlLabel,
		.AnSISOP_llamarSinRetorno        = silverstack_llamarSinRetorno,
		.AnSISOP_llamarConRetorno        = silverstack_llamarConRetorno,
		.AnSISOP_retornar                = silverstack_retornar,
};
AnSISOP_kernel primitivasKernel = {
		.AnSISOP_signal = silverstack_signal,
		.AnSISOP_wait   = silverstack_wait,
};

// main
int main(int argc, char *argv[])
{
	signal(SIGINT, depuracion);
	signal(SIGUSR1, depuracion);
	// Definicion de variables
	seguirEjecutando = 1; // Mediante la señal SIGUSR1 se puede dejar de ejecutar el cpu
	// Obtengo datos de archivo de configuracion y se crea el logger
	variables = list_create();
	config = config_create("../cpu.config");
	strcpy(umvip, config_get_string_value(config, "UMV_IP"));
	strcpy(kernelip, config_get_string_value(config, "KERNEL_IP"));
	port_kernel = config_get_int_value(config, "PORT_KERNEL");
	port_umv = config_get_int_value(config, "PORT_UMV");
	config_destroy(config);
	logger = log_create("../logcpu.log", "CPU", true, LOG_LEVEL_INFO);
	log_info(logger, "Se leyo el arch de config y se creo el logger satisfactoriamente.");
	// Me conecto al kernel
	ConectarA(&sockKernel, &port_kernel, kernelip, &kernel_addr, logger);
	log_info(logger, "Conectado al kernel.");
	// Obtengo datos de la umv
	ConectarA(&sockUmv, &port_umv, umvip, &umv_addr, logger);
	log_info(logger, "Conectado a la UMV.");
	// Handshake con el kernel
	mensaje.id_proceso = CPU;
	mensaje.tipo = HANDSHAKE;
	strcpy(mensaje.mensaje, "Hola kernel.");
	send(sockKernel, &mensaje, sizeof(t_mensaje), 0);
	if (recv(sockKernel, &mensaje, sizeof(t_mensaje), 0) == 0)
	{
		log_error(logger, "Kernel desconectado.");
		depuracion(SIGINT);
	}
	if (mensaje.tipo == HANDSHAKEOK)
	{
		log_info(logger, "Handshake con kernel satisfactorio.");
	}
	else
	{
		log_error(logger, "Handshake con kernel erroneo.");
		depuracion(SIGINT);
	}
	// Handshake con la UMV
	msg_handshake.tipo = CPU;
	send(sockUmv, &msg_handshake, sizeof(t_msg_handshake), 0);
	if (recv(sockUmv, &msg_handshake, sizeof(t_msg_handshake), 0) == 0)
	{
		log_error(logger, "UMV desconectada.");
		depuracion(SIGINT);
	}
	if (msg_handshake.tipo == UMV)
	{
		log_info(logger, "Handshake con UMV satisfactorio.");
	}
	else
	{
		log_error(logger, "Handshake con UMV erroneo.");
		depuracion(SIGINT);
	}
	t_mensaje msg_aux;
	int i;
	int j;
	int quantum, retardo;
	int salir_bucle = 0;
	int inicio_instruccion = 0;
	int cantidad_letras_instruccion = 0;
	int pos_en_instruccion = 0;
	char buf[82]; // Variable auxiliar para almacenar la linea de codigo
	char instruccion[82];
	int bufferaux[2];
	if (recv(sockKernel, &mensaje, sizeof(t_mensaje), 0) == 0)
	{
		log_error(logger, "Kernel desconectado.");
		depuracion(SIGINT);
	}
	quantum = mensaje.datosNumericos;
	if (recv(sockKernel, &mensaje, sizeof(t_mensaje), 0) == 0)
	{
		log_error(logger, "Kernel desconectado.");
		depuracion(SIGINT);
	}
	retardo = mensaje.datosNumericos;
	if (recv(sockKernel, &mensaje, sizeof(t_mensaje), 0) == 0)
	{
		log_error(logger, "Kernel desconectado.");
		depuracion(SIGINT);
	}
	stack = mensaje.datosNumericos;
	char cadena_aux[15];
	// Bucle principal del proceso
	while(seguirEjecutando)
	{
		// Recibo el pcb del kernel
		if (recv(sockKernel, &pcb, sizeof(t_pcb), 0) == 0)
		{
			log_error(logger, "Kernel desconectado.");
			depuracion(SIGINT);
		}
		log_info(logger,"Recibi PCB de Kernel");
		// Regenero diccionario de variables
		regenerarDiccionario();
		for (i = 0; i < quantum; i++)
		{
			if (proceso_bloqueado == 0 && proceso_finalizo == 0)
			{
				salir_bucle = 0;
				inicio_instruccion = 0;
				cantidad_letras_instruccion = 0;
				// Preparo mensaje para la UMV
				msg_solicitud_bytes.base = pcb.instruction_index;
				msg_solicitud_bytes.offset = pcb.program_counter * 8;
				msg_solicitud_bytes.tamanio = 8;
				msg_cambio_proceso_activo.id_programa = pcb.unique_id;
				mensaje.tipo = SOLICITUDBYTES;
				send(sockUmv, &mensaje, sizeof(t_mensaje), 0);
				send(sockUmv, &msg_cambio_proceso_activo, sizeof(t_msg_cambio_proceso_activo), 0);
				send(sockUmv, &msg_solicitud_bytes, sizeof(t_msg_solicitud_bytes), 0);
				// Espero la respuesta de la UMV
				if (recv(sockUmv, &mensaje, sizeof(t_mensaje), 0) == 0)
				{
					log_error(logger, "UMV desconectada.");
					depuracion(SIGINT);
				}
				if (recv(sockUmv, &bufferaux, 8, 0) == 0)
				{
					log_error(logger, "UMV desconectada.");
					depuracion(SIGINT);
				}
				// Preparo mensaje para la UMV
				msg_solicitud_bytes.base = pcb.code_segment;
				msg_solicitud_bytes.offset = bufferaux[0];
				msg_solicitud_bytes.tamanio = bufferaux[1];
				msg_cambio_proceso_activo.id_programa = pcb.unique_id;
				mensaje.tipo = SOLICITUDBYTES;
				send(sockUmv, &mensaje, sizeof(t_mensaje), 0);
				send(sockUmv, &msg_cambio_proceso_activo, sizeof(t_msg_cambio_proceso_activo), 0);
				send(sockUmv, &msg_solicitud_bytes, sizeof(t_msg_solicitud_bytes), 0);
				// Espero la respuesta de la UMV
				if (recv(sockUmv, &mensaje, sizeof(t_mensaje), 0) == 0)
				{
					log_error(logger, "UMV desconectada.");
					depuracion(SIGINT);
				}
				if (recv(sockUmv, &buf, bufferaux[1], 0) == 0)
				{
					log_error(logger, "UMV desconectada.");
					depuracion(SIGINT);
				}
				buf[bufferaux[1]] = '\0';
				// Verifico limites de instruccion
				while (salir_bucle != 1)
				{
					if(buf[inicio_instruccion] == '\t' || buf[inicio_instruccion] == '\0')
					{
						inicio_instruccion++;
					}
					else
					{
						salir_bucle = 1;
					}
				}
				salir_bucle = 0;
				pos_en_instruccion = inicio_instruccion;
				while (salir_bucle != 1)
				{
					if(buf[pos_en_instruccion] != '\n')
					{
						cantidad_letras_instruccion++;
					}
					else
					{
						salir_bucle = 1;
					}
					pos_en_instruccion++;
				}
				memcpy(&instruccion[0], &buf[inicio_instruccion], cantidad_letras_instruccion);
				instruccion[cantidad_letras_instruccion] = '\0';
				// Analizo la instruccion y ejecuto primitivas necesarias
				log_info(logger, "Me llego la instruccion: %s.", instruccion);
				usleep(retardo);
				analizadorLinea(strdup(instruccion), &primitivas, &primitivasKernel);
				log_info(logger, "Se termino de procesar la instruccion: %s.", instruccion);
				// Actualizo el pcb
				pcb.program_counter++;
			}
			else
			{
				break;
			}
		}
		// Aviso al kernel que termino el quantum del proceso y devuelvo pcb actualizado
		if (proceso_finalizo == 1)
		{
			// Envio a consola de programa el valor final de las variables
			proceso_finalizo = 0;
			silverstack_imprimirTexto("Valor final de variables:\n");
			for (j = 0; j < list_size(variables); j++)
			{
				nueva_var = list_get(variables, j);
				cadena_aux[0] = nueva_var->id;
				cadena_aux[1] = ' ';
				cadena_aux[2] = '=';
				cadena_aux[3] = ' ';
				cadena_aux[4] = '\0';
				silverstack_imprimirTexto(cadena_aux);
				silverstack_imprimir(nueva_var->valor);
			}
			mensaje.id_proceso = CPU;
			mensaje.tipo = PROGRAMFINISH;
			send(sockKernel, &mensaje, sizeof(t_mensaje), 0);
			log_info(logger, "Envie PROGRAMFINISH al kernel.");
			send(sockKernel, &pcb, sizeof(t_pcb), 0);
			log_info(logger, "Envie PCB al kernel.");
			proceso_finalizo = 0;
			proceso_imprimir_valores_finales = 1;
		}
		else
		{
			mensaje.id_proceso = CPU;
			mensaje.tipo = QUANTUMFINISH;
			send(sockKernel, &mensaje, sizeof(t_mensaje), 0);
			log_info(logger,"Envie QUANTUMFINISH al Kernel");
			send(sockKernel, &pcb, sizeof(t_pcb), 0);
			log_info(logger,"Envie PCB al Kernel");
			proceso_bloqueado = 0;
		}
	}
	log_info(logger, "Se deja de dar servicio a sistema.");
	// Libero memoria
	log_destroy(logger);
	config_destroy(config);
	free(variables);
	// Cierro los sockets
	close(sockKernel);
	close(sockUmv);
	return 0;
}

void regenerarDiccionario()
{
	// Regenero diccionario de variables
	t_mensaje msg_aux;
	char buffer_stack[stack];
	int i;

	list_clean(variables);

	if (pcb.context_actual != 0)
	{
		msg_solicitud_bytes.base = pcb.stack_segment;
		msg_solicitud_bytes.offset = pcb.stack_pointer - pcb.stack_segment;
		msg_solicitud_bytes.tamanio = pcb.context_actual * 5;
		msg_cambio_proceso_activo.id_programa = pcb.unique_id;
		mensaje.tipo = SOLICITUDBYTES;
		send(sockUmv, &mensaje, sizeof(t_mensaje), 0);
		send(sockUmv, &msg_cambio_proceso_activo, sizeof(t_msg_cambio_proceso_activo), 0);
		send(sockUmv, &msg_solicitud_bytes, sizeof(t_msg_solicitud_bytes), 0);
		if (recv(sockUmv, &mensaje, sizeof(t_mensaje), 0) == 0)
		{
			log_error(logger, "UMV desconectada.");
			depuracion(SIGINT);
		}
		if (mensaje.tipo == ENVIOBYTES)
		{
			if (recv(sockUmv, &buffer_stack, (pcb.context_actual * 5), 0) == 0)
			{
				log_error(logger, "UMV desconectada.");
				depuracion(SIGINT);
			}
			for (i = 0; i < pcb.context_actual; i++)
			{
				nueva_var = (t_variable *)malloc(sizeof(t_variable));
				nueva_var->id = buffer_stack[i * 5];
				nueva_var->dir = pcb.stack_pointer + (i * 5);
				memcpy(&nueva_var->valor, &buffer_stack[(i * 5) + 1], 4);
				list_add(variables, nueva_var);
			}
		}
		else
		{
			msg_aux.tipo = mensaje.tipo;
			send(sockKernel, &msg_aux, sizeof(t_mensaje), 0);
			proceso_finalizo = 1;
		}
	}
}

// Definicion de funciones
void ConectarA(int *sock, int *puerto, char *ip, struct sockaddr_in *their_addr, t_log *logger)
{
	// Creo el descriptor para el socket y compruebo errores
	if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		log_info(logger, "Error cuando se crea el socket.");
		exit(1);
	}
	// Asigno las variables de direccion a conectar
	their_addr->sin_family = AF_INET;
	their_addr->sin_port = htons(*puerto);
	their_addr->sin_addr.s_addr = inet_addr(ip);
	memset(&(their_addr->sin_zero), '\0', 8);
	// Conecto el socket y compruebo errores
	if (connect(*sock, (struct sockaddr *)their_addr, sizeof(struct sockaddr)) == -1)
	{
		log_info(logger, "Error conectando el socket.");
		exit(1);
	}
}

t_puntero silverstack_definirVariable(t_nombre_variable var)
{
	// 1) Reservar espacio en memoria para la variable
	// 2) Registrar variable en el Stack
	// 3) Registrar variable en el diccionario de variables
	// 4) Guardar contexto actual en el pcb
	// 5) Retornar la posicion de la variable
	t_puntero ptr;
	t_mensaje msg_aux;
	char buffaux[5];
	buffaux[0] = var;
	msg_cambio_proceso_activo.id_programa = pcb.unique_id;
	mensaje.tipo = ENVIOBYTES;
	msg_envio_bytes.base = pcb.stack_segment;
	msg_envio_bytes.offset = (pcb.stack_pointer - pcb.stack_segment) + (5 * pcb.context_actual);
	msg_envio_bytes.tamanio = 5;
	send(sockUmv, &mensaje, sizeof(t_mensaje), 0);
	send(sockUmv, &msg_cambio_proceso_activo, sizeof(t_msg_cambio_proceso_activo), 0);
	send(sockUmv, &msg_envio_bytes, sizeof(t_msg_envio_bytes), 0);
	send(sockUmv, buffaux, 5, 0);
	if (recv(sockUmv, &mensaje, sizeof(t_mensaje), 0) == 0)
	{
		log_error(logger, "UMV desconectada.");
		depuracion(SIGINT);
	}
	if (mensaje.tipo == ENVIOBYTES)
	{
		ptr = pcb.stack_pointer + (5 * pcb.context_actual);
		pcb.context_actual++;
		nueva_var = (t_variable *)malloc(sizeof(t_variable));
		nueva_var->id = var;
		nueva_var->dir = ptr;
		list_add(variables, nueva_var);
	}
	else
	{
		msg_aux.tipo = mensaje.tipo;
		send(sockKernel, &msg_aux, sizeof(t_mensaje), 0);
		proceso_finalizo = 1;
	}
	return ptr;
}

t_puntero silverstack_obtenerPosicionVariable(t_nombre_variable var)
{
	// 1) Busco la direccion de la variable en el diccionario de variables y la devuelvo
	t_mensaje msg_aux;
	t_puntero ptr = 0;
	int encontre = 0;
	int i = 0;
	for (i = 0; i < list_size(variables); i++)
	{
		nueva_var = list_get(variables, i);
		if (nueva_var->id == var)
		{
			encontre = 1;
			break;
		}
	}
	if (encontre == 1)
	{
		ptr = nueva_var->dir;
	}
	else
	{
		msg_aux.tipo = SEGMENTATION_FAULT;
		send(sockKernel, &msg_aux, sizeof(t_mensaje), 0);
		proceso_finalizo = 1;
		proceso_imprimir_valores_finales = 0;
	}
	return ptr;
	/*
		while(no_encontre)
		{
			msg_solicitud_bytes.base = pcb.stack_pointer;
			msg_solicitud_bytes.offset = offset;
			msg_solicitud_bytes.tamanio = 5;
			msg_cambio_proceso_activo.id_programa = pcb.unique_id;
			mensaje.tipo = SOLICITUDBYTES;
			send(sockUmv, &mensaje, sizeof(t_mensaje), 0);
			send(sockUmv, &msg_cambio_proceso_activo, sizeof(t_msg_cambio_proceso_activo), 0);
			send(sockUmv, &msg_solicitud_bytes, sizeof(t_msg_solicitud_bytes), 0);
			recv(sockUmv, &mensaje, sizeof(t_mensaje), 0);
			recv(sockUmv, &buffer, sizeof(buffer), 0);
			if (buffer[0] == var)
			{
				no_encontre = 0;
			}
			else
			{
				offset += 5;
			}
		}
		*/
}

t_valor_variable silverstack_dereferenciar(t_puntero dir_var)
{
	// 1) Se lee el valor de la variable almacenada en dir_var
	t_valor_variable valor = 0;
	t_mensaje msg_aux;
	char buffer[5];
	msg_solicitud_bytes.base = pcb.stack_segment;
	msg_solicitud_bytes.offset = dir_var - pcb.stack_segment;
	msg_solicitud_bytes.tamanio = 5;
	msg_cambio_proceso_activo.id_programa = pcb.unique_id;
	mensaje.tipo = SOLICITUDBYTES;
	send(sockUmv, &mensaje, sizeof(t_mensaje), 0);
	send(sockUmv, &msg_cambio_proceso_activo, sizeof(t_msg_cambio_proceso_activo), 0);
	send(sockUmv, &msg_solicitud_bytes, sizeof(t_msg_solicitud_bytes), 0);
	if (recv(sockUmv, &mensaje, sizeof(t_mensaje), 0) == 0)
	{
		log_error(logger, "UMV desconectada.");
		depuracion(SIGINT);
	}
	if (mensaje.tipo == ENVIOBYTES)
	{
		if (recv(sockUmv, &buffer, 5, 0) == 0)
		{
			log_error(logger, "UMV desconectada.");
			depuracion(SIGINT);
		}
		memcpy(&valor, &buffer[1], 4);
	}
	else
	{
		msg_aux.tipo = mensaje.tipo;
		send(sockKernel, &msg_aux, sizeof(t_mensaje), 0);
		proceso_finalizo = 1;
		proceso_imprimir_valores_finales = 0;
	}
	return valor;
}

void silverstack_asignar(t_puntero dir_var, t_valor_variable valor)
{
	// 1) Mando a la UMV el valor de la variable junto con su direccion
	// 2) Actualizo diccionario de variables
	int buffer;
	t_mensaje msg_aux;
	msg_envio_bytes.base = pcb.stack_segment;
	msg_envio_bytes.offset = (pcb.stack_pointer - pcb.stack_segment) + dir_var - pcb.stack_pointer + 1;
	msg_envio_bytes.tamanio = 4;
	mensaje.tipo = ENVIOBYTES;
	msg_cambio_proceso_activo.id_programa = pcb.unique_id;
	send(sockUmv, &mensaje, sizeof(t_mensaje), 0);
	send(sockUmv, &msg_cambio_proceso_activo, sizeof(t_msg_cambio_proceso_activo), 0);
	send(sockUmv, &msg_envio_bytes, sizeof(t_msg_envio_bytes), 0);
	buffer = valor;
	send(sockUmv, &buffer, sizeof(buffer), 0);
	if (recv(sockUmv, &mensaje, sizeof(t_mensaje), 0) == 0)
	{
		log_error(logger, "UMV desconectada.");
		depuracion(SIGINT);
	}
	if (mensaje.tipo == ENVIOBYTES)
	{
		int i = 0;
		for (i = 0; i < list_size(variables); i++)
		{
			nueva_var = list_get(variables, i);
			if (nueva_var->dir == (int)dir_var)
			{
				break;
			}
		}
		nueva_var->valor = valor;
	}
	else
	{
		msg_aux.tipo = mensaje.tipo;
		send(sockKernel, &msg_aux, sizeof(t_mensaje), 0);
		proceso_finalizo = 1;
	}
}

void silverstack_imprimir(t_valor_variable valor)
{
	// 1) Envio al kernel el valor para que lo imprima la correspondiente consola
	if (proceso_finalizo != 1 && proceso_imprimir_valores_finales == 1)
	{
		t_mensaje msg;
		msg.id_proceso = CPU;
		msg.tipo = IMPRIMIR;
		msg.datosNumericos = valor;
		send(sockKernel, &msg, sizeof(t_mensaje), 0);
		if (recv(sockKernel, &msg, sizeof(t_mensaje), 0) == 0)
		{
			log_error(logger, "Kernel desconectado.");
			depuracion(SIGINT);
		}
	}
}

void silverstack_imprimirTexto(char *texto)
{
	// 1) Envio al kernel la cadena de texto para que la reenvíe a la correspondiente consola
	t_mensaje msg;
	msg.id_proceso = CPU;
	msg.tipo = IMPRIMIRTEXTO;
	msg.datosNumericos = strlen(texto);
	send(sockKernel, &msg, sizeof(t_mensaje), 0);
	char buf[strlen(texto)];
	strcpy(buf, texto);
	send(sockKernel, &buf, sizeof(buf), 0);
	if (recv(sockKernel, &msg, sizeof(t_mensaje), 0) == 0)
	{
		log_error(logger, "Kernel desconectado.");
		depuracion(SIGINT);
	}
}

t_valor_variable silverstack_obtenerValorCompartida(t_nombre_compartida varCom)
{
	// 1) Solicito al kernel el valor de la variable varCom
	// 2) Devuelvo el valor recibido
	t_valor_variable valor = 0;
	t_mensaje msg;
	msg.id_proceso = CPU;
	msg.tipo = VARCOMREQUEST;
	strcpy(msg.mensaje, varCom);
	send(sockKernel, &msg, sizeof(t_mensaje), 0);
	if (recv(sockKernel, &msg, sizeof(t_mensaje), 0) == 0)
	{
		log_error(logger, "Kernel desconectado.");
		depuracion(SIGINT);
	}
	valor = msg.datosNumericos;
	return valor;
}

void silverstack_entradaSalida(t_nombre_dispositivo dispositivo, int tiempo)
{
	// 1) Envio al kernel el tiempo de entrada/salida de dispositivo
	t_mensaje msg;
	msg.id_proceso = CPU;
	msg.tipo = ENTRADASALIDA;
	msg.datosNumericos = tiempo;
	strcpy(msg.mensaje, dispositivo);
	send(sockKernel, &msg, sizeof(t_mensaje), 0);
	if (recv(sockKernel, &msg, sizeof(t_mensaje), 0) == 0)
	{
		log_error(logger, "Kernel desconectado.");
		depuracion(SIGINT);
	}
	proceso_bloqueado = 1;
}

void silverstack_finalizar()
{
	int nuevo_contexto;
	int buffer;
	if (pcb.stack_pointer == pcb.stack_segment)
	{
		proceso_finalizo = 1;
	}
	else
	{
		// Busco el program counter del contexto anterior
		msg_solicitud_bytes.base = pcb.stack_segment;
		msg_solicitud_bytes.offset = (pcb.stack_pointer - pcb.stack_segment) - 4;
		msg_solicitud_bytes.tamanio = 4;
		msg_cambio_proceso_activo.id_programa = pcb.unique_id;
		mensaje.tipo = SOLICITUDBYTES;
		send(sockUmv, &mensaje, sizeof(t_mensaje), 0);
		send(sockUmv, &msg_cambio_proceso_activo, sizeof(t_msg_cambio_proceso_activo), 0);
		send(sockUmv, &msg_solicitud_bytes, sizeof(t_msg_solicitud_bytes), 0);
		if (recv(sockUmv, &mensaje, sizeof(t_mensaje), 0) == 0)
		{
			log_error(logger, "UMV desconectada.");
			depuracion(SIGINT);
		}
		if (recv(sockUmv, &buffer, 4, 0) == 0)
		{
			log_error(logger, "UMV desconectada.");
			depuracion(SIGINT);
		}
		pcb.program_counter = buffer;
		// Busco direccion del contexto anterior
		msg_solicitud_bytes.base = pcb.stack_segment;
		msg_solicitud_bytes.offset = (pcb.stack_pointer - pcb.stack_segment) - 8;
		msg_solicitud_bytes.tamanio = 4;
		msg_cambio_proceso_activo.id_programa = pcb.unique_id;
		mensaje.tipo = SOLICITUDBYTES;
		send(sockUmv, &mensaje, sizeof(t_mensaje), 0);
		send(sockUmv, &msg_cambio_proceso_activo, sizeof(t_msg_cambio_proceso_activo), 0);
		send(sockUmv, &msg_solicitud_bytes, sizeof(t_msg_solicitud_bytes), 0);
		if (recv(sockUmv, &mensaje, sizeof(t_mensaje), 0) == 0)
		{
			log_error(logger, "UMV desconectada.");
			depuracion(SIGINT);
		}
		if (recv(sockUmv, &buffer, 4, 0) == 0)
		{
			log_error(logger, "UMV desconectada.");
			depuracion(SIGINT);
		}
		nuevo_contexto = (pcb.stack_pointer - buffer - 8) / 5;
		pcb.context_actual = nuevo_contexto;
		pcb.stack_pointer = buffer;
		regenerarDiccionario();
	}
}

t_valor_variable silverstack_asignarValorCompartida(t_nombre_compartida varCom, t_valor_variable valor)
{
	// 1) Envio al kernel el valor de la variable compartida a asignar
	// 2) Devuelvo el valor asignado
	t_mensaje msg;
	msg.id_proceso = CPU;
	msg.tipo = ASIGNACION;
	msg.datosNumericos = valor;
	strcpy(msg.mensaje, varCom);
	send(sockKernel, &msg, sizeof(t_mensaje), 0);
	if (recv(sockKernel, &msg, sizeof(t_mensaje), 0) == 0)
	{
		log_error(logger, "Kernel desconectado.");
		depuracion(SIGINT);
	}
	return valor;
}

void silverstack_irAlLabel(t_nombre_etiqueta etiqueta)
{
	int salir = 0;
	int dir_instruccion;
	char buffer[pcb.size_etiquetas_index];
	msg_solicitud_bytes.base = pcb.etiquetas_index;
	msg_solicitud_bytes.offset = 0;
	msg_solicitud_bytes.tamanio = pcb.size_etiquetas_index;
	msg_cambio_proceso_activo.id_programa = pcb.unique_id;
	mensaje.tipo = SOLICITUDBYTES;
	send(sockUmv, &mensaje, sizeof(t_mensaje), 0);
	send(sockUmv, &msg_cambio_proceso_activo, sizeof(t_msg_cambio_proceso_activo), 0);
	send(sockUmv, &msg_solicitud_bytes, sizeof(t_msg_solicitud_bytes), 0);
	if (recv(sockUmv, &mensaje, sizeof(t_mensaje), 0) == 0)
	{
		log_error(logger, "UMV desconectada.");
		depuracion(SIGINT);
	}
	if (recv(sockUmv, &buffer, sizeof(buffer), 0) == 0)
	{
		log_error(logger, "UMV desconectada.");
		depuracion(SIGINT);
	}
	dir_instruccion = metadata_buscar_etiqueta(etiqueta, buffer, pcb.size_etiquetas_index);
	pcb.program_counter = dir_instruccion - 1;
}

void silverstack_llamarSinRetorno(t_nombre_etiqueta etiqueta)
{
	// 1) Preservo el contexto actual
	// 2) Preservo el program counter
	// 3) Asigno el nuevo contexto al puntero de stack
	// 4) Reseteo a 0 el tamanio del contexto actual
	int buffer;
	t_mensaje msg_aux;
	int nuevo_contexto = pcb.stack_pointer + (5 * pcb.context_actual);
	buffer = pcb.stack_pointer;
	pcb.stack_pointer = nuevo_contexto;
	msg_envio_bytes.base = pcb.stack_segment;
	msg_envio_bytes.offset = pcb.stack_pointer - pcb.stack_segment;
	msg_envio_bytes.tamanio = 4;
	mensaje.tipo = ENVIOBYTES;
	msg_cambio_proceso_activo.id_programa = pcb.unique_id;
	send(sockUmv, &mensaje, sizeof(t_mensaje), 0);
	send(sockUmv, &msg_cambio_proceso_activo, sizeof(t_msg_cambio_proceso_activo), 0);
	send(sockUmv, &msg_envio_bytes, sizeof(t_msg_envio_bytes), 0);
	send(sockUmv, &buffer, sizeof(buffer), 0);
	if (recv(sockUmv, &mensaje, sizeof(t_mensaje), 0) == 0)
	{
		log_error(logger, "UMV desconectada.");
		depuracion(SIGINT);
	}
	if (mensaje.tipo == ENVIOBYTES)
	{
		pcb.stack_pointer += 4;
		buffer = pcb.program_counter;
		msg_envio_bytes.base = pcb.stack_segment;
		msg_envio_bytes.offset = pcb.stack_pointer - pcb.stack_segment;
		msg_envio_bytes.tamanio = 4;
		mensaje.tipo = ENVIOBYTES;
		msg_cambio_proceso_activo.id_programa = pcb.unique_id;
		send(sockUmv, &mensaje, sizeof(t_mensaje), 0);
		send(sockUmv, &msg_cambio_proceso_activo, sizeof(t_msg_cambio_proceso_activo), 0);
		send(sockUmv, &msg_envio_bytes, sizeof(t_msg_envio_bytes), 0);
		send(sockUmv, &buffer, sizeof(buffer), 0);
		if (recv(sockUmv, &mensaje, sizeof(t_mensaje), 0) == 0)
		{
			log_error(logger, "UMV desconectada.");
			depuracion(SIGINT);
		}
		if (mensaje.tipo == ENVIOBYTES)
		{
			pcb.stack_pointer += 4;
			pcb.context_actual = 0;
			list_clean(variables);
			silverstack_irAlLabel(etiqueta);
		}
		else
		{
			msg_aux.tipo = mensaje.tipo;
			send(sockKernel, &msg_aux, sizeof(t_mensaje), 0);
			proceso_finalizo = 1;
		}
	}
	else
	{
		msg_aux.tipo = mensaje.tipo;
		send(sockKernel, &msg_aux, sizeof(t_mensaje), 0);
		proceso_finalizo = 1;
	}
}

void silverstack_llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar)
{
	// 1) Preservo el contexto actual
	// 2) Preservo el program counter
	// 3) Preservo donde retornar el valor
	// 4) Asigno el nuevo contexto al puntero de stack
	// 5) Reseteo a 0 el tamanio del contexto actual
	int buffer;
	t_mensaje msg_aux;
	int nuevo_contexto = pcb.stack_pointer + (5 * pcb.context_actual);
	buffer = pcb.stack_pointer;
	pcb.stack_pointer = nuevo_contexto;
	msg_envio_bytes.base = pcb.stack_segment;
	msg_envio_bytes.offset = pcb.stack_pointer - pcb.stack_segment;
	msg_envio_bytes.tamanio = 4;
	mensaje.tipo = ENVIOBYTES;
	msg_cambio_proceso_activo.id_programa = pcb.unique_id;
	send(sockUmv, &mensaje, sizeof(t_mensaje), 0);
	send(sockUmv, &msg_cambio_proceso_activo, sizeof(t_msg_cambio_proceso_activo), 0);
	send(sockUmv, &msg_envio_bytes, sizeof(t_msg_envio_bytes), 0);
	send(sockUmv, &buffer, sizeof(buffer), 0);
	if (recv(sockUmv, &mensaje, sizeof(t_mensaje), 0) == 0)
	{
		log_error(logger, "UMV desconectada.");
		depuracion(SIGINT);
	}
	if (mensaje.tipo == ENVIOBYTES)
	{
		pcb.stack_pointer += 4;
		buffer = pcb.program_counter;
		msg_envio_bytes.base = pcb.stack_segment;
		msg_envio_bytes.offset = pcb.stack_pointer - pcb.stack_segment;
		msg_envio_bytes.tamanio = 4;
		mensaje.tipo = ENVIOBYTES;
		msg_cambio_proceso_activo.id_programa = pcb.unique_id;
		send(sockUmv, &mensaje, sizeof(t_mensaje), 0);
		send(sockUmv, &msg_cambio_proceso_activo, sizeof(t_msg_cambio_proceso_activo), 0);
		send(sockUmv, &msg_envio_bytes, sizeof(t_msg_envio_bytes), 0);
		send(sockUmv, &buffer, sizeof(buffer), 0);
		if (recv(sockUmv, &mensaje, sizeof(t_mensaje), 0) == 0)
		{
			log_error(logger, "UMV desconectada.");
			depuracion(SIGINT);
		}
		if (mensaje.tipo == ENVIOBYTES)
		{
			pcb.stack_pointer += 4;
			buffer = donde_retornar;
			msg_envio_bytes.base = pcb.stack_segment;
			msg_envio_bytes.offset = pcb.stack_pointer - pcb.stack_segment;
			msg_envio_bytes.tamanio = 4;
			mensaje.tipo = ENVIOBYTES;
			msg_cambio_proceso_activo.id_programa = pcb.unique_id;
			send(sockUmv, &mensaje, sizeof(t_mensaje), 0);
			send(sockUmv, &msg_cambio_proceso_activo, sizeof(t_msg_cambio_proceso_activo), 0);
			send(sockUmv, &msg_envio_bytes, sizeof(t_msg_envio_bytes), 0);
			send(sockUmv, &buffer, sizeof(buffer), 0);
			if (recv(sockUmv, &mensaje, sizeof(t_mensaje), 0) == 0)
			{
				log_error(logger, "UMV desconectada.");
				depuracion(SIGINT);
			}
			if (mensaje.tipo == ENVIOBYTES)
			{
				pcb.stack_pointer += 4;
				pcb.context_actual = 0;
				list_clean(variables);
				silverstack_irAlLabel(etiqueta);
			}
			else
			{
				msg_aux.tipo = mensaje.tipo;
				send(sockKernel, &msg_aux, sizeof(t_mensaje), 0);
				proceso_finalizo = 1;
			}
		}
		else
		{
			msg_aux.tipo = mensaje.tipo;
			send(sockKernel, &msg_aux, sizeof(t_mensaje), 0);
			proceso_finalizo = 1;
		}
	}
	else
	{
		msg_aux.tipo = mensaje.tipo;
		send(sockKernel, &msg_aux, sizeof(t_mensaje), 0);
		proceso_finalizo = 1;
	}
}

void silverstack_retornar(t_valor_variable valor)
{
	int buffer;
	int nuevo_contexto;
	int donde_retornar;
	// Busco donde retornar
	msg_solicitud_bytes.base = pcb.stack_segment;
	msg_solicitud_bytes.offset = (pcb.stack_pointer - pcb.stack_segment) - 4;
	msg_solicitud_bytes.tamanio = 4;
	msg_cambio_proceso_activo.id_programa = pcb.unique_id;
	mensaje.tipo = SOLICITUDBYTES;
	send(sockUmv, &mensaje, sizeof(t_mensaje), 0);
	send(sockUmv, &msg_cambio_proceso_activo, sizeof(t_msg_cambio_proceso_activo), 0);
	send(sockUmv, &msg_solicitud_bytes, sizeof(t_msg_solicitud_bytes), 0);
	if (recv(sockUmv, &mensaje, sizeof(t_mensaje), 0) == 0)
	{
		log_error(logger, "UMV desconectada.");
		depuracion(SIGINT);
	}
	if (recv(sockUmv, &buffer, 4, 0) == 0)
	{
		log_error(logger, "UMV desconectada.");
		depuracion(SIGINT);
	}
	donde_retornar = buffer;
	// Asigno el valor de retorno donde debo
	buffer = valor;
	msg_envio_bytes.base = pcb.stack_segment;
	msg_envio_bytes.offset = donde_retornar - pcb.stack_segment + 1;
	msg_envio_bytes.tamanio = 4;
	mensaje.tipo = ENVIOBYTES;
	msg_cambio_proceso_activo.id_programa = pcb.unique_id;
	send(sockUmv, &mensaje, sizeof(t_mensaje), 0);
	send(sockUmv, &msg_cambio_proceso_activo, sizeof(t_msg_cambio_proceso_activo), 0);
	send(sockUmv, &msg_envio_bytes, sizeof(t_msg_envio_bytes), 0);
	send(sockUmv, &buffer, sizeof(buffer), 0);
	if (recv(sockUmv, &mensaje, sizeof(t_mensaje), 0) == 0)
	{
		log_error(logger, "UMV desconectada.");
		depuracion(SIGINT);
	}
	// Busco el program counter del contexto anterior
	msg_solicitud_bytes.base = pcb.stack_segment;
	msg_solicitud_bytes.offset = (pcb.stack_pointer - pcb.stack_segment) - 8;
	msg_solicitud_bytes.tamanio = 4;
	msg_cambio_proceso_activo.id_programa = pcb.unique_id;
	mensaje.tipo = SOLICITUDBYTES;
	send(sockUmv, &mensaje, sizeof(t_mensaje), 0);
	send(sockUmv, &msg_cambio_proceso_activo, sizeof(t_msg_cambio_proceso_activo), 0);
	send(sockUmv, &msg_solicitud_bytes, sizeof(t_msg_solicitud_bytes), 0);
	if (recv(sockUmv, &mensaje, sizeof(t_mensaje), 0) == 0)
	{
		log_error(logger, "UMV desconectada.");
		depuracion(SIGINT);
	}
	if (recv(sockUmv, &buffer, 4, 0) == 0)
	{
		log_error(logger, "UMV desconectada.");
		depuracion(SIGINT);
	}
	pcb.program_counter = buffer;
	// Busco direccion del contexto anterior
	msg_solicitud_bytes.base = pcb.stack_segment;
	msg_solicitud_bytes.offset = (pcb.stack_pointer - pcb.stack_segment) - 12;
	msg_solicitud_bytes.tamanio = 4;
	msg_cambio_proceso_activo.id_programa = pcb.unique_id;
	mensaje.tipo = SOLICITUDBYTES;
	send(sockUmv, &mensaje, sizeof(t_mensaje), 0);
	send(sockUmv, &msg_cambio_proceso_activo, sizeof(t_msg_cambio_proceso_activo), 0);
	send(sockUmv, &msg_solicitud_bytes, sizeof(t_msg_solicitud_bytes), 0);
	if (recv(sockUmv, &mensaje, sizeof(t_mensaje), 0) == 0)
	{
		log_error(logger, "UMV desconectada.");
		depuracion(SIGINT);
	}
	if (recv(sockUmv, &buffer, 4, 0) == 0)
	{
		log_error(logger, "UMV desconectada.");
		depuracion(SIGINT);
	}
	nuevo_contexto = (pcb.stack_pointer - buffer - 8) / 5;
	pcb.context_actual = nuevo_contexto;
	pcb.stack_pointer = buffer;
	regenerarDiccionario();
}

void silverstack_signal(t_nombre_semaforo identificador_semaforo)
{
	// 1) Envio al kernel el semaforo para que ejecute signal en él
	t_mensaje msg;
	msg.id_proceso = CPU;
	msg.tipo = SIGNALSEM;
	strcpy(msg.mensaje, identificador_semaforo);
	send(sockKernel, &msg, sizeof(t_mensaje), 0);
	if (recv(sockKernel, &msg, sizeof(t_mensaje), 0) == 0)
	{
		log_error(logger, "Kernel desconectado.");
		depuracion(SIGINT);
	}
}

void silverstack_wait(t_nombre_semaforo identificador_semaforo)
{
	// 1) Envio al kernel el semaforo para que se ejecute wait en él
	t_mensaje msg;
	msg.id_proceso = CPU;
	msg.tipo = WAITSEM;
	strcpy(msg.mensaje, identificador_semaforo);
	send(sockKernel, &msg, sizeof(t_mensaje), 0);
	if (recv(sockKernel, &msg, sizeof(t_mensaje), 0) == 0)
	{
		log_error(logger, "Kernel desconectado.");
		depuracion(SIGINT);
	}
	if (msg.tipo == BLOCK)
	{
		proceso_bloqueado = 1;
	}
}

void depuracion(int senial)
{
	switch(senial)
	{
	case SIGINT:
		log_info(logger, "Inicia depuracion de variables y memoria.");
		log_destroy(logger);
		exit(0);
		break;
	case SIGUSR1:
		log_info(logger, "Se solicito por señal que el cpu deje de dar servicio.");
		seguirEjecutando = 0;
		break;
	}
}

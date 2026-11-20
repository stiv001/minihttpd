// server.c - passiveTCP

#include <netdb.h>
#include <sys/socket.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "server.h"

// passiveTCP: crea un socket TCP pasivo que escucha en el puerto service
// y deja que qlen clientes esperen el accept

int
passiveTCP(const char *service, int qlen)
{
	struct addrinfo  direccion_pedida;      // que tipo de direccion pedimos
	struct addrinfo *direccion_encontrada;  // la que llena getaddrinfo
	int              socket_servidor;       // el socket que escucha
	int              codigo_error;          // lo que devuelve getaddrinfo
	int              activar = 1;           // 1 = activar SO_REUSEADDR

	memset(&direccion_pedida, 0, sizeof(direccion_pedida)); // sin basura
	direccion_pedida.ai_family = AF_INET;       // IPv4
	direccion_pedida.ai_socktype = SOCK_STREAM; // TCP
	direccion_pedida.ai_flags = AI_PASSIVE;     // cualquier IP local

	codigo_error = getaddrinfo(NULL, service, &direccion_pedida,
	                           &direccion_encontrada);
	if (codigo_error != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(codigo_error));
		exit(1);
	}

	socket_servidor = socket(direccion_encontrada->ai_family,
	                         direccion_encontrada->ai_socktype,
	                         direccion_encontrada->ai_protocol);
	if (socket_servidor < 0) {
		perror("socket");
		exit(1);
	}
	// permite reusar el puerto aunque este en TIME_WAIT
	if (setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEADDR, &activar,
	               sizeof(activar)) < 0) {
		perror("setsockopt");
		exit(1);
	}

	if (bind(socket_servidor, direccion_encontrada->ai_addr,
	         direccion_encontrada->ai_addrlen) < 0) {
		perror("bind");
		exit(1);
	}
	freeaddrinfo(direccion_encontrada); // ya no se necesita

	if (listen(socket_servidor, qlen) < 0) {
		perror("listen");
		exit(1);
	}
	return socket_servidor;
}

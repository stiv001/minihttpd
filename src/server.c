/* server.c - passiveTCP */

#include <netdb.h>
#include <sys/socket.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "server.h"

// passiveTCP crea un socket TCP pasivo escuchando en un puerto

int
passiveTCP(const char *service, int qlen)
{
	struct addrinfo  hints;  // que tipo de direccion pedimos
	struct addrinfo *res;    // direccion que llena el sistema
	int              s, err; // socket y codigo de error

	memset(&hints, 0, sizeof(hints)); // todo en cero esas direcciones de
	                                  // memoria pueden tener basura
	hints.ai_family = AF_INET;        // IPv4
	hints.ai_socktype = SOCK_STREAM;  // TCP
	hints.ai_flags = AI_PASSIVE;      // escucharcualquier IP local

	err = getaddrinfo(NULL, service, &hints, &res);
	if (err != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
		exit(1);
	}

	s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (s < 0) {
		perror("socket");
		exit(1);
	}

	if (bind(s, res->ai_addr, res->ai_addrlen) < 0) {
		perror("bind");
		exit(1);
	}
	freeaddrinfo(res); /* ya no se necesita la lista	 */

	if (listen(s, qlen) < 0) {
		perror("listen");
		exit(1);
	}
	return s;
}

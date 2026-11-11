/* main.c - main */

#include <netinet/in.h>
#include <sys/socket.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "server.h"

#define QLEN 32      // clientes que esperan el accept
#define BUFSIZE 4096 // tamano del buffer de la peticion

// main - servidor HTTP secuencial

int
main(int argc, char *argv[])
{
	char              *service = "8080"; // puerto por defecto
	struct sockaddr_in client;           // direccion del cliente
	socklen_t          alen;             // largo de esa direccion
	int                sd, nsd;          // socket pasivo y del cliente
	char               buf[BUFSIZE];     // peticion que llega
	char              *resp;             // respuesta fija
	int                n;                // bytes recibidos

	if (argc > 1) //./minihttpd 9000
		service = argv[1];

	sd = passiveTCP(service, QLEN);
	printf("minihttpd escuchando en el puerto %s\n", service);

	while (1) { // un cliente por vuelta
		alen = sizeof(client);
		nsd = accept(sd, (struct sockaddr *)&client, &alen);
		if (nsd < 0) {
			perror("accept");
			continue; // el servidor sigue vivo
		}

		n = recv(nsd, buf, sizeof(buf) - 1, 0);
		if (n > 0) {
			buf[n] = '\0'; // cerrar el texto
			printf("---- peticion (%d bytes) ----\n%s", n, buf);
		}

		resp = "HTTP/1.1 200 OK\r\n"
		       "Content-Type: text/plain\r\n"
		       "Content-Length: 20\r\n"
		       "Connection: close\r\n"
		       "\r\n"
		       "Hola desde minihttpd";
		send(nsd, resp, strlen(resp), 0);
		close(nsd); // fin de este cliente
	}
}

// main.c - servidor HTTP secuencial

#include <sys/socket.h>

#include <stdio.h>
#include <unistd.h>

#include "http.h"
#include "server.h"

#define QLEN 32 // clientes que esperan el accept

int
main(int argc, char *argv[])
{
	char           *puerto = "8080"; // puerto por defecto
	int             sd, nsd;         // socket pasivo y del cliente
	char            texto_recibido[TAMANO_MAXIMO_PETICION]; // lo que llega
	char            cuerpo_respuesta[2048]; // texto que se devuelve
	struct peticion peticion;               // la peticion ya separada
	int             bytes_recibidos;        // lo que trajo el recv
	int             codigo_estado;          // 200, 400 o 405

	if (argc > 1) //./minihttpd 9000
		puerto = argv[1];

	sd = passiveTCP(puerto, QLEN);
	printf("minihttpd escuchando en el puerto %s\n", puerto);

	while (1) { // un cliente por vuelta
		nsd = accept(sd, NULL, NULL);
		if (nsd < 0) {
			perror("accept");
			continue;
		}

		bytes_recibidos = recv(nsd, texto_recibido,
		                       sizeof(texto_recibido) - 1, 0);
		if (bytes_recibidos > 0) {
			texto_recibido[bytes_recibidos] = '\0'; // cerrar el texto
			codigo_estado = leer_peticion(texto_recibido, &peticion);
			printf("%s %s -> %d\n", peticion.metodo, peticion.ruta,
			       codigo_estado);
			if (codigo_estado == 200)
				snprintf(cuerpo_respuesta, sizeof(cuerpo_respuesta),
				         "Ruta solicitada: %s\nHost: %s\nUser-Agent: %s\n",
				         peticion.ruta, peticion.host,
				         peticion.programa_cliente);
			else
				snprintf(cuerpo_respuesta, sizeof(cuerpo_respuesta),
				         "Error %d\n", codigo_estado);
			enviar_respuesta(nsd, codigo_estado, cuerpo_respuesta);
		}
		close(nsd); // fin de este cliente
	}
}

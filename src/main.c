// main.c - servidor HTTP con epoll

#include <sys/epoll.h>
#include <sys/socket.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "files.h"
#include "http.h"
#include "mime.h"
#include "server.h"

#define QLEN             32   // clientes que esperan el accept
#define MAXIMO_AVISOS    64   // avisos que se atienden por vuelta
#define MAXIMO_SOCKETS   1024 // numero de socket mas alto que aceptamos
#define TAMANO_PEDACITO  4096 // bytes del archivo que se mandan por aviso

// un cuaderno por cliente: la posicion es el numero de su socket
char texto_recibido[MAXIMO_SOCKETS][TAMANO_MAXIMO_PETICION];
int  bytes_recibidos[MAXIMO_SOCKETS]; // cuanto lleva escrito cada cuaderno
int  archivo_abierto[MAXIMO_SOCKETS]; // archivo que se le esta enviando
                                      // (-1 = ninguno)

// cerrar_cliente: lo saca de la lista de vigilancia, cierra el archivo
// que se le estaba enviando (si habia) y cierra la conexion
void
cerrar_cliente(int lista_vigilancia, int socket_cliente)
{
	epoll_ctl(lista_vigilancia, EPOLL_CTL_DEL, socket_cliente, NULL);
	if (archivo_abierto[socket_cliente] >= 0) {
		close(archivo_abierto[socket_cliente]);
		archivo_abierto[socket_cliente] = -1;
	}
	close(socket_cliente);
}

// atender_cliente: guarda lo que llego y, si la peticion ya esta
// completa, abre el archivo pedido y manda la cabecera
void
atender_cliente(int lista_vigilancia, int socket_cliente)
{
	struct peticion    peticion;          // la peticion ya separada
	struct epoll_event cambio;            // para pedir otro tipo de aviso
	char               cuerpo_error[64];  // texto de las respuestas de error
	int                bytes_nuevos;      // lo que trajo este recv
	int                codigo_estado;     // 200, 400, 403, 404, 405 o 500
	int                archivo;           // el archivo pedido, ya abierto
	long               bytes_archivo;     // tamano de ese archivo

	// escribir a continuacion de lo que ya habia en su cuaderno
	bytes_nuevos = recv(socket_cliente,
	                    texto_recibido[socket_cliente] +
	                        bytes_recibidos[socket_cliente],
	                    TAMANO_MAXIMO_PETICION - 1 -
	                        bytes_recibidos[socket_cliente],
	                    0);
	if (bytes_nuevos <= 0) { // 0: el cliente cerro, -1: error
		cerrar_cliente(lista_vigilancia, socket_cliente);
		return;
	}
	bytes_recibidos[socket_cliente] += bytes_nuevos;
	texto_recibido[socket_cliente][bytes_recibidos[socket_cliente]] = '\0';

	// la peticion termina con una linea vacia: \r\n\r\n
	if (strstr(texto_recibido[socket_cliente], "\r\n\r\n") == NULL) {
		if (bytes_recibidos[socket_cliente] < TAMANO_MAXIMO_PETICION - 1)
			return; // falta, pero cabe: esperar el resto
		enviar_respuesta(socket_cliente, 400, "Error 400\n"); // no cabe
		cerrar_cliente(lista_vigilancia, socket_cliente);
		return;
	}

	codigo_estado = leer_peticion(texto_recibido[socket_cliente],
	                              &peticion);
	if (codigo_estado == 200) { // la peticion esta bien: buscar el archivo
		if (strcmp(peticion.ruta, "/") == 0) // la portada
			snprintf(peticion.ruta, sizeof(peticion.ruta),
			         "/index.html");
		codigo_estado = abrir_archivo(peticion.ruta, &archivo,
		                              &bytes_archivo);
	}
	printf("[socket %d] %s %s -> %d\n", socket_cliente, peticion.metodo,
	       peticion.ruta, codigo_estado);

	if (codigo_estado != 200) { // error: respuesta corta y cerrar
		snprintf(cuerpo_error, sizeof(cuerpo_error), "Error %d\n",
		         codigo_estado);
		enviar_respuesta(socket_cliente, codigo_estado, cuerpo_error);
		cerrar_cliente(lista_vigilancia, socket_cliente);
		return;
	}

	// todo bien: mandar la cabecera ahora y el archivo por pedacitos
	enviar_cabecera(socket_cliente, 200, tipo_mime(peticion.ruta),
	                bytes_archivo);
	archivo_abierto[socket_cliente] = archivo;

	// desde ahora avisar cuando se le pueda enviar (EPOLLOUT),
	// ya no cuando mande datos (EPOLLIN)
	cambio.events = EPOLLOUT;
	cambio.data.fd = socket_cliente;
	epoll_ctl(lista_vigilancia, EPOLL_CTL_MOD, socket_cliente, &cambio);
}

// enviar_pedacito: manda el siguiente pedacito del archivo. Asi un
// archivo grande no deja esperando a los demas clientes.
void
enviar_pedacito(int lista_vigilancia, int socket_cliente)
{
	char pedacito[TAMANO_PEDACITO]; // parte del archivo que se envia
	int  bytes_leidos;              // cuanto se leyo del archivo

	bytes_leidos = read(archivo_abierto[socket_cliente], pedacito,
	                    sizeof(pedacito));
	if (bytes_leidos > 0 &&
	    send(socket_cliente, pedacito, bytes_leidos, 0) == bytes_leidos)
		return; // falta mas: el resto en los siguientes avisos

	// se termino el archivo (read dio 0) o algo fallo
	cerrar_cliente(lista_vigilancia, socket_cliente);
}

int
main(int argc, char *argv[])
{
	char              *puerto = "8080"; // puerto por defecto
	int                sd, nsd;         // socket pasivo y del cliente
	int                lista_vigilancia;        // la lista de epoll
	struct epoll_event inscripcion;             // para agregar un socket
	struct epoll_event avisos[MAXIMO_AVISOS];   // avisos que llegan
	int                cantidad_avisos, i;
	int                socket_con_aviso;        // de quien es el aviso

	if (argc > 1) //./minihttpd 9000
		puerto = argv[1];

	signal(SIGPIPE, SIG_IGN); // si un cliente se va antes, no morir

	sd = passiveTCP(puerto, QLEN);
	printf("minihttpd escuchando en el puerto %s\n", puerto);

	lista_vigilancia = epoll_create1(0);
	if (lista_vigilancia < 0) {
		perror("epoll_create1");
		exit(1);
	}
	inscripcion.events = EPOLLIN; // avisar cuando llegue un cliente
	inscripcion.data.fd = sd;
	epoll_ctl(lista_vigilancia, EPOLL_CTL_ADD, sd, &inscripcion);

	while (1) {
		// unico lugar donde el servidor duerme
		cantidad_avisos = epoll_wait(lista_vigilancia, avisos,
		                             MAXIMO_AVISOS, -1);
		for (i = 0; i < cantidad_avisos; i++) {
			socket_con_aviso = avisos[i].data.fd;

			if (socket_con_aviso == sd) { // llego un cliente nuevo
				nsd = accept(sd, NULL, NULL);
				if (nsd < 0)
					continue;
				if (nsd >= MAXIMO_SOCKETS) { // no cabe
					close(nsd);
					continue;
				}
				bytes_recibidos[nsd] = 0; // cuaderno vacio
				archivo_abierto[nsd] = -1; // nada que enviar
				inscripcion.events = EPOLLIN; // avisar si manda datos
				inscripcion.data.fd = nsd;
				epoll_ctl(lista_vigilancia, EPOLL_CTL_ADD, nsd,
				          &inscripcion);
			} else if (avisos[i].events & EPOLLOUT) { // puede recibir
				enviar_pedacito(lista_vigilancia,
				                socket_con_aviso);
			} else { // un cliente conectado mando datos
				atender_cliente(lista_vigilancia,
				                socket_con_aviso);
			}
		}
	}
}

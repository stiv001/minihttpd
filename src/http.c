// http.c - leer_peticion, enviar_respuesta

#include <sys/socket.h>

#include <stdio.h>
#include <string.h>

#include "http.h"

// leer_peticion: separa el texto recibido en partes y devuelve el codigo
// de estado (200 valida, 400 mal escrita, 405 metodo distinto de GET)
int
leer_peticion(char *texto_recibido, struct peticion *peticion)
{
	char *inicio_header; // donde empieza un header dentro del texto

	memset(peticion, 0, sizeof(*peticion)); // todo vacio al empezar

	// primera linea: GET /ruta HTTP/1.1
	// los numeros limitan cuanto se copia en cada campo
	if (sscanf(texto_recibido, "%7s %1023s %15s", peticion->metodo,
	           peticion->ruta, peticion->version) != 3)
		return 400; // no vinieron las 3 partes
	if (peticion->ruta[0] != '/' ||
	    strncmp(peticion->version, "HTTP/", 5) != 0)
		return 400; // ruta o version mal escritas
	if (strcmp(peticion->metodo, "GET") != 0)
		return 405; // por ahora solo aceptamos GET

	// headers: buscar el nombre y copiar el valor hasta el \r
	inicio_header = strstr(texto_recibido, "\r\nHost: ");
	if (inicio_header != NULL)
		sscanf(inicio_header + 8, "%255[^\r]", peticion->host);

	inicio_header = strstr(texto_recibido, "\r\nConnection: ");
	if (inicio_header != NULL)
		sscanf(inicio_header + 14, "%31[^\r]", peticion->conexion);

	inicio_header = strstr(texto_recibido, "\r\nUser-Agent: ");
	if (inicio_header != NULL)
		sscanf(inicio_header + 14, "%255[^\r]",
		       peticion->programa_cliente);

	return 200;
}

// frase_del_codigo: el texto que va junto al numero en la respuesta
static const char *
frase_del_codigo(int codigo_estado)
{
	if (codigo_estado == 200)
		return "OK";
	if (codigo_estado == 405)
		return "Method Not Allowed";
	return "Bad Request";
}

// enviar_respuesta: manda la linea de estado, los headers y el cuerpo
void
enviar_respuesta(int socket_cliente, int codigo_estado,
                 const char *cuerpo_respuesta)
{
	char cabecera[256];  // linea de estado + headers
	int  bytes_cabecera; // cuantos bytes ocupo la cabecera

	bytes_cabecera = snprintf(cabecera, sizeof(cabecera),
	                          "HTTP/1.1 %d %s\r\n"
	                          "Content-Type: text/plain\r\n"
	                          "Content-Length: %zu\r\n"
	                          "Connection: close\r\n"
	                          "\r\n",
	                          codigo_estado,
	                          frase_del_codigo(codigo_estado),
	                          strlen(cuerpo_respuesta));
	send(socket_cliente, cabecera, bytes_cabecera, 0);
	send(socket_cliente, cuerpo_respuesta, strlen(cuerpo_respuesta), 0);
}

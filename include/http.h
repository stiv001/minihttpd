// http.h - leer peticiones HTTP y enviar respuestas

#ifndef HTTP_H
#define HTTP_H

#define TAMANO_MAXIMO_PETICION 4096 // bytes que puede tener una peticion

// las partes de una peticion, ya separadas
struct peticion {
	char metodo[8];             // GET, POST, ...
	char ruta[1024];            // lo que se pide, ej: /index.html
	char version[16];           // HTTP/1.1
	char host[256];             // header Host
	char conexion[32];          // header Connection
	char programa_cliente[256]; // header User-Agent (curl, navegador...)
};

int  leer_peticion(char *texto_recibido, struct peticion *peticion);
void enviar_respuesta(int socket_cliente, int codigo_estado,
                      const char *cuerpo_respuesta);

#endif

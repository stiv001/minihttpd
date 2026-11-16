// files.c - abrir_archivo

#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "files.h"

// codigo_del_error: traduce el errno a un codigo de estado
static int
codigo_del_error(void)
{
	if (errno == ENOENT || errno == ENOTDIR)
		return 404; // no existe
	if (errno == EACCES)
		return 403; // existe, pero no hay permiso
	return 500;         // otro problema del servidor
}

// abrir_archivo: busca la ruta pedida dentro de www y la abre.
// Devuelve el codigo de estado (200, 403, 404 o 500). Si es 200 deja
// el archivo abierto en *archivo y su tamano en *bytes_archivo.
int
abrir_archivo(const char *ruta, int *archivo, long *bytes_archivo)
{
	char        ruta_pedida[2048];   // www + la ruta que pidio el cliente
	char        ruta_real[PATH_MAX]; // la ruta completa y sin ..
	char        carpeta_real[PATH_MAX]; // donde esta www de verdad
	size_t      largo_carpeta;          // letras de carpeta_real
	struct stat info;                   // datos del archivo abierto

	snprintf(ruta_pedida, sizeof(ruta_pedida), "%s%s", CARPETA_WWW, ruta);

	// realpath quita los .. y nos da la direccion verdadera
	if (realpath(CARPETA_WWW, carpeta_real) == NULL)
		return 500; // no encontramos la carpeta www
	if (realpath(ruta_pedida, ruta_real) == NULL)
		return codigo_del_error();

	// directory traversal: lo pedido tiene que estar dentro de www
	largo_carpeta = strlen(carpeta_real);
	if (strncmp(ruta_real, carpeta_real, largo_carpeta) != 0 ||
	    ruta_real[largo_carpeta] != '/')
		return 403;

	*archivo = open(ruta_real, O_RDONLY);
	if (*archivo < 0)
		return codigo_del_error();

	// fstat: tamano del archivo y si es un archivo normal (no carpeta)
	if (fstat(*archivo, &info) < 0) {
		close(*archivo);
		return 500;
	}
	if (!S_ISREG(info.st_mode)) {
		close(*archivo);
		return 403;
	}
	*bytes_archivo = info.st_size;
	return 200;
}

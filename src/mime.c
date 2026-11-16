// mime.c - tipo_mime

#include <string.h>

#include "mime.h"

#define CANTIDAD_TIPOS 5 // filas de la tabla

// la tabla del PDF: extension y su tipo de contenido
static const char *tabla_mime[CANTIDAD_TIPOS][2] = {
	{".html", "text/html"},
	{".css",  "text/css"},
	{".js",   "application/javascript"},
	{".png",  "image/png"},
	{".jpg",  "image/jpeg"},
};

// tipo_mime: busca la extension de la ruta en la tabla y devuelve
// su tipo. Si no esta, devuelve el tipo para datos desconocidos.
const char *
tipo_mime(const char *ruta)
{
	const char *extension; // desde el ultimo punto, ej: ".css"
	int         i;

	extension = strrchr(ruta, '.');
	if (extension != NULL)
		for (i = 0; i < CANTIDAD_TIPOS; i++)
			if (strcmp(extension, tabla_mime[i][0]) == 0)
				return tabla_mime[i][1];
	return "application/octet-stream";
}

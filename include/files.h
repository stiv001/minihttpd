// files.h - buscar y abrir los archivos de la carpeta www

#ifndef FILES_H
#define FILES_H

#define CARPETA_WWW "www" // de aqui salen todos los archivos

int abrir_archivo(const char *ruta, int *archivo, long *bytes_archivo);

#endif

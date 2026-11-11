/* server.h - passiveTCP */

#ifndef SERVER_H
#define SERVER_H
// Crea un socket TCP pasivo escuchando en un puerto sus parametros son el
// nombre
//  del servicio y la longitud de la cola de espera
int passiveTCP(const char *service, int qlen);

#endif

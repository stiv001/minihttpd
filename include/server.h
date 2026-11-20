// server.h - passiveTCP

#ifndef SERVER_H
#define SERVER_H

// crea un socket TCP pasivo; service es el puerto ("8080") y qlen
// cuantos clientes pueden esperar el accept
int passiveTCP(const char *service, int qlen);

#endif

# minihttpd

Servidor HTTP/1.1 básico en C para Linux. Atiende varios clientes a la vez con un solo hilo usando `epoll` y sirve los archivos que están en la carpeta `www/`.

Es el proyecto 1 de Computación Distribuida. Todavía está en proceso; abajo voy anotando cómo fue avanzando.

## Cómo compilar y ejecutar

```
make
./minihttpd          # escucha en el puerto 8080
./minihttpd 9000     # o en el puerto que se le pase
```

Hay que ejecutarlo desde la carpeta del proyecto, porque los archivos se buscan en `www/` (ruta relativa). Para borrar lo compilado: `make clean`.

## Estructura

```
minihttpd/
├── Makefile
├── README.md
├── include/    http.h, server.h, files.h
├── src/        main.c, server.c, http.c, files.c
└── www/        index.html, style.css
```

- `main.c`: el bucle con `epoll` y la atención de cada cliente.
- `server.c`: `passiveTCP`, crea el socket que escucha.
- `http.c`: lee la petición y arma las respuestas.
- `files.c`: busca el archivo pedido dentro de `www/`.

## Cómo fue avanzando

**1. Estructura inicial.** Lo primero fue crear las carpetas y archivos que pide el enunciado (`include/`, `src/`, `www/`, el `Makefile` y este README), todavía casi vacíos.

**2. Servidor secuencial con respuesta fija.** Antes de meterme con HTTP quise asegurarme de que la parte del socket funcionara: que `bind` y `listen` no dieran error y que se pudiera conectar alguien. Para eso hice un servidor secuencial (atiende a un cliente, termina y pasa al siguiente) que siempre responde lo mismo. Con eso ya pude probar con `curl` y el navegador que la respuesta HTTP llegaba bien.

**3. El error "Address already in use".** Al probar me pasaba que si cerraba el servidor y lo volvía a abrir enseguida, `bind` fallaba con *Address already in use* y tenía que esperar un rato. Investigando vi que el puerto se queda un tiempo en estado TIME_WAIT después de cerrar. Hice una rama aparte para probar la solución y, cuando funcionó, la uní a main. La solución es `setsockopt` con `SO_REUSEADDR` antes del `bind`, así el servidor se puede reiniciar sin esperar.

**4. Leer la petición (`http.c`).** Después empecé con la parte de HTTP: separar la primera línea en método, ruta y versión, y sacar los headers `Host`, `Connection` y `User-Agent`. Con eso el servidor ya responde:
- `200 OK` si la petición está bien,
- `400 Bad Request` si viene mal escrita o no cabe en 4096 bytes,
- `405 Method Not Allowed` si el método no es `GET`.

Para no tener problemas de buffer overflow uso `snprintf` y `sscanf` con un tamaño máximo en cada campo.

**5. De secuencial a `epoll`.** Con el servidor secuencial vi que si un cliente se conectaba con `nc` y no mandaba nada, todos los demás se quedaban esperando. Como el proyecto pide atender varios clientes a la vez con un solo hilo, lo cambié a `epoll`: el servidor vigila todos los sockets y solo atiende al que tiene algo listo. Cada cliente tiene su propio buffer, así una petición que llega en partes se va juntando hasta que llega el `\r\n\r\n`. Ahora un cliente callado ya no traba a los demás.

**6. Servir archivos de `www/` (`files.c`).** Para buscar el archivo pedido uso `realpath()`, que convierte la ruta en la ruta real sin `..`. Después reviso que siga dentro de `www/`; así una petición como `GET /../../etc/passwd` no puede salirse de la carpeta (directory traversal). Con esto se agregaron los códigos:
- `403 Forbidden` si la ruta se sale de `www/` o no es un archivo normal,
- `404 Not Found` si el archivo no existe,
- `500 Internal Server Error` si falla algo inesperado del sistema.

**7. Archivos grandes por pedacitos.** Consultando con el profesor, me recomendó que los archivos grandes no se manden de una sola vez, porque mientras el servidor manda un archivo enorme los demás clientes se quedan esperando. Ahora el archivo se manda en pedacitos de 4096 bytes: cada vez que `epoll` avisa que el cliente puede recibir (`EPOLLOUT`) se manda un pedacito y se pasa a atender a los demás.

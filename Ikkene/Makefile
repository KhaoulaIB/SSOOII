CC=gcc 
CFLAGS=-c -g -Wall -std=gnu99
LDFLAGS=-pthread

SOURCES=bloques.c mi_mkfs.c ficheros_basico.c leer_sf.c ficheros.c escribir.c leer.c permitir.c truncar.c  mi_escribir_varios.c prueba_cache_tabla.c directorios.c mi_mkdir.c mi_chmod.c mi_ls.c  mi_stat.c  mi_touch.c mi_escribir.c mi_cat.c mi_link.c mi_rm.c mi_rmdir.c  mi_rm_r.c semaforo_mutex_posix.c simulacion.c verificacion.c mi_rn.c mi_cp.c mi_mv.c mi_cp_f.c
LIBRARIES=bloques.o ficheros_basico.o ficheros.o directorios.o semaforo_mutex_posix.o 
INCLUDES=bloques.h ficheros_basico.h ficheros.h directorios.h semaforo_mutex_posix.h simulacion.h
PROGRAMS=mi_mkfs leer_sf escribir leer permitir truncar mi_escribir_varios prueba_cache_tabla mi_mkdir mi_chmod mi_ls mi_stat mi_touch mi_escribir mi_cat mi_link  mi_rm mi_rmdir  mi_rm_r simulacion verificacion mi_rn mi_cp mi_mv mi_cp_f
OBJS=$(SOURCES:.c=.o)

all: $(OBJS) $(PROGRAMS)

$(PROGRAMS): $(LIBRARIES) $(INCLUDES)
	$(CC) $(LDFLAGS) $(LIBRARIES) $@.o -o $@

%.o: %.c $(INCLUDES)
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm -rf *.o *~ $(PROGRAMS) disco* ext* res* 

#include "ficheros.h"
#include <sys/time.h>


#define TAMNOMBRE 60 //tamaño del nombre de directorio o fichero, en Ext2 = 256
struct entrada {
    char nombre[TAMNOMBRE];
    unsigned int ninodo;
};

#define DEBUGN7 0
#define DEBUGN9 0
#define DEBUGN12 1




#define USARCACHE 2 //0:sin caché, 1: última L/E, 2:tabla FIFO, 3:tabla LRU

#define PROFUNDIDAD 32 //profundidad máxima del árbol de directorios

struct UltimaEntrada{
   char camino [TAMNOMBRE*PROFUNDIDAD];
   int p_inodo;

  #if USARCACHE==3 // tabla LRU
      struct timeval ultima_consulta;
  #endif
  #if USARCACHE==2
  int ultima_insercion;
  #endif


};

// #if USARCACHE==1//ultima L/E
// struct UltimaEntrada UltimaEntradaEscritura;
// #endif

//tabla caché directorios
#if (USARCACHE==2 || USARCACHE==3)
   #define CACHE_SIZE 3 // cantidad de entradas para la caché
//struct UltimaEntrada UltimasEntradas[CACHE_SIZE];//se declara en directorios.c 

#endif


//errores
#define ERROR_CAMINO_INCORRECTO (-2)
#define ERROR_PERMISO_LECTURA (-3)
#define ERROR_NO_EXISTE_ENTRADA_CONSULTA (-4)
#define ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO (-5)
#define ERROR_PERMISO_ESCRITURA (-6)
#define ERROR_ENTRADA_YA_EXISTENTE (-7)
#define ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO (-8)

//funciones
int extraer_camino(const char *camino, char *inicial, char *final, char *tipo);
int buscar_entrada(const char *camino_parcial, unsigned int *p_inodo_dir, unsigned int *p_inodo, unsigned int *p_entrada, char reservar, unsigned char permisos);
//funcion auxiliar para mostrar mensajes de los diferentes errores  
void mostrar_error_buscar_entrada(int error);
int mi_creat(const char *camino, unsigned char permisos);
int mi_dir(const char *camino, char *buffer, char tipo);
int mi_chmod(const char *camino, unsigned char permisos);
int mi_stat(const char *camino, struct STAT *p_stat);
int mi_write(const char *camino, const void *buf, unsigned int offset, unsigned int nbytes);
int mi_read(const char *camino, void *buf, unsigned int offset, unsigned int nbytes);
//NIVEL 10
int mi_link(const char *camino1, const char *camino2);
int mi_unlink(const char *camino);
int mi_rm_r(const char *ruta);

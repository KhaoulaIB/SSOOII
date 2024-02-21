#include "bloques.h"
#include <stdio.h>
#include <string.h>


/**
 * Programa principal
 * Formatea el dispositivo virtual con el tamaño de nbloques. 
 * @param   argc número de parametros en argv
 * @param   argv    cadena de comandos
 * @return  0 si el programa se ejecuto correctamente
*/
 

int main(int argc, char **argv){

if (argc!=3){
    fprintf(stderr, RED"Sintaxis incorrecta:./mi_mkfs <nombre_dispositivo> <nbloques>.\n"RESET);
    return FALLO;
}
//obtener nombre de dispositivo
char * camino = argv[1];
//número de bloques
int nbloques = atoi(argv[2]);

//montar el dispositivo virtual
  if (bmount(camino)<0){
    fprintf(stderr, RED "Se ha producido un error al montar el dispositivo.\n"RESET);
    return FALLO;
  }
    //buffer 
    unsigned char buf [nbloques];
    //inicializar los elementos del buffer a 0s
    memset(buf, 0, nbloques);
    //Escritura en el dispositivo
  for (int i = 0; i<nbloques; i++){
   if ( bwrite(i, buf)<0){
    fprintf(stderr, RED"Error al escribir en el bloque %i\n."RESET, i);
    return FALLO;
   }

  }

  //desmontar el dispositivo virtual
  if (bumount(camino)<0){

    fprintf(stderr, RED"Error al desmontar el dispositivo.\n" RESET);
    return FALLO;
  }

    


}

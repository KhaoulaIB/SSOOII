
#include "ficheros.h"

/**
 * Devuelve la cantidad de bytes escritos realmente 
 * @param   ninodo  posición del inodo en el AI
 * @param   buf_original  contiene el contenido que queremos escribir
 * @param   offset  posición de escritura inicial en bytes lógicos
 * @param   nbytes  número de bytes que queremos escribir
**/

int mi_write_f(unsigned int ninodo, const void *buf_original, unsigned int offset, unsigned int nbytes){

    unsigned int primerBL = offset/BLOCKSIZE;
    unsigned int ultimoBL = (offset+nbytes-1)/BLOCKSIZE;

    unsigned int desp1 = offset%BLOCKSIZE;
    unsigned int desp2 = (offset+nbytes-1)%BLOCKSIZE;
    unsigned char buf_bloque[BLOCKSIZE];

    if(primerBL == ultimoBL){ // Cuando solo se escribe sobre un bloque
        unsigned int nbfisico = traducir_bloque_inodo(&inodo,primerBL,1);
        if(bread(nbfisico,buf_bloque)== FALLO){
            fprintf(stderr, RED "Error al leer el bloque\n"RESET);
            return FALLO;
        }

        memcpy(buf_bloque+desp1,buf_original,nbytes);
        if(bwrite(nbytes, buf_bloque)<0){
            fprintf(stderr, RED "Error al escribir en el bloque\n"RESET);
            return FALLO;
        }
        return nbytes;

    }else if (primerBL < ultimoBL){ // Cuando hay más de un bloque
        // PRIMER PASO: Primer bloque lógico
        unsigned int nbfisico = traducir_bloque_inodo(&inodo,primerBL,1);
        if(bread(nbfisico,buf_bloque)== FALLO){
            fprintf(stderr, RED "Error al leer el bloque\n"RESET);
            return FALLO;
        }

        memcpy(buf_bloque + desp1, buf_original, BLOCKSIZE-desp1);
    //    if(bwrite(nbfisico, buf_bloque)<0){
            fprintf(stderr, RED "Error al escribir en el bloque\n"RESET);
            return FALLO;
        }

        // SEGUNDO PASO: Bloques lógicos intermedios
        if(bwrite(nbfisico, buf_original+(BLOCKSIZE-desp1)+(ultimoBL-primerBL-1)*BLOCKSIZE)<0){
            fprintf(stderr, RED "Error al escribir en el bloque\n"RESET);
            return FALLO;
        }

        // TERCER PASO: Último bloque lógico
        nbfisico = traducir_bloque_inodo(&inodo, ultimoBL, 1);
        if(bread(nbfisico,buf_bloque)== FALLO){
            fprintf(stderr, RED "Error al leer el bloque\n"RESET);
            return FALLO;
        }

        memcpy(buf_bloque, buf_original + (nbytes - (desp2 +1)), desp2 + 1);
//        if(bwrite(nbfisico, buf_original+(BLOCKSIZE-desp1)+(ultimoBL-primerBL-1)*BLOCKSIZE)<0){
            fprintf(stderr, RED "Error al escribir en el bloque\n"RESET);
            return FALLO;
        }
    }

    /*FALTA ACTUALIZAR:
    tamEnBytesLog
    mtime
    ctime*/

    /*if (escribir_inodo(ninodo,inodo)<0){
        fprintf(stderr, "Error al salvar el inodo()\n"RESET);
        return FALLO;
    }*/

    return nbytes;
}

/**
 * @param   ninodo  posicion del inodo en el AI
 * @param   buf_original    almacenará el contenido leido
 * @param   offset posicion de lectura inicial en bytes logicos
 * @param   nbytes  numero de bytes que queremos leer
*/

int mi_read_f(unsigned int inodo, void *buf_original, unsigned int offset, unsigned int nbytes){
    //Comprobamos si tenemos permisos
    if ((inodo.permisos&4) != 4){
        fprintf(stderr, RED"No hay permisos de lectura\n"RESET);
        return FALLO;
    }
    int leidos = 0;
    //Comprobamos si podemos leer
    if (offset >= inodo.tamEnBytesLog) {
        return leidos
    }

    if ((offset+nbytes) >= inodo.tamEnBytesLog) {
        nbytes = inodo.tamEnBytesLog-offset;
    }
    primerBL = offset/BLOCKSIZE;
    ultimoBL = (offset+nbytes-1)/BLOCKSIZE;
    desp1 = offset%BLOCKSIZE;
    desp2 = (offset+nbytes-1)%BLOCKSIZE;

    //PRIMER CASO 
    if (primerBL == ultimoBL) {
        unsigned int numero_bloque_fisico = traducir_bloque_inodo(&inodo, primerBL, 0);
        leidos = bread(numero_bloque_fisico, buf_bloque);
        if (leidos < 0) {
            fprintf(stderr, RED"Error al leer el bloque fisico en mi_read_f()\n"RESET);
            return FALLO;
        }
        memcpy(buf_original, buf_bloque+desp1, nbytes);
        return nbytes;
    }
    //SEGUNDO CASO
    else if (primerBL < ultimoBL) {
        //Primer bloque logico
        unsigned int numero_bloque_fisico = traducir_bloque_inodo(&inodo, primerBL, 0);
        leidos = bread(numero_bloque_fisico, buf_bloque);
        if (leidos < 0) {
            fprintf(stderr, RED"Error al leer el bloque fisico en mi_read_f()\n"RESET);
            return FALLO;
        }
        memcpy(buf_original, buf_bloque+desp1, BLOCKSIZE-desp1);
        int bytes_copiados = BLOCKSIZE - desp1;

        //Bloques intermedios
        for (int i=primerBL+1; i < ultimoBL; i++) {
            numero_bloque_fisico = traducir_bloque_inodo(&inodo, i, 0);
            leidos = bread(numero_bloque_fisico, buf_bloque);
            if (leidos < 0) {
                fprintf(stderr, RED"Error al leer el bloque fisico en mi_read_f()\n"RESET);
                return FALLO;
            }
            memcpy(buf_original + bytes_copiados, buf_bloque, BLOCKSIZE);
            bytes_copiados += BLOCKSIZE;
        }

        //Ultimo bloque
        numero_bloque_fisico = traducir_bloque_inodo(&inodo, ultimoBL, 0);
        leidos = bread(numero_bloque_fisico, buf_bloque);
        if (leidos < 0) {
            fprintf(stderr, RED"Error al leer el bloque fisico en mi_read_f()\n"RESET);
            return FALLO;
        }
        memcpy(buf_original + bytes_copiados, buf_bloque, desp2+1);
        bytes_copiados += desp2 + 1;
        
    }

    return bytes_copiados;
}

/**
 * Devuelve la metainformación de un fichero/directorio correspondiente 
 * al inodo pasado.
 * @param   ninodo  posición del inodo en el AI
 * @param   p_stat  strcut semejante al inodo pero sin los puntero ni el padding
*/
int mi_stat_f(unsigned int ninodo, struct STAT *p_stat){
    //tipo, permisos, cantidad de enlaces de entradas en directorio,
    // tamaño en bytes lógicos, timestamps y cantidad de bloques ocupados en la zona de datos
    struct inodo inodo;
    //leemos el inodo para obtener sus datos
    if (leer_inodo(ninodo,&inodo)==FALLO){
            perror("Errir en leer_inodo()");
            return FALLO;
    }

   //asignación de los datos del inodo a p_stat
    p_stat->atime=inodo.atime;
    p_stat->ctime=inodo.ctime;
    p_stat->mtime= inodo.mtime;
    p_stat->nlinks=inodo.nlinks;
    p_stat->numBloquesOcupados= inodo.numBloquesOcupados;
    p_stat->permisos=inodo.permisos;
    p_stat->tamEnBytesLog=inodo.tamEnBytesLog;
    p_stat->tipo= inodo.tipo;

 //printf ("%s %s %i %i %t %i", p_stat->tipo, p_stat->permisos, p_stat->nlinks, p_stat->tamEnBytesLog, p_stat->atime, p_stat->numBloquesOcupados);
    return EXITO;


}

/**
 * Cambia los permisos de un fichero/directorio (correspondiente al nº de inodo pasado como argumento, ninodo) 
 * @param   ninodo    nº inodo que cambairemos sus permisos
 * @param   permisos  permisos nuevos
 * @return  EXITO o FALLO
*/
int mi_chmod_f(unsigned int ninodo, unsigned char permisos){
        struct inodo inodo; 
        //leer el inodo para obtener los permisos
        if (leer_inodo(ninodo, &inodo)==FALLO){
              fprintf(stderr,RED"Error al leer el inodo en mi_chmod_f()\n"RESET);  
              return FALLO;
        }
        //cambiar los permisos
        inodo.permisos= permisos;
        //actualizar el tiempo de ultima modificación del inodo
        inodo.ctime= time(NULL);

        //guardar las modificaciones del inodo
        if (escribir_inodo(ninodo, &inodo)==FALLO){
            fprintf(stderr, RED"Error en escribir_inodo : mi_chmod_f()\n"RESET);    
            return FALLO;
        }
        return EXITO;
}

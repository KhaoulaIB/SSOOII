/**
 * @authors Khaoula Ikkene
 **/
#include "ficheros_basico.h"

/**
 * Calcula el tamaño en bloques para el mapa de bits.
 * @param   nbloques
 * @return  tamaño de bloques para el MB
 */

int tamMB(unsigned int nbloques)
{

    int tambloquesMb = (nbloques / 8) / BLOCKSIZE;
    if ((nbloques / 8) % BLOCKSIZE != 0)
    {
        // nbloques no es multiple de BLOCKSIZE
        tambloquesMb++;
    }
    return tambloquesMb;
}

/**
 * Calcula el tamaño en bloques para el array de inodos.
 * @param   ninodos
 * @return  tamaño de bloques para el AI.
 */

int tamAI(unsigned int ninodos)
{
    int tamAI = (ninodos * INODOSIZE) / BLOCKSIZE;
    if ((ninodos * INODOSIZE) % BLOCKSIZE == 0)
    {
        // Los inodos caben justo en los bloques
        return tamAI;
    }
    else
    {
        // Añadimos un bloque extra para los inodos restantes
        return tamAI + 1;
    }
}

/**
 * Inicializa los datos del superbloque.
 * @param   nbloques   número de bloques
 * @param   ninodos    número de inodos del sistema d ficheros
 * @return  0 o -1 si o no la grabación de estructura fue exitosa
 */

int initSB(unsigned int nbloques, unsigned int ninodos)
{

    struct superbloque SB;
    // Datos del mapa de bits
    SB.posPrimerBloqueMB = posSB + tamSB; // posSB= 0, tamSB= 1
    SB.posUltimoBloqueMB = SB.posPrimerBloqueMB + tamMB(nbloques) - 1;

    // Datos del array de inodos
    SB.posPrimerBloqueAI = SB.posUltimoBloqueMB + 1;
    SB.posUltimoBloqueAI = SB.posPrimerBloqueAI + tamAI(ninodos) - 1;

    // Datos del bloque de datos
    SB.posPrimerBloqueDatos = SB.posUltimoBloqueAI + 1;
    SB.posUltimoBloqueDatos = nbloques - 1;

    // Datos de los inodos y bloques
    SB.posPrimerInodoLibre = 0;
    // Posición del primer inodo libre
    SB.posInodoRaiz = 0;
    // Cantidad de bloques libres en el DISCO VIRTUAL
    SB.cantBloquesLibres = nbloques;
    // Cantidad de inodos libres en AI
    SB.cantInodosLibres = ninodos;
    // Cantidad total de bloques
    SB.totBloques = nbloques;
    // Cantidad total de inodos (BLOCKSIZE/4)
    SB.totInodos = ninodos;

    // Escribir la estructura en el bloques posSB
    if (bwrite(posSB, &SB) < 0)
    {
        fprintf(stderr, RED "Error al escribir la estrcutra del bloque.\n" RESET);
        return FALLO;
    }
    return EXITO;
}
/**
 * Inicializa lel mapa de bits poniendo a 1 los bits de metadatos.
 * @return
 */
int initMB(){
    struct superbloque SB;
    unsigned char bufferMB[BLOCKSIZE];
    if (memset(bufferMB, 0, BLOCKSIZE) == NULL)
    {
        fprintf(stderr, RED "Error de memset()\n" RESET);
        return FALLO;
    }

    // leer el superbloque para obtener nº de bloques y inodos
    if (bread(posSB, &SB) < 0)
    {
        fprintf(stderr, RED "Error al leer el SB en el metodo initMB() \n" RESET);
        return FALLO;
    }

    int bloquesMetadatos = tamSB + tamMB(SB.totBloques) + tamAI(SB.totInodos);

    int posEscribir_bloque = SB.posPrimerBloqueMB;
    int bytes_bincompleto = (bloquesMetadatos / 8) % BLOCKSIZE;

    if ((bloquesMetadatos / 8) > BLOCKSIZE)
    {
        int bloques_completos = (bloquesMetadatos / 8) / BLOCKSIZE;

        while (posEscribir_bloque < (bloques_completos + SB.posPrimerBloqueMB))
        {
            if (memset(bufferMB, 255, BLOCKSIZE) == NULL)
            {
                fprintf(stderr, RED "Error de memset()\n" RESET);
                return FALLO;
            }
            if (bwrite(posEscribir_bloque, bufferMB) < 0)
            {
                fprintf(stderr, RED "Error al escribir en el disco el MB\n" RESET);
                return FALLO;
            }
            posEscribir_bloque++;
        }
    }
    if (bytes_bincompleto != 0)
    {
        if (memset(bufferMB, 0, BLOCKSIZE) == NULL)
        {
            fprintf(stderr, RED "Error de memset()\n" RESET);
            return FALLO;
        }
        for (int i = 0; i < bytes_bincompleto; i++)
        {
            bufferMB[i] = 255;
        }
        if (bloquesMetadatos % 8 != 0)
        {
            bufferMB[bytes_bincompleto] = 255;
            bufferMB[bytes_bincompleto] = bufferMB[bytes_bincompleto] << (8 - (bloquesMetadatos % 8));
        }
        if (bwrite(posEscribir_bloque, bufferMB) < 0)
        {
            fprintf(stderr, RED "Error initMB bwrite (MB)" RESET);
            return FALLO;
        }
    }

    // restar estos bloques de la cantidad de bloques libres
    SB.cantBloquesLibres -= bloquesMetadatos;
    // guardar los cambios en el superbloque
    if (bwrite(posSB, &SB) < 0)
    {
        fprintf(stderr, RED "Error al guardar los cambios en el SB  \n" RESET);
        return FALLO;
    }

    return EXITO;
}

/**
 * Inicializa la lista de nodos libres
 * @return   0 o -1 si la inicializacion fue exitosa o no
 */

int initAI(){

    struct superbloque SB;
    // Preparar buffer de inodos segun BLOCKSIZE /INODOSIZE
    struct inodo inodos[BLOCKSIZE / INODOSIZE];

    // leer el superbloque para obtener la localizacion del AI
    if (bread(posSB, &SB) < 0)
    {
        fprintf(stderr, RED "Error al leer el SB en initAI()\n" RESET);
        return FALLO;
    }

    int contInodos = SB.posPrimerInodoLibre + 1;
    int fi = 0;

    // Leemos cada bloque
    for (int i = SB.posPrimerBloqueAI; i <= SB.posUltimoBloqueAI && fi == 0; i++)
    {
        if (bread(i, inodos) == -1)
        {
            fprintf(stderr, RED "ficheros_basico.c: Error al leer el bloque de inodos %d\n" RESET, i);
            return FALLO;
        }

        // Inicializamos cada inodo del bloque leído
        for (int j = 0; j < BLOCKSIZE / INODOSIZE; j++)
        {
            inodos[j].tipo = 'l';
            // Enlazamos con el siguiente inodo libre
            if (contInodos < SB.totInodos)
            {
                inodos[j].punterosDirectos[0] = contInodos;
                contInodos++;
            }
            else
            {
                inodos[j].punterosDirectos[0] = UINT_MAX;
                fi = 1;
                break;
            }
        }
        if (bwrite(i, inodos) == -1)
        {
            perror(RED "Error en escribir los inodos" RESET);
            return FALLO;
        }
    }
    return EXITO;
}

/**
 * Escribe un 0(libre) o 1(ocupado) en el bloque físico
 * que se pasa por parámetro.
 * @param   nbloque bit del MB a modificar.
 * @param   bit valor a escribir.
 * @return  0 o -1 si la inicializacion fue exitosa o no.
 */

int escribir_bit(unsigned int nbloque, unsigned int bit)
{
    struct superbloque SB;

    // leemos el superbloque para obtenir la posición del MB
    if (bread(posSB, &SB) == FALLO)
    {
        fprintf(stderr, RED "Error al leer el bloque\n" RESET);
        return FALLO;
    }
    int posbyte = nbloque / 8;
    int posbit = nbloque % 8;
    // posicion del bloque que contiene el byte
    int nbloqueMB = posbyte / BLOCKSIZE;
    // posición absoluta del bloque que buscamos
    int nbloqueabs = SB.posPrimerBloqueMB + nbloqueMB;
    unsigned char bufferMB[BLOCKSIZE];

    if (bread(nbloqueabs, bufferMB) == FALLO)
    {
        fprintf(stderr, RED "Error al leer el bloque\n" RESET);
        return FALLO;
    }
    // obtener la posición de este byte dentro del bufferMB
    posbyte = posbyte % BLOCKSIZE;

    unsigned char mascara = 128; // 10000000
    mascara >>= posbit;          // desplazamiento de bits a la derecha

    if (bit == 1)
    {
        bufferMB[posbyte] |= mascara; // Poner el byte a 1.
    }
    else if (bit == 0)
    {
        bufferMB[posbyte] &= ~mascara; // Poner el byte a 0.
    }
    else
    {
        fprintf(stderr, RED "Error, el bit a escribir debe ser 0 o 1\n" RESET);
        return FALLO;
    }

    if (bwrite(nbloqueabs, bufferMB) < 0)
    {
        fprintf(stderr, RED "Error al guardar los cambios en el SB  \n" RESET);
        return FALLO;
    }
    return EXITO;
}

/**
 * Escribe un 0(libre) o 1(ocupado) en el bloque
 * que se pasa por parámetro.
 * @param   nbloque bit del MB a leer.
 * @return  Devuelve el valor del nbloque en el MB.
 */

char leer_bit(unsigned int nbloque)
{
    struct superbloque SB;

    // leemos el superbloque para obtenir la posición del MB
    if (bread(posSB, &SB) == FALLO)
    {
        fprintf(stderr, RED "Error al leer el bloque\n" RESET);
        return FALLO;
    }
    int posbyte = nbloque / 8;
    int posbit = nbloque % 8;
    // posicion del bloque que contiene el byte
    int nbloqueMB = posbyte / BLOCKSIZE;
    // posición absoluta del bloque que buscamos
    int nbloqueabs = SB.posPrimerBloqueMB + nbloqueMB;
    unsigned char bufferMB[BLOCKSIZE];

    if (bread(nbloqueabs, bufferMB) == FALLO)
    {
        perror(RED "Error al leer el bloque\n" RESET);
        return FALLO;
    }
    // obtener la posición de este byte dentro del bufferMB
    int copyposbyte = posbyte;
    posbyte = posbyte % BLOCKSIZE;

    unsigned char mascara = 128;  // 10000000
    mascara >>= posbit;           // Desplazamiento del bit "posbit" veces a la derecha.
    mascara &= bufferMB[posbyte]; // operador AND
    mascara >>= (7 - posbit);     // desplazamos el bit al extremo derecho.

    printf(GRAY "[leer_bit(%i) → posbyte:%i, posbyte (ajustado): %i, posbit:%i, nbloqueMB:%i, nbloqueabs:%i)]\n\n" RESET, nbloque, copyposbyte, posbyte, posbit, nbloqueMB, nbloqueabs);

    return mascara;
}

/**
 * Reserva el primer bloque libre que encuentra
 * @return nº del bloque reservado o FALLO si no se pudo reservar un bloque
 */

int reservar_bloque(){

    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO)
    {
        fprintf(stderr, RED "Error al leer el superbloque\n" RESET);
        return FALLO;
    }
    // Hay bloques libres?
    if (SB.cantBloquesLibres > 0)
    { // SÍ
        // Encontrar el primer bloque libre
        unsigned char bufferMB[BLOCKSIZE];
        unsigned char bufferAux[BLOCKSIZE];
        memset(bufferAux, 255, BLOCKSIZE); // poner los bits de bufferAux a 1

        int nbloqueMB = SB.posPrimerBloqueMB;

        while (nbloqueMB <= SB.posUltimoBloqueMB)
        {
            if (bread(nbloqueMB, bufferMB) < 0)
            {
                perror(RED "Error al leer el bloque\n. " RESET);
                return FALLO;
            }
            if (memcmp(bufferMB, bufferAux, BLOCKSIZE) != 0)
            { // bloque libre
                break;
            }
            nbloqueMB++;
        }
        // ahora buefferMB contiene el byte con algún bit a 0
        // localizamos de cúal byte se trata
        int posbyte = 0;
        for (int i = 0; i < BLOCKSIZE; i++)
        {
            if (bufferMB[i] != 255)
            {
                posbyte = i;
                break;
            }
        }

        // localisamos cual bit está a 0 del byte (posbyte)
        unsigned char mascara = 128; // 10000000
        int posbit = 0;
        while (bufferMB[posbyte] & mascara)
        {                            // operador AND para bits
            bufferMB[posbyte] <<= 1; // desplazamiento hacia la izequierda de bits
            posbit++;
        }

        // nº de bloque físico a reservar
        int nbloque = ((nbloqueMB - SB.posPrimerBloqueMB) * BLOCKSIZE + posbyte) * 8 + posbit;
        // escrbimos el bit a 1 para indicar que está reservado
        if (escribir_bit(nbloque, 1) < 0)
        {
            fprintf(stderr, RED "Error al escribir el bit 1 para reservar el bloque\n" RESET);
            return FALLO;
        }
        // decrementamos la cantidad de bloques libres
        SB.cantBloquesLibres--;

        // limpiamos ese bloque en la zona de datos
        int posVirtual = nbloque + SB.posPrimerBloqueDatos - 1;
        memset(bufferAux, 0, BLOCKSIZE);
        if (bwrite(posVirtual, bufferAux) < 0)
        {
            fprintf(stderr, RED "Error al limpiar el bloque reservado en al zona de datos\n" RESET);
            return FALLO;
        }

        if (bwrite(posSB, &SB) < 0)
        {
            fprintf(stderr, RED "Error al guardar los cambios en el SB  \n" RESET);
            return FALLO;
        }

        // devolver el nª del bloque reservado
        return nbloque;
    }
    else
    { // No hay bloques libres
        fprintf(stderr, RED "No hay bloques libres.!\n" RESET);
        return FALLO;
    }
}

int liberar_bloque(unsigned int nbloque)
{
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO)
    {
        fprintf(stderr, RED "Error al leer el superbloque\n" RESET);
        return FALLO;
    }
    // Escribimos el bit a 0 para indicar que esta libre
    if (escribir_bit(nbloque, 0) < 0)
    {
        fprintf(stderr, RED "Error al escribir 0 para liberar el bloque\n" RESET);
        return FALLO;
    }
    SB.cantBloquesLibres++;
    // guardar el SB modificado
    if (bwrite(posSB, &SB) < 0)
    {
        fprintf(stderr, RED "Error al guardar los cambios en el SB  \n" RESET);
        return FALLO;
    }

    return nbloque;
}

/**
 * Escribir ninodo del AI para volcarlo en inodo
 * @param ninodo    posición del inodo en el array de inodos
 * @param inodo     el inodo a leer del array de inodos
 * @return  EXITO si todo ha ido bien. FALLO en caso contrario
 *
 */
int escribir_inodo(unsigned int ninodo, struct inodo *inodo)
{
    struct superbloque SB;
    // leer el superbloque
    if (bread(posSB, &SB) == FALLO)
    {
        fprintf(stderr, RED "Error al leer el superbloque\n" RESET);
        return FALLO;
    }

    // obtener el nº de bloque de AI que contiene ninodo
    int nInodosPorBloque = BLOCKSIZE / INODOSIZE;

    int nbloqueAI = ninodo / nInodosPorBloque;
    int nbloqueabs = nbloqueAI + SB.posPrimerBloqueAI;
    struct inodo inodos[BLOCKSIZE / INODOSIZE];
    if (bread(nbloqueabs, inodos) < 0)
    {
        fprintf(stderr, RED "Error al leer el inodo\n" RESET);
        return FALLO;
    }
    // posición absoluta del inodo

    inodos[ninodo % (nInodosPorBloque)] = *inodo;
    if (bwrite(nbloqueabs, inodos) < 0)
    {
        fprintf(stderr, RED "Error al guardar los cambios en el SB  \n" RESET);
        return FALLO;
    }
    return EXITO;
}

/**
 * Lee ninodo del AI para volcarlo en inodo
 * @param ninodo    posición del inodo en el array de inodos
 * @param inodo     el inodo a leer del array de inodos
 * @return  EXITO si todo ha ido bien. FALLO en caso contrario
 *
 */

int leer_inodo(unsigned int ninodo, struct inodo *inodo)
{
    // leemos el SB para localizar el AI
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO)
    {
        fprintf(stderr, RED "Error al leer el superbloque\n" RESET);
        return FALLO;
    }
    // obtener el nº de bloque de AI que contiene ninodo
    int nInodosPorBloque = BLOCKSIZE / INODOSIZE;
    int nbloqueAI = ninodo / nInodosPorBloque;
    // posición absoluta del inodo
    int nbloqueabs = nbloqueAI + SB.posPrimerBloqueAI;

    struct inodo inodos[nInodosPorBloque];
    if (bread(nbloqueabs, inodos) < 0)
    {
        perror(RED "Error al leer el inodo\n" RESET);
        return FALLO;
    }
    // guardar el contenido en el inodo
    *inodo = inodos[ninodo % nInodosPorBloque];

    return EXITO;
}
/**
 * Reserva el primer inodo libre que encuentra
 *
 * @param tipo  tipo de inodo
 * @param permisos  los permisos a asignar al inodo que reservaremos
 * @return posInodoReservado o FALLO si la reserva del inodo ha fallado.
 */

int reservar_inodo(unsigned char tipo, unsigned char permisos)
{

    int posInodoReservado = -1;

    struct superbloque SB;
    // leemos el superbloque
    if (bread(posSB, &SB) == FALLO)
    {
        fprintf(stderr, RED "Error al leer el superbloque\n" RESET);
        return FALLO;
    }

    if (SB.cantInodosLibres == 0)
    {
        fprintf(stderr, RED "No hay inodos libres para reservar\n" RESET);
        return FALLO;
    }

    posInodoReservado = SB.posPrimerInodoLibre;
    // actualizar la lista de inodos libres
    // dado que los inodos libres son en posiciones contiguas el siguiente inodo
    // libre está en SB.posPrimerInodoLibre+1
    SB.posPrimerInodoLibre++;
    // inicializamos los campos de inodo reservado
    struct inodo inodoReservado;
    inodoReservado.tipo = tipo;
    inodoReservado.permisos = permisos;
    inodoReservado.nlinks = 1;
    inodoReservado.tamEnBytesLog = 0;
    inodoReservado.numBloquesOcupados = 0;
    inodoReservado.ctime = time(NULL);
    inodoReservado.atime = time(NULL);
    inodoReservado.mtime = time(NULL);

    for (int i = 0; i < 12; i++)
    {
        inodoReservado.punterosDirectos[i] = 0;
    }
    for (int i = 0; i < 3; i++)
    {
        inodoReservado.punterosIndirectos[i] = 0;
    }

    // escribir el inodo
    if (escribir_inodo(posInodoReservado, &inodoReservado) < 0)
    {
        fprintf(stderr, RESET "Error al escribir el inodo\n" RESET);
        return FALLO;
    }
    // decrementar la cantidad de inodos libres
    SB.cantInodosLibres--;
    // reescribir el SB para guardar los cambios realizados
    if (bwrite(posSB, &SB) < 0)
    {
        fprintf(stderr, RED "Error al guardar los cambios en el SB  \n" RESET);
        return FALLO;
    }
    return posInodoReservado;
}

/**
 * Calculo el rango del bloque lógico
 * @param inodo estructura del inodo
 * @param nblogico bloque logico
 * @param ptr direccion del puntero
 */

int obtener_nRangoBL(struct inodo *inodo, unsigned int nblogico, unsigned int *ptr)
{
    if (nblogico < DIRECTOS)
    {
        *ptr = inodo->punterosDirectos[nblogico];
        return 0;
    }
    else if (nblogico < INDIRECTOS0)
    {
        *ptr = inodo->punterosIndirectos[0];
        return 1;
    }
    else if (nblogico < INDIRECTOS1)
    {
        *ptr = inodo->punterosIndirectos[1];
        return 2;
    }
    else if (nblogico < INDIRECTOS2)
    {
        *ptr = inodo->punterosIndirectos[2];
        return 3;
    }
    else
    {
        *ptr = 0;
        fprintf(stderr, RED "Error al obtener el rango del bloque lógico\n" RESET);
        return FALLO;
    }
}

/**
 * Calcula el índice del bloque de punteros.
 *
 * @param nblogico, bloque lógico.
 * @param nivel_punteros, nivel del que cuelgan el bloque de punteros.
 * @return el índice del bloque de punteros o FALLO si los parámetros son incorrectos.
 */

int obtener_indice(unsigned int nblogico, int nivel_punteros)
{

    if (nblogico < DIRECTOS)
    { // Dentro de los bloques directos.
        return nblogico;
    }
    else if (nblogico < INDIRECTOS0)
    { // Dentro de los bloques indirectos 0.
        return nblogico - DIRECTOS;
    }
    else if (nblogico < INDIRECTOS1)
    { // Dentro de los bloques indirectos 1.

        if (nivel_punteros == 2)
        {

            return ((nblogico - INDIRECTOS0) / NPUNTEROS);
        }
        else if (nivel_punteros == 1)
        {

            return ((nblogico - INDIRECTOS0) % NPUNTEROS);
        }
        else
        {
            fprintf(stderr, RED "Error, el nivel del puntero no es correcto.\n" RESET);
            return FALLO;
        }
    }
    else if (nblogico < INDIRECTOS2)
    { // Dentro de los bloques indirectos 2.
        if (nivel_punteros == 3)
        {

            return ((nblogico - INDIRECTOS1) / (NPUNTEROS * NPUNTEROS));
        }
        else if (nivel_punteros == 2)
        {

            return (((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) / NPUNTEROS);
        }
        else if (nivel_punteros == 1)
        {

            return (((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) % NPUNTEROS);
        }
        else
        {

            fprintf(stderr, RED "Error, el nivel del puntero no es correcto.  \n" RESET);
            return FALLO;
        }
    }
    else
    {
        fprintf(stderr, RED "Error, El puntero no está dentro del rango máximo.  \n" RESET);
        return FALLO;
    }
}

/** Obtiene el nº del bloque físico correspondiente a un bloque lógico determinado
 * del indo especificado.
 * @param   inodo       inodo del bloque logico
 * @param   nblogic     nº del bloque lógico
 * @param   reservar    indica el modo de uso de este metodo
 * reservar = 0 --> consultas
 * reservar = 1 --> consultas para bloques existentes, y reservación de bloque si el bloque no existe.
 * @return  el bloque fisico deseado o FALLO si la traducción no fue existosa
 */

int traducir_bloque_inodo(struct inodo *inodo, unsigned int nblogico, unsigned char reservar)
{
    unsigned int ptr, ptr_ant;
    int nRangoBL, nivel_punteros, indice;
    unsigned int buffer[NPUNTEROS];

    ptr = 0;
    ptr_ant = 0;
    nRangoBL = obtener_nRangoBL(inodo, nblogico, &ptr); // 0:D, 1:I0, 2:I1, 3:I2

    if (nRangoBL < 0)
    {
        fprintf(stderr, RED "Error obtener_RangoBL\n" RESET);
        return FALLO;
    }

    nivel_punteros = nRangoBL; // nivel mas alto es el que cuelga directamente del inodo
    while (nivel_punteros > 0)
    { // iteramos por todos los niveles de punteros
        if (ptr == 0)
        { // no cuelgan punteros de bloques
            if (reservar == 0)
            { // hacer consultas
                // no notificamos el error
                // bloque inexistente
                return FALLO;
            }
            else
            {
                // reservar bloques de punteros y crear punteros del inodo a los bloques de datos
                ptr = reservar_bloque();
                if (ptr < 0)
                {
                    fprintf(stderr, RED "Error de reservar_bloque() en traduicr_bloque_inodo\n" RESET);
                    return FALLO;
                }
                // inodo->numBloquesOcupados++;
                inodo->numBloquesOcupados = inodo->numBloquesOcupados + 1;

                inodo->ctime = time(NULL); // actualizar el ctime ya que estamos modificando datos del inodo
                                           // el bloque cuelga directamente del inodo
                if (nivel_punteros == nRangoBL)
                {
#if DEBUGN5 || DEBUGCP
                    printf(GRAY "\n[traducir_bloque_inodo()→ inodo.punterosIndirectos[%i] = %i (reservado BF %i para punteros_nivel%i)]\n" RESET, nRangoBL - 1, ptr, ptr, nivel_punteros);
#endif
                    inodo->punterosIndirectos[nRangoBL - 1] = ptr;
                }
                else
                {
                    // el bloque cuelga de otro bloque de punteros
                    buffer[indice] = ptr;
#if DEBUGN5
                    printf(GRAY "[traducir_bloque_inodo()→ punteros_nivel%i[%i] = %i (reservado BF %i para punteros_nivel%i)]\n" RESET, nivel_punteros + 1, indice, ptr, ptr, nivel_punteros);
#endif

                    if (bwrite(ptr_ant, buffer) < 0)
                    {
                        fprintf(stderr, RED "Error al escribir en el buffer\n" RESET);
                        return FALLO;
                    }
                }
                if (memset(buffer, 0, BLOCKSIZE) == NULL)
                {
                    perror("memset()");
                    return FALLO;
                }
            }
        }
        else
        {
            if (bread(ptr, buffer) < 0)
            {
                fprintf(stderr, RED " Error al leer del buffer\n" RESET);
                return FALLO;
            }
        }
        indice = obtener_indice(nblogico, nivel_punteros);
        if (indice < 0)
        {
            fprintf(stderr, RED "Indice invalido\n" RESET);
            return FALLO;
        }
        ptr_ant = ptr;        // actualizar el puntero
        ptr = buffer[indice]; // desplazar el puntero al siguiente nivel
        nivel_punteros--;
    }

    // estamos al nivel de datos
    if (ptr == 0)
    {
        if (reservar == 0)
        {
            // error de lectura, bloque inexistente
            return FALLO;
        }
        else
        {
            ptr = reservar_bloque();
            if (ptr < 0)
            {
                fprintf(stderr, RED "Error de reservar_bloque() en traduicr_bloque_inodo\n" RESET);
                return FALLO;
            }
            inodo->numBloquesOcupados = inodo->numBloquesOcupados + 1;
            inodo->ctime = time(NULL);
            if (nRangoBL == 0)
            { // puntero directo
#if DEBUGN5 || DEBUGCP
                printf(GRAY "\n[traducir_bloque_inodo()→ punterosDirectos[%i] = %i (reservado BF %i para BL %i)]\n" RESET, nblogico, ptr, ptr, nblogico);
#endif
                inodo->punterosDirectos[nblogico] = ptr; // asignamos la direción del bl. de datos en el inodo
            }
            else
            {
                buffer[indice] = ptr; // asignamos la dirección del bloque de datos en el buffer
#if DEBUGN5
                printf(GRAY "[traducir_bloque_inodo()→ punteros_nivel1[%i] = %i (reservado BF %i para BL %i)]\n\n" RESET, indice, ptr, ptr, nblogico);
#endif
                if (bwrite(ptr_ant, buffer) < 0)
                { // salvamos en el dispositivo el buffer de punteros modificado
                    fprintf(stderr, RED "Error al escribir en el buffer bwrite(ptr_ant, buffer) \n" RESET);
                    return FALLO;
                }
            }
        }
    }

    // mi_write_f salvará los cambios del inodo

    return ptr;
}

/**
 * Libera un inodo y los bloques de datos  y de índices que ocupaba
 * @param   nº del inodo a liberar
 * @return  nº de inodo liberao o FALLO si ha ido algo mal
 */
int liberar_inodo(unsigned int ninodo)
{

    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO)
    {
        fprintf(stderr, RED "liberar_inodo: Error lectura del superbloque\n" RESET);
        return FALLO;
    }

    // Hay inodos para liberar?
    if (SB.cantInodosLibres == SB.totInodos)
    {
        fprintf(stderr, RED "liberar_inodo: No hay inodos para liberar\n" RESET);
        return FALLO;
    }
    struct inodo inodo;

    // leemos el inodo
    if (leer_inodo(ninodo, &inodo) == FALLO)
    {
        perror(RED "Error al leer el inodo\n" RESET);
        return FALLO;
    }

    // llamada a la función que libera todos los bloques del inodo

    int bloques_liberados = liberar_bloques_inodo(0, &inodo);
    if (bloques_liberados == FALLO)
    {
        fprintf(stderr, RED "Error al liberar los bloques del inodo %i\n" RESET, ninodo);
        return FALLO;
    }
    // actualizar el valor de bloques ocupados del inodo
    inodo.numBloquesOcupados -= bloques_liberados;
    if (inodo.numBloquesOcupados > 0)
    {
        fprintf(stderr, RED "No se han borrado todos los blqoues del inodo %i\n" RESET, ninodo);
        return FALLO;
    }

    // Leer el superbloque actualizado por liberar_bloques_inodo
    if (bread(posSB, &SB) == FALLO)
    {
        fprintf(stderr, RED "Error al leer el superbloque en liberar_inodo()\n" RESET);
        return FALLO;
    }

    inodo.tipo = 'l'; // marcar el inodo como libre
    inodo.tamEnBytesLog = 0;

    unsigned int tmp = SB.posPrimerInodoLibre;
    SB.posPrimerInodoLibre = ninodo;
    inodo.punterosDirectos[0] = tmp;
    SB.cantInodosLibres++; // incrementar la cantidad de inodos libres

    if (bwrite(posSB, &SB) == FALLO)
    {
        fprintf(stderr, RED "Error al guardar los cambios en el SB  \n" RESET);
        return FALLO;
    }

    // actaulizamos el tiempo de modificación del inodo
    inodo.ctime = time(NULL);
    // guardar los cambios del inodo
    if (escribir_inodo(ninodo, &inodo) < 0)
    {
        perror(RED "Error al escribir el inodo" RESET);
        return FALLO;
    }
    return ninodo;
}

/**
 * Libera todos los bloques ocupados a partir de un bloque lógico dado.
 * @param   primerBl    primer bloque lógico
 * @param   inodo       inodo que liberaremos sus bloques ocupados
 * @return              nº de bloques liberados
 *
 */

// int liberar_bloques_inodo(unsigned int primerBL, struct inodo *inodo)
// {

//     unsigned int nivel_punteros, indice, ptr, nBL, ultimoBL;
//     int nRangoBL;
//     unsigned int bloques_punteros[3][NPUNTEROS]; // array de bloques de punteros
//     unsigned char bufAux_punteros[BLOCKSIZE];
//     int ptr_nivel[3];  // punteros a bloques de punteros de cada nivel
//     int indices[3];    // indices de cada nivel
//     int liberados = 0; // nº de bloques liberados
//     int breads = 0;
//     int bwrites = 0;
//     int nblPrevia = 0;
//     liberados = 0;
//     if (inodo->tamEnBytesLog == 0)
//     {
//         return 0;
//     } // el fichero está vacío
//     // obtenemos el último bloque lógico del inodo
//     if (inodo->tamEnBytesLog % BLOCKSIZE == 0)
//     {
//         ultimoBL = ((inodo->tamEnBytesLog) / BLOCKSIZE) - 1;
//     }
//     else
//     {
//         ultimoBL = (inodo->tamEnBytesLog) / BLOCKSIZE;
//     }
//     if (memset(bufAux_punteros, 0, BLOCKSIZE)==NULL){
//         perror ("liberar_bloques_inodo: Error memset");
//         return FALLO;
//     }
//     ptr = 0;

//     fprintf(stdout,GRAY"[liberar_bloques_inodo()-> primerBL: %d, ultimoBL: %d]\n"RESET, primerBL, ultimoBL);

//     for (nBL = primerBL; nBL <= ultimoBL; nBL++)
//     {                                                  // recorrido BLs
//         nRangoBL = obtener_nRangoBL(inodo, nBL, &ptr); // 0:D, 1:I0, 2:I1, 3:I2
//         if (nRangoBL < 0)
//         {
//             fprintf(stderr, RED "liberar_bloques_inodo: rango negativo\n" RESET);
//             return FALLO;
//         }
//         nivel_punteros = nRangoBL; // el nivel_punteros +alto cuelga del inodo

//         while (ptr > 0 && nivel_punteros > 0)
//         { // cuelgan bloques de punteros
//             indice = obtener_indice(nBL, nivel_punteros);
//             if (indice == 0 || nBL == primerBL)
//             {
//                 // solo hay que leer del dispositivo si no está ya cargado previamente en un buffer
//                 if (bread(ptr, bloques_punteros[nivel_punteros - 1]) == -1)
//                 {
//                     fprintf(stderr, RED "liberar_bloques_inodo: error en bread\n" RESET);
//                     return FALLO;
//                 }
//                 breads++;
//             }
//             ptr_nivel[nivel_punteros - 1] = ptr;
//             indices[nivel_punteros - 1] = indice;
//             ptr = bloques_punteros[nivel_punteros - 1][indice];
//             nivel_punteros--;
//         }

//         if (ptr > 0)
//         { // si existe bloque de datos
//             if (liberar_bloque(ptr) == -1)
//             {
//                 fprintf(stderr, RED "liberar_bloques_inodo: error en liberar_bloque %d\n" RESET, ptr);
//                 return FALLO;
//             }
//             liberados++;
//             #if DEBUGN6
//             fprintf(stdout, GRAY "[liberar_bloques_inodo()-> liberado BF %d de datos par a BL %d]\n" RESET, ptr, nBL);
//             #endif
//             if (nRangoBL == 0)
//             { // es un puntero Directo
//                 inodo->punterosDirectos[nBL] = 0;
//             }
//             else
//             {
//                 nivel_punteros = 1;
//                 while (nivel_punteros <= nRangoBL)
//                 {
//                     indice = indices[nivel_punteros - 1];
//                     bloques_punteros[nivel_punteros - 1][indice] = 0;
//                     ptr = ptr_nivel[nivel_punteros - 1];
//                     if (memcmp(bloques_punteros[nivel_punteros - 1], bufAux_punteros, BLOCKSIZE) == 0)
//                     {
//                         // No cuelgan más bloques ocupados, hay que liberar el bloque de punteros
//                         if (liberar_bloque(ptr) == -1)
//                         {
//                             fprintf(stderr, RED "liberar_bloques_inodo: error en liberar_bloque %d\n" RESET, ptr);
//                             return FALLO;
//                         }
//                         liberados++;
//                         #if DEBUGN6
//                         fprintf(stdout, GRAY "[liberar_bloques_inodo()→ liberado BF %i de punteros_nivel%i correspondiente al BL: %i]\n" RESET, ptr, nivel_punteros, nBL);
//                         #endif
//                          //  Incluir primera mejora para saltar los bloques que no sea necesario explorar !!!
                       
//                       nblPrevia = nBL;
//                         if (nivel_punteros == 1)
//                         {
//                             //salta los bloques que cleugan de los bloques indices de
//                             nBL += NPUNTEROS - indices[nivel_punteros - 1] - 1;
                           
//                         }
//                         else if (nivel_punteros == 2)
//                         {
//                             nBL += NPUNTEROS * (NPUNTEROS - indices[nivel_punteros - 1]) -NPUNTEROS;
//                         }
                      
                        
//                         #if DEBUGN6
//                          fprintf(stderr, GREEN "Del BL %i saltamos hasta BL %i\n" RESET, nblPrevia, nBL);
//                         #endif

//                         if (nivel_punteros == nRangoBL)
//                         {
//                             inodo->punterosIndirectos[nRangoBL - 1] = 0;
//                         }
//                         nivel_punteros++;
//                     }
//                     else
//                     { // escribimos en el dispositivo el bloque de punteros modificado
//                         if (bwrite(ptr, bloques_punteros[nivel_punteros - 1]) == FALLO)
//                         {
//                             fprintf(stderr, RED "liberar_bloques_inodo: error en bwrite del bloque %d\n" RESET, ptr);
//                             return FALLO;
//                         }
//                         #if DEBUGN6
//                         fprintf(stdout, YELLOW "[liberar_bloques_inodo()→ salvado BF %i de punteros_nivel%i correspondiente al BL %i\n"RESET, ptr, nivel_punteros, nBL);
//                         #endif
//                         bwrites++;
//                         // hemos de salir del bucle ya que no será necesario liberar los bloques de niveles
//                         // superiores de los que cuelga
//                         nivel_punteros = nRangoBL + 1;
//                     }
//                 }
//             }
//         }
//         //incluir mejora 2
//     }
//     #if DEBUGN6
//     printf(BLUE "[liberar_bloques_inodo()-> total bloques liberados: %d, total breads: %d, total bwrites: %d]\n" RESET, liberados, breads, bwrites);
//     #endif
//     return liberados;
// }
                



//version recursiva---------------------------------------------------

int liberar_bloques_inodo(unsigned int primerBL, struct inodo *inodo) 
{
    int contador_breads=0, contador_bwrites=0, nRangoBL= 0, liberados =0, eof=0;
    unsigned int ultimoBL, ptr = 0, nivel_punteros = 0;

    if (inodo->tamEnBytesLog == 0) return 0;
    if (inodo->tamEnBytesLog % BLOCKSIZE == 0) ultimoBL = inodo->tamEnBytesLog / BLOCKSIZE - 1;
    else ultimoBL = inodo->tamEnBytesLog / BLOCKSIZE;

    #if DEBUGN6
    fprintf(stderr, BLUE "[liberar_bloques_inodo() -> primerBL: %d, ultimoBL: %d]\n" RESET, primerBL, ultimoBL);
    #endif

    unsigned int nBL = primerBL;

while (!eof)
    {
        nRangoBL = obtener_nRangoBL(inodo, nBL, &ptr);
        nivel_punteros = nRangoBL;

        liberados += liberar_bloques_auxiliar(
            &nBL, primerBL, ultimoBL, inodo, 
            nRangoBL, nivel_punteros, &ptr, &eof, &contador_breads, &contador_bwrites
        );
    }
    #if DEBUGN6
    fprintf(stderr, BLUE "[liberar_bloques_inodo() -> total bloques liberados: %d, total_breads: %d, total_bwrites: %d]\n" RESET, liberados, contador_breads, contador_bwrites);
    #endif

    return liberados;
}



int liberar_bloques_auxiliar(unsigned int *nBL, unsigned int primerBL, unsigned int ultimoBL, struct inodo *inodo, int nRangoBL, unsigned int nivel_punteros, unsigned int *ptr, int *eof, int *contador_breads, int *contador_bwrites) 
{
    int liberados = 0;
    unsigned int bloquePunteros[NPUNTEROS];
    unsigned int bloquePunteros_Aux[NPUNTEROS];
    unsigned int bufferCeros[NPUNTEROS];
    int indice_inicial;

    memset(bufferCeros, 0, BLOCKSIZE);

        if (nRangoBL == 0) {//bloques directos
        for (int d = *nBL; d < DIRECTOS && !*eof; d++) {
            if (inodo->punterosDirectos[d]) {
                liberar_bloque(inodo->punterosDirectos[d]);
                #if DEBUGN6
                fprintf(stderr, GRAY "[liberar_bloques() -> liberado BF %d de datos para BL %d]\n" RESET, inodo->punterosDirectos[d], *nBL);
                #endif
                inodo->punterosDirectos[d] = 0;//marcar como libre
                liberados++;
            }
            *nBL = *nBL + 1;
            if (*nBL > ultimoBL) *eof = 1; //fin del archivo
        }

        return liberados;
    } 

    if (*ptr) { //si cuelga un bloque de punteros
        indice_inicial = obtener_indice(*nBL, nivel_punteros);
        if (indice_inicial==0 || *nBL == primerBL) 
        {//solo leemos bloque si no estaba cargado

            if (bread(*ptr, bloquePunteros) == FALLO) return FALLO;
            *contador_breads= *contador_breads+1;
            // Guardamos copia del bloque para ver si hay cambios
            memcpy(bloquePunteros_Aux, bloquePunteros, BLOCKSIZE); 
        }   
                
                //exploramos el bloque de punteros iterando el índice
        for (int i = indice_inicial; i < NPUNTEROS && !*eof; i++) {
            if (bloquePunteros[i] != 0)  {
                if (nivel_punteros == 1) {
                    liberar_bloque(bloquePunteros[i]); //de datos
                    #if DEBUGN6
                        fprintf(stderr, GRAY"[liberar_indirectos_recursivo() -> liberado BF %d de datos para BL %d]\n" RESET, bloquePunteros[i], *nBL);
                    #endif
                    bloquePunteros[i] = 0;
                    liberados++;
                    *nBL = *nBL + 1;
                } 
                //llamada recursiva para explorar el nivel siguiente de punteros hacia los datos
                else liberados += liberar_bloques_auxiliar(nBL, primerBL, ultimoBL, inodo, nRangoBL, nivel_punteros-1, &bloquePunteros[i], eof, contador_breads, contador_bwrites);
            } else {
               //bloquePunteros[i]=0    
               // Saltos al valer 0 un puntero según nivel
 
                if (nivel_punteros==1){
                    *nBL = *nBL+1 ;
                }else if (nivel_punteros==2){
                  *nBL += NPUNTEROS; 

                }else{
                 *nBL += NPUNTEROS*NPUNTEROS; 

                }
            }
            
            if (*nBL > ultimoBL) *eof = 1; // Comprobamos si hemos llegado al fin del archivo

        }
       // Si el bloque de punteros es distinto al original
        if (memcmp(bloquePunteros, bloquePunteros_Aux, BLOCKSIZE) != 0)  {   
            //si quedan punteros != 0 en el bloque lo salvamos
            if (memcmp(bloquePunteros, bufferCeros, BLOCKSIZE) != 0) 
            {
                bwrite(*ptr, bloquePunteros);
                #if DEBUGN6
                    *contador_bwrites = *contador_bwrites+1;
                    fprintf(stderr, ORANGE "[liberar_indirectos_recursivo() -> salvado BF %d de punteros_nivel%d correspondiente al BL %d]\n" RESET, *ptr, nivel_punteros, *nBL); 
                #endif
            } else{
                liberar_bloque(*ptr); //de punteros
                #if DEBUGN6
                    fprintf(stderr, GRAY "[liberar_indirectos_recursivo() -> liberado BF %d de punteros_nivel%d correspondiente al BL %d]\n" RESET, *ptr, nivel_punteros, *nBL);
                #endif
                *ptr = 0;//ponemos a 0 el puntero que apuntaba al bloque liberado
                liberados++;
            }
        }
    }
    else 
    {//*ptr=0
        switch (nRangoBL) {
            case 1:
             *nBL = INDIRECTOS0; 
             break;
            case 2:
             *nBL = INDIRECTOS1; 
             break;
            case 3: 
            *nBL = INDIRECTOS2; 
            break;
        } 
    }
    
    return liberados;
}
/**
Autores: Khaoula Ikkene, Tomás Gallardo Rago, Francesc Gayá Piña  
**/
#include "simulacion.h"

struct INFORMACION {
  int pid;
  unsigned int nEscrituras; //validadas 
  struct REGISTRO PrimeraEscritura;
  struct REGISTRO UltimaEscritura;
  struct REGISTRO MenorPosicion;
  struct REGISTRO MayorPosicion;
};
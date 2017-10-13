#include <stdint.h>

//extern int valvestate;

// Liest Daten von der Vitodens:
int vito_dtr_read( int location, int size, uint8_t *vitomem );

// Schreibt Daten an die Vitodens:
int vito_dtr_write( int location, int size, uint8_t *vitomem );

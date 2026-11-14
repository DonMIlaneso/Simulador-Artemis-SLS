#ifndef ESCENA_H
#define ESCENA_H

#include <windows.h>
#include "fisica.h"

/* Todo el dibujo se deriva del estado. Ninguna funcion altera la fisica. */
void dibujar_escena(HDC dc, RECT area, const Simulacion *sim,
                    double reloj_visual, HFONT fuente, HFONT mono);

#endif

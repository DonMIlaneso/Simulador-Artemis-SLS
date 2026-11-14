#ifndef FISICA_H
#define FISICA_H

#include <stddef.h>

#define MAX_EVENTOS 4
#define ERROR_CAPACIDAD 256
#define A_MIN 0.1
#define A_MAX 60.0
#define T_MIN 1.0
#define T_MAX 600.0
#define H_MIN 1000.0
#define H_MAX 2000000.0
#define G_MIN 1.0
#define G_MAX 25.0

typedef struct { double a1, t1, a2, h, g; } Parametros;
typedef struct {
    double y1, v1, v2, dt2, t_meco, h_max, t_apogeo, factor_g;
    int desacople_previo;
} Resultados;
typedef enum { FASE_SRB, FASE_NUCLEO, FASE_BALISTICA, FASE_APOGEO } Fase;
typedef struct { double t, y, v, a; Fase fase; } Estado;
typedef enum { EVENTO_DESPEGUE, EVENTO_DESACOPLE, EVENTO_MECO, EVENTO_APOGEO } TipoEvento;
typedef struct { double t; TipoEvento tipo; } Evento;
typedef struct {
    Parametros p;
    Resultados r;
    Estado estado;
    Evento eventos[MAX_EVENTOS];
    size_t cantidad_eventos;
    double t, t_fin, velocidad;
    int iniciado, corriendo, balistico, camara_lenta;
} Simulacion;

Parametros parametros_nominales(void);
/* Se aceptan punto o coma decimal, espacios exteriores y exponentes. */
int convertir_numero(const char *texto, double minimo, double maximo, double *salida);
int calcular_mision(const Parametros *p, Resultados *r, char *error, size_t capacidad);
int estado_en_instante(const Parametros *p, const Resultados *r, double t, Estado *e);
/* Una configuracion invalida no modifica la simulacion anterior. */
int preparar_simulacion(Simulacion *s, const Parametros *p, int balistico,
                       char *error, size_t capacidad);
void lanzar_simulacion(Simulacion *s);
void alternar_pausa(Simulacion *s);
int fijar_tiempo(Simulacion *s, double t);
/* dt_real en segundos; cada avance se acota a 0.05 s. */
int avanzar_reloj(Simulacion *s, double dt_real);
size_t eventos_visibles(const Simulacion *s);
const char *nombre_evento(TipoEvento tipo);
const char *nombre_fase(Fase fase);
const char *nombre_capa(double altitud);

#endif

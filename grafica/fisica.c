#include "fisica.h"

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(MAX_EVENTOS >= 4, "Se necesitan hasta cuatro eventos.");

static int finito(double valor)
{
    /* Tambien rechaza NaN: sus comparaciones son falsas. */
    return valor >= -DBL_MAX && valor <= DBL_MAX;
}

static int rango(double valor, double minimo, double maximo)
{
    return finito(valor) && valor >= minimo && valor <= maximo;
}

static int error_calculo(char *error, size_t capacidad, const char *mensaje)
{
    if (error != NULL && capacidad > 0U) {
        (void)snprintf(error, capacidad, "%s", mensaje);
    }
    return 0;
}

Parametros parametros_nominales(void)
{
    const Parametros p = {20.0, 120.0, 12.5, 160000.0, 9.8};
    return p;
}

int convertir_numero(const char *texto, double minimo, double maximo, double *salida)
{
    char limpio[128];
    char *fin;
    size_t longitud;
    size_t usados = 0U;
    double valor;

    if (texto == NULL || salida == NULL || !finito(minimo) || !finito(maximo)
        || minimo > maximo) {
        return 0;
    }
    longitud = strlen(texto);
    if (longitud >= sizeof limpio) {
        return 0;
    }
    while (isspace((unsigned char)*texto)) {
        ++texto;
    }
    longitud = strlen(texto);
    while (longitud > 0U && isspace((unsigned char)texto[longitud - 1U])) {
        --longitud;
    }
    if (longitud == 0U) {
        return 0;
    }
    for (size_t i = 0U; i < longitud; ++i) {
        char c = texto[i];
        if (!isdigit((unsigned char)c) && c != '+' && c != '-'
            && c != '.' && c != ',' && c != 'e' && c != 'E') {
            return 0;
        }
        limpio[usados++] = c == ',' ? '.' : c;
    }
    limpio[usados] = '\0';
    errno = 0;
    valor = strtod(limpio, &fin);
    if (fin == limpio || *fin != '\0' || errno == ERANGE
        || !rango(valor, minimo, maximo)) {
        return 0;
    }
    *salida = valor;
    return 1;
}

int calcular_mision(const Parametros *p, Resultados *r, char *error, size_t capacidad)
{
    Resultados nuevo = {0};
    double altitud_teorica;
    double velocidad_cuadrada;

    if (p == NULL || r == NULL) {
        return error_calculo(error, capacidad, "Faltan parametros o destino de resultados.");
    }
    if (!rango(p->a1, A_MIN, A_MAX) || !rango(p->a2, A_MIN, A_MAX)) {
        return error_calculo(error, capacidad, "a1 y a2 deben estar entre 0.1 y 60 m/s^2.");
    }
    if (!rango(p->t1, T_MIN, T_MAX)) {
        return error_calculo(error, capacidad, "t1 debe estar entre 1 y 600 segundos.");
    }
    if (!rango(p->h, H_MIN, H_MAX)) {
        return error_calculo(error, capacidad, "h debe estar entre 1000 y 2000000 metros.");
    }
    if (!rango(p->g, G_MIN, G_MAX)) {
        return error_calculo(error, capacidad, "g debe estar entre 1 y 25 m/s^2.");
    }
    altitud_teorica = 0.5 * p->a1 * p->t1 * p->t1;
    if (!finito(altitud_teorica)) {
        return error_calculo(error, capacidad, "La altitud intermedia no es representable.");
    }
    if (altitud_teorica <= p->h) {
        nuevo.desacople_previo = 1;
        nuevo.y1 = altitud_teorica;
        nuevo.v1 = p->a1 * p->t1;
        velocidad_cuadrada = nuevo.v1 * nuevo.v1 + 2.0 * p->a2 * (p->h - nuevo.y1);
        nuevo.v2 = sqrt(velocidad_cuadrada);
        /* Forma equivalente a (v2-v1)/a2, estable si las velocidades son cercanas. */
        nuevo.dt2 = 2.0 * (p->h - nuevo.y1) / (nuevo.v2 + nuevo.v1);
        nuevo.t_meco = p->t1 + nuevo.dt2;
        if (nuevo.dt2 > 0.0 && nuevo.t_meco <= p->t1) {
            return error_calculo(error, capacidad, "La fase 2 es demasiado breve para distinguir su tiempo.");
        }
    } else {
        /* Caso limite: se alcanza h antes del desacople previsto. */
        nuevo.desacople_previo = 0;
        nuevo.t_meco = sqrt(2.0 * p->h / p->a1);
        nuevo.v2 = p->a1 * nuevo.t_meco;
        nuevo.v1 = nuevo.v2;
        nuevo.y1 = p->h;
        nuevo.dt2 = 0.0;
        velocidad_cuadrada = nuevo.v2 * nuevo.v2;
    }
    nuevo.h_max = p->h + velocidad_cuadrada / (2.0 * p->g);
    nuevo.t_apogeo = nuevo.t_meco + nuevo.v2 / p->g;
    nuevo.factor_g = p->a1 / p->g;
    if (!finito(nuevo.v1) || !finito(nuevo.v2) || !finito(nuevo.dt2)
        || !finito(nuevo.t_meco) || !finito(nuevo.h_max) || !finito(nuevo.t_apogeo)
        || !finito(nuevo.factor_g) || nuevo.v2 <= 0.0 || nuevo.t_meco <= 0.0
        || nuevo.h_max <= p->h || nuevo.t_apogeo <= nuevo.t_meco) {
        return error_calculo(error, capacidad, "Un resultado fisico no es representable.");
    }
    *r = nuevo;
    if (error != NULL && capacidad > 0U) {
        error[0] = '\0';
    }
    return 1;
}

int estado_en_instante(const Parametros *p, const Resultados *r, double t, Estado *e)
{
    Estado nuevo;
    double local;

    if (p == NULL || r == NULL || e == NULL || !finito(t)) {
        return 0;
    }
    if (t < 0.0) {
        t = 0.0;
    } else if (t > r->t_apogeo) {
        t = r->t_apogeo;
    }
    nuevo.t = t;
    /* Fronteras posteriores: se apaga la llama exactamente en MECO.
       y y v son continuas; a cambia de acuerdo con la fase siguiente. */
    if (t >= r->t_apogeo) {
        nuevo.y = r->h_max;
        nuevo.v = 0.0;
        nuevo.a = -p->g;
        nuevo.fase = FASE_APOGEO;
    } else if (t >= r->t_meco) {
        local = t - r->t_meco;
        nuevo.y = p->h + r->v2 * local - 0.5 * p->g * local * local;
        nuevo.v = r->v2 - p->g * local;
        nuevo.a = -p->g;
        nuevo.fase = FASE_BALISTICA;
    } else if (r->desacople_previo && t >= p->t1) {
        local = t - p->t1;
        nuevo.y = r->y1 + r->v1 * local + 0.5 * p->a2 * local * local;
        nuevo.v = r->v1 + p->a2 * local;
        nuevo.a = p->a2;
        nuevo.fase = FASE_NUCLEO;
    } else {
        nuevo.y = 0.5 * p->a1 * t * t;
        nuevo.v = p->a1 * t;
        nuevo.a = p->a1;
        nuevo.fase = FASE_SRB;
    }
    *e = nuevo;
    return 1;
}

int preparar_simulacion(Simulacion *s, const Parametros *p, int balistico,
                       char *error, size_t capacidad)
{
    Simulacion nueva = {0};

    if (s == NULL) {
        return error_calculo(error, capacidad, "Falta el destino de la simulacion.");
    }
    if (!calcular_mision(p, &nueva.r, error, capacidad)) {
        return 0;
    }
    nueva.p = *p;
    nueva.balistico = balistico != 0;
    nueva.camara_lenta = 1;
    nueva.velocidad = 10.0;
    nueva.t_fin = nueva.balistico ? nueva.r.t_apogeo : nueva.r.t_meco;
    nueva.eventos[nueva.cantidad_eventos++] = (Evento){0.0, EVENTO_DESPEGUE};
    if (nueva.r.desacople_previo && nueva.r.dt2 > 1e-9) {
        nueva.eventos[nueva.cantidad_eventos++] = (Evento){p->t1, EVENTO_DESACOPLE};
    }
    nueva.eventos[nueva.cantidad_eventos++] = (Evento){nueva.r.t_meco, EVENTO_MECO};
    if (nueva.balistico) {
        nueva.eventos[nueva.cantidad_eventos++] = (Evento){nueva.r.t_apogeo, EVENTO_APOGEO};
    }
    (void)estado_en_instante(&nueva.p, &nueva.r, 0.0, &nueva.estado);
    *s = nueva;
    return 1;
}

void lanzar_simulacion(Simulacion *s)
{
    if (s == NULL) {
        return;
    }
    if (s->t >= s->t_fin) {
        s->t = 0.0;
        (void)estado_en_instante(&s->p, &s->r, 0.0, &s->estado);
    }
    s->iniciado = 1;
    s->corriendo = 1;
}

void alternar_pausa(Simulacion *s)
{
    if (s != NULL && s->iniciado && s->t < s->t_fin) {
        s->corriendo = !s->corriendo;
    }
}

int fijar_tiempo(Simulacion *s, double t)
{
    if (s == NULL || !finito(t)) {
        return 0;
    }
    if (t < 0.0) {
        t = 0.0;
    } else if (t >= s->t_fin - 0.01) {
        /* El deslizador visual redondea a centesimas; el final sigue siendo exacto. */
        t = s->t_fin;
    }
    s->t = t;
    s->iniciado = 1;
    s->corriendo = 0;
    return estado_en_instante(&s->p, &s->r, t, &s->estado);
}

static int velocidad_admitida(double velocidad)
{
    return velocidad == 1.0 || velocidad == 5.0 || velocidad == 10.0
           || velocidad == 20.0 || velocidad == 50.0;
}

int avanzar_reloj(Simulacion *s, double dt_real)
{
    double velocidad;
    double objetivo;

    if (s == NULL || !finito(dt_real) || dt_real < 0.0
        || !velocidad_admitida(s->velocidad)) {
        return 0;
    }
    if (!s->corriendo || dt_real == 0.0) {
        return 1;
    }
    if (dt_real > 0.05) {
        dt_real = 0.05;
    }
    velocidad = s->velocidad;
    if (s->camara_lenta && velocidad > 2.0) {
        for (size_t i = 0U; i < s->cantidad_eventos; ++i) {
            if (s->eventos[i].t > 0.0 && fabs(s->eventos[i].t - s->t) < 3.0) {
                velocidad = 2.0;
                break;
            }
        }
    }
    objetivo = s->t + dt_real * velocidad;
    if (objetivo >= s->t_fin) {
        objetivo = s->t_fin;
        s->corriendo = 0;
    }
    s->t = objetivo;
    return estado_en_instante(&s->p, &s->r, objetivo, &s->estado);
}

size_t eventos_visibles(const Simulacion *s)
{
    size_t cantidad = 0U;
    if (s == NULL || !s->iniciado) {
        return 0U;
    }
    while (cantidad < s->cantidad_eventos && s->eventos[cantidad].t <= s->t) {
        ++cantidad;
    }
    return cantidad;
}

const char *nombre_evento(TipoEvento tipo)
{
    switch (tipo) {
        case EVENTO_DESPEGUE: return "Despegue";
        case EVENTO_DESACOPLE: return "Desacople SRB";
        case EVENTO_MECO: return "MECO: apagado de motores";
        case EVENTO_APOGEO: return "Apogeo: altitud maxima";
        default: return "Evento desconocido";
    }
}

const char *nombre_fase(Fase fase)
{
    switch (fase) {
        case FASE_SRB: return "FASE 1 - NUCLEO + SRB";
        case FASE_NUCLEO: return "FASE 2 - SOLO NUCLEO";
        case FASE_BALISTICA: return "VUELO BALISTICO - a = -g";
        case FASE_APOGEO: return "APOGEO - v = 0";
        default: return "FASE DESCONOCIDA";
    }
}

const char *nombre_capa(double altitud)
{
    if (altitud < 12000.0) { return "Troposfera"; }
    if (altitud < 50000.0) { return "Estratosfera"; }
    if (altitud < 85000.0) { return "Mesosfera"; }
    if (altitud < 100000.0) { return "Termosfera"; }
    return "Termosfera / espacio";
}

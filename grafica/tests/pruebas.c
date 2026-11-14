#include "fisica.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

typedef struct { unsigned int total, fallos; } Cuenta;

static void comprobar(Cuenta *c, int condicion, const char *texto, int linea)
{
    ++c->total;
    if (!condicion) {
        ++c->fallos;
        fprintf(stderr, "FALLO %d: %s\n", linea, texto);
    }
}
#define VER(c, x) comprobar((c), (x), #x, __LINE__)

static int cerca(double a, double b, double tolerancia)
{
    return a >= -DBL_MAX && a <= DBL_MAX && fabs(a - b) <= tolerancia;
}

static void probar_nominal(Cuenta *c)
{
    Parametros p = parametros_nominales();
    Resultados r;
    Estado e;
    char error[ERROR_CAPACIDAD];
    VER(c, calcular_mision(&p, &r, error, sizeof error));
    VER(c, cerca(r.y1, 144000.0, 1e-8));
    VER(c, cerca(r.v1, 2400.0, 1e-9));
    VER(c, cerca(r.v2, 2481.934729, 2e-6));
    VER(c, cerca(r.dt2, 6.554778, 2e-6));
    VER(c, cerca(r.t_meco, 126.554778, 2e-6));
    VER(c, cerca(r.h_max, 474285.714286, 2e-6));
    VER(c, cerca(r.t_apogeo, 379.813424, 2e-6));
    VER(c, cerca(r.factor_g, 2.040816, 2e-6));
    VER(c, r.desacople_previo);
    VER(c, estado_en_instante(&p, &r, p.t1, &e));
    VER(c, e.fase == FASE_NUCLEO && e.a == p.a2);
    VER(c, e.y == r.y1 && e.v == r.v1);
    VER(c, estado_en_instante(&p, &r, r.t_meco, &e));
    VER(c, e.fase == FASE_BALISTICA && e.a == -p.g);
    VER(c, e.y == p.h && e.v == r.v2);
    VER(c, estado_en_instante(&p, &r, r.t_apogeo, &e));
    VER(c, e.fase == FASE_APOGEO && e.v == 0.0 && e.a == -p.g);
    VER(c, e.y == r.h_max);
    VER(c, estado_en_instante(&p, &r, DBL_MAX, &e));
    VER(c, e.t == r.t_apogeo && e.v == 0.0);
    VER(c, estado_en_instante(&p, &r, -1.0, &e));
    VER(c, e.t == 0.0 && e.y == 0.0 && e.v == 0.0);
}

static void probar_continuidad(Cuenta *c, Parametros p)
{
    Resultados r;
    Estado antes, frontera, despues;
    char error[ERROR_CAPACIDAD];
    const double eps = 1e-5;
    double eventos[2];
    VER(c, calcular_mision(&p, &r, error, sizeof error));
    eventos[0] = r.desacople_previo ? p.t1 : r.t_meco;
    eventos[1] = r.t_meco;
    for (size_t i = 0U; i < 2U; ++i) {
        VER(c, estado_en_instante(&p, &r, eventos[i] - eps, &antes));
        VER(c, estado_en_instante(&p, &r, eventos[i], &frontera));
        VER(c, estado_en_instante(&p, &r, eventos[i] + eps, &despues));
        VER(c, cerca(frontera.y, antes.y + antes.v * eps + 0.5 * antes.a * eps * eps, 1e-6));
        VER(c, cerca(frontera.v, antes.v + antes.a * eps, 1e-7));
        VER(c, cerca(despues.y, frontera.y + frontera.v * eps + 0.5 * frontera.a * eps * eps, 1e-6));
        VER(c, cerca(despues.v, frontera.v + frontera.a * eps, 1e-7));
    }
}

static void probar_meco_anticipado(Cuenta *c)
{
    Parametros p = parametros_nominales();
    Simulacion s;
    char error[ERROR_CAPACIDAD];
    p.h = 100000.0;
    VER(c, preparar_simulacion(&s, &p, 1, error, sizeof error));
    VER(c, !s.r.desacople_previo && s.r.dt2 == 0.0);
    VER(c, cerca(s.r.t_meco, 100.0, 1e-10));
    VER(c, cerca(s.r.v2, 2000.0, 1e-10));
    VER(c, cerca(s.r.h_max, 304081.632653, 2e-6));
    VER(c, cerca(s.r.t_apogeo, 304.081633, 2e-6));
    VER(c, s.cantidad_eventos == 3U);
    VER(c, s.eventos[1].tipo == EVENTO_MECO);
    probar_continuidad(c, p);
    p.h = 144000.0;
    VER(c, preparar_simulacion(&s, &p, 1, error, sizeof error));
    VER(c, s.r.desacople_previo && s.r.dt2 == 0.0);
    VER(c, s.r.t_meco == p.t1 && s.r.v2 == 2400.0);
    VER(c, cerca(s.r.h_max, 437877.551020, 2e-6));
    VER(c, cerca(s.r.t_apogeo, 364.897959, 2e-6));
    VER(c, s.cantidad_eventos == 3U && s.eventos[1].tipo == EVENTO_MECO);
    VER(c, fijar_tiempo(&s, p.t1));
    VER(c, s.estado.fase == FASE_BALISTICA && s.estado.a == -p.g);
    probar_continuidad(c, p);
}

static void probar_controles(Cuenta *c)
{
    Parametros p = parametros_nominales();
    Simulacion s;
    unsigned char copia[sizeof s];
    char error[ERROR_CAPACIDAD];
    double antes;
    VER(c, preparar_simulacion(&s, &p, 1, error, sizeof error));
    VER(c, !s.iniciado && !s.corriendo && s.t == 0.0);
    VER(c, s.velocidad == 10.0 && s.camara_lenta && s.cantidad_eventos == 4U);
    VER(c, eventos_visibles(&s) == 0U);
    lanzar_simulacion(&s);
    VER(c, s.iniciado && s.corriendo && eventos_visibles(&s) == 1U);
    VER(c, avanzar_reloj(&s, 0.05));
    VER(c, cerca(s.t, 0.5, 1e-12));
    alternar_pausa(&s);
    VER(c, !s.corriendo);
    memcpy(copia, &s, sizeof s);
    VER(c, avanzar_reloj(&s, 0.05));
    VER(c, memcmp(copia, &s, sizeof s) == 0);
    alternar_pausa(&s);
    VER(c, s.corriendo);
    VER(c, avanzar_reloj(&s, 10.0));
    VER(c, cerca(s.t, 1.0, 1e-12)); /* frame acotado a 0.05 real */
    VER(c, fijar_tiempo(&s, p.t1));
    VER(c, !s.corriendo && s.t == p.t1 && eventos_visibles(&s) == 2U);
    VER(c, fijar_tiempo(&s, 10.0));
    VER(c, s.t == 10.0 && eventos_visibles(&s) == 1U);
    VER(c, fijar_tiempo(&s, s.t_fin - 0.005));
    VER(c, s.t == s.t_fin && s.estado.v == 0.0 && !s.corriendo);
    VER(c, eventos_visibles(&s) == 4U);
    alternar_pausa(&s);
    VER(c, !s.corriendo);
    lanzar_simulacion(&s);
    VER(c, s.corriendo && s.t == 0.0);
    VER(c, fijar_tiempo(&s, p.t1 - 2.0));
    s.corriendo = 1;
    s.velocidad = 50.0;
    antes = s.t;
    VER(c, avanzar_reloj(&s, 0.05));
    VER(c, cerca(s.t - antes, 0.1, 1e-12));
    s.camara_lenta = 0;
    antes = s.t;
    VER(c, avanzar_reloj(&s, 0.05));
    VER(c, cerca(s.t - antes, 2.5, 1e-12));
    s.velocidad = 1.0;
    antes = s.t;
    VER(c, avanzar_reloj(&s, 0.05));
    VER(c, cerca(s.t - antes, 0.05, 1e-12));
    VER(c, preparar_simulacion(&s, &p, 0, error, sizeof error));
    VER(c, s.t_fin == s.r.t_meco && s.cantidad_eventos == 3U);
    VER(c, fijar_tiempo(&s, DBL_MAX));
    VER(c, s.t == s.r.t_meco && s.estado.y == p.h && s.estado.v == s.r.v2);
    VER(c, s.estado.fase == FASE_BALISTICA && eventos_visibles(&s) == 3U);
}

static void probar_vuelo(Cuenta *c, Parametros p, int balistico)
{
    Simulacion s;
    char error[ERROR_CAPACIDAD];
    size_t pasos = 0U;
    VER(c, preparar_simulacion(&s, &p, balistico, error, sizeof error));
    s.velocidad = 50.0;
    s.camara_lenta = 0;
    lanzar_simulacion(&s);
    while (s.corriendo && pasos < 100000U) {
        Estado anterior = s.estado;
        VER(c, avanzar_reloj(&s, 0.033));
        VER(c, s.t > anterior.t && s.t <= s.t_fin);
        VER(c, s.estado.y >= anterior.y - 1e-7 && s.estado.v >= 0.0);
        VER(c, s.estado.y >= 0.0 && s.estado.y <= s.r.h_max + 1e-7);
        if (s.estado.fase == FASE_SRB) {
            VER(c, cerca(s.estado.v * s.estado.v, 2.0 * p.a1 * s.estado.y, 1e-5));
        } else if (s.estado.fase == FASE_NUCLEO) {
            VER(c, cerca(s.estado.v * s.estado.v - s.r.v1 * s.r.v1,
                         2.0 * p.a2 * (s.estado.y - s.r.y1), 1e-5));
        } else {
            VER(c, cerca(s.estado.v * s.estado.v - s.r.v2 * s.r.v2,
                         -2.0 * p.g * (s.estado.y - p.h), 1e-5));
        }
        ++pasos;
    }
    VER(c, pasos < 100000U && !s.corriendo && s.t == s.t_fin);
    VER(c, eventos_visibles(&s) == s.cantidad_eventos);
    if (balistico) {
        VER(c, s.estado.v == 0.0 && s.estado.y == s.r.h_max);
    } else {
        VER(c, s.estado.y == p.h && s.estado.v == s.r.v2);
    }
}

static void probar_invalidos(Cuenta *c)
{
    const Parametros nominal = parametros_nominales();
    Parametros invalidos[] = {
        {0.0,120.0,12.5,160000.0,9.8}, {61.0,120.0,12.5,160000.0,9.8},
        {20.0,0.0,12.5,160000.0,9.8}, {20.0,601.0,12.5,160000.0,9.8},
        {20.0,120.0,-1.0,160000.0,9.8}, {20.0,120.0,12.5,999.0,9.8},
        {20.0,120.0,12.5,2000001.0,9.8}, {20.0,120.0,12.5,160000.0,0.0},
        {NAN,120.0,12.5,160000.0,9.8}, {20.0,120.0,INFINITY,160000.0,9.8},
        {20.0,120.0,12.5,160000.0,DBL_MAX}
    };
    const double dt_invalidos[] = {-1.0, NAN, INFINITY};
    Simulacion s;
    Estado e = {0};
    unsigned char copia[sizeof s];
    char error[ERROR_CAPACIDAD];
    VER(c, preparar_simulacion(&s, &nominal, 1, error, sizeof error));
    memcpy(copia, &s, sizeof s);
    for (size_t i = 0U; i < sizeof invalidos / sizeof invalidos[0]; ++i) {
        VER(c, !preparar_simulacion(&s, &invalidos[i], 1, error, sizeof error));
        VER(c, error[0] != '\0' && memcmp(copia, &s, sizeof s) == 0);
    }
    for (size_t i = 0U; i < sizeof dt_invalidos / sizeof dt_invalidos[0]; ++i) {
        VER(c, !avanzar_reloj(&s, dt_invalidos[i]));
        VER(c, memcmp(copia, &s, sizeof s) == 0);
    }
    VER(c, !fijar_tiempo(&s, NAN) && !fijar_tiempo(&s, INFINITY));
    VER(c, memcmp(copia, &s, sizeof s) == 0);
    VER(c, !estado_en_instante(&s.p, &s.r, NAN, &e));
    s.velocidad = 3.0;
    VER(c, !avanzar_reloj(&s, 0.01));
}

static void probar_entradas(Cuenta *c)
{
    const char *invalidos[] = {"", "  ", "abc", "12x", "1 2", "nan", "inf", "1e9999",
                              "1e-9999", "1,2,3", "1.2,3", "++1", "0x10", "1e", "0", "-1", "61"};
    char largo[180];
    double valor = 0.0;
    VER(c, convertir_numero("12,5", A_MIN, A_MAX, &valor) && valor == 12.5);
    VER(c, convertir_numero("12.5", A_MIN, A_MAX, &valor) && valor == 12.5);
    VER(c, convertir_numero(" \t+1,2e1\r\n", A_MIN, A_MAX, &valor) && valor == 12.0);
    VER(c, convertir_numero("0.1", A_MIN, A_MAX, &valor) && valor == A_MIN);
    VER(c, convertir_numero("60", A_MIN, A_MAX, &valor) && valor == A_MAX);
    for (size_t i = 0U; i < sizeof invalidos / sizeof invalidos[0]; ++i) {
        valor = -777.0;
        VER(c, !convertir_numero(invalidos[i], A_MIN, A_MAX, &valor));
        VER(c, valor == -777.0);
    }
    memset(largo, '1', sizeof largo - 1U);
    largo[sizeof largo - 1U] = '\0';
    VER(c, !convertir_numero(largo, A_MIN, A_MAX, &valor));
    VER(c, !convertir_numero(NULL, A_MIN, A_MAX, &valor));
    VER(c, !convertir_numero("1", NAN, A_MAX, &valor));
}

int main(void)
{
    Cuenta c = {0,0};
    Parametros p = parametros_nominales();
    probar_nominal(&c);
    probar_continuidad(&c, p);
    probar_meco_anticipado(&c);
    probar_controles(&c);
    probar_invalidos(&c);
    probar_entradas(&c);
    probar_vuelo(&c, p, 1);
    probar_vuelo(&c, p, 0);
    p.h = 100000.0;
    probar_vuelo(&c, p, 1);
    p = (Parametros){A_MIN,T_MIN,A_MIN,H_MIN,G_MAX};
    probar_vuelo(&c, p, 1);
    p = (Parametros){A_MAX,T_MAX,A_MAX,H_MAX,G_MIN};
    probar_vuelo(&c, p, 1);
    printf("Pruebas C: %u comprobaciones, %u fallos.\n", c.total, c.fallos);
    return c.fallos == 0U ? 0 : 1;
}

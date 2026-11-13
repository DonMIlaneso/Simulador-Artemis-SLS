/*
 * artemis.h
 * Módulo de Cabecera: constantes, tipos de datos y prototipos públicos
 * del simulador. Situación - Problema 1: ascenso y propulsión con el SLS.
 */
#ifndef ARTEMIS_H
#define ARTEMIS_H

/* ---------- Datos del enunciado ---------- */
#define A1_NOMINAL   20.0       /* m/s²  aceleración fase 1 (núcleo + SRB) */
#define T1_NOMINAL   120.0      /* s     instante del desacople de los SRB */
#define A2_NOMINAL   12.5       /* m/s²  aceleración fase 2 (solo núcleo)  */
#define H_NOMINAL    160000.0   /* m     altitud de apagado (MECO)         */
#define G_NOMINAL    9.80       /* m/s²  gravedad terrestre                */

/* ---------- Rangos válidos de entrada ---------- */
#define A_MIN    0.1
#define A_MAX    60.0
#define T1_MIN   1.0
#define T1_MAX   600.0
#define H_MIN    1000.0
#define H_MAX    2000000.0
#define G_MIN    1.0
#define G_MAX    25.0
#define DT_MIN   0.1            /* paso mínimo de la telemetría (s) */
#define DT_MAX   600.0
#define LIMITE_G_TRIPULACION 4.0

/* ---------- Tamaños de arreglos y de la tabla ---------- */
#define MAX_MUESTRAS      1000  /* filas de telemetría por simulación */
#define MAX_RONDAS        5     /* rondas por partida del desafío     */
#define MAX_PARTIDAS      20    /* partidas guardadas en la sesión    */
#define CANT_PREGUNTAS    5     /* v1, y1, v2, hmax y a1/g            */
#define ANCHO_BARRA       20    /* largo del perfil de altitud ASCII  */
#define FILAS_POR_PAGINA  20    /* filas de telemetría antes de pausar */

/* ---------- Reglas de puntaje del desafío ---------- */
#define TOLERANCIA_EXACTA     1.0   /* % de error */
#define TOLERANCIA_BUENA      5.0
#define TOLERANCIA_ACEPTABLE  10.0
#define PUNTOS_EXACTA         100
#define PUNTOS_BUENA          60
#define PUNTOS_ACEPTABLE      30
#define PENALIZACION_AYUDA    20    /* % que se descuenta si se piden las fórmulas */
#define RESPUESTA_MAX         1e9

/* ---------- Fases y eventos del vuelo ---------- */
#define FASE_SRB          1     /* núcleo + cohetes sólidos */
#define FASE_NUCLEO       2     /* solo el núcleo           */
#define FASE_BALISTICA    3     /* motores apagados, a = -g */

#define EVENTO_NINGUNO    0
#define EVENTO_DESPEGUE   1
#define EVENTO_DESACOPLE  2
#define EVENTO_MECO       3
#define EVENTO_APOGEO     4

/* Tolerancia para comparar números reales (dos double casi nunca son exactamente iguales) */
#define EPSILON 1e-9


/* ---------- Estructuras ---------- */

/* Datos de entrada de la misión */
typedef struct {
    double a1;      /* aceleración fase 1 (m/s²) */
    double t1;      /* instante del desacople (s) */
    double a2;      /* aceleración fase 2 (m/s²) */
    double h;       /* altitud de apagado (m) */
    double g;       /* gravedad (m/s²) */
} Parametros_Mision;

/* Respuestas a), b), c) y d) e instantes clave del vuelo */
typedef struct {
    int    desacople_previo;  /* 1: los SRB se sueltan antes del MECO (caso normal) */
    double v1, y1;            /* a) estado al desacople (m/s, m)            */
    double dist_fase2;        /*    distancia recorrida en la fase 2 (m)    */
    double dt_fase2;          /*    duración de la fase 2 (s)               */
    double t_meco;            /*    instante del apagado de motores (s)     */
    double v2;                /* b) rapidez máxima (m/s)                    */
    double subida_libre;      /* c) altura ganada sin motores (m)           */
    double h_max;             /* c) altitud máxima (m)                      */
    double t_apogeo;          /* c) instante del apogeo (s)                 */
    double factor_g;          /* d) a1 / g                                  */
    double carga_g;           /* d) (a1 + g) / g: lo que siente la tripulación */
} Resultados_Mision;

/* Una fila de telemetría */
typedef struct {
    double t, y, v, a;   /* s, m, m/s, m/s² */
    int fase;            /* FASE_...   */
    int evento;          /* EVENTO_... */
} Muestra_Telemetria;

/* Todas las filas de una simulación */
typedef struct {
    Muestra_Telemetria muestras[MAX_MUESTRAS];
    int cantidad;
} Telemetria;

/* Una ronda del Desafío del Director de Vuelo */
typedef struct {
    Parametros_Mision parametros;
    double respuestas[CANT_PREGUNTAS];
    double correctas[CANT_PREGUNTAS];
    double errores[CANT_PREGUNTAS];     /* error relativo en % */
    int uso_ayuda;                      /* 1 si pidió las fórmulas */
    int puntos;
} Ronda_Desafio;

/* Resumen de una partida terminada */
typedef struct {
    int numero;
    int rondas;
    int puntaje;
    int puntaje_maximo;
    double error_promedio;   /* % */
} Resumen_Partida;

/* Desempeño acumulado mientras el programa está abierto */
typedef struct {
    Resumen_Partida partidas[MAX_PARTIDAS];
    int cantidad_partidas;
    int simulaciones_realizadas;
    double apogeo_record;    /* m   */
    double rapidez_record;   /* m/s */
} Historial_Sesion;


/* ---------- Funciones públicas ---------- */

/* Parámetros */
void cargar_valores_nominales(Parametros_Mision *parametros);
void mostrar_parametros(const Parametros_Mision *parametros);
void configurar_parametros(Parametros_Mision *parametros);

/* Cálculos físicos */
void calcular_mision(const Parametros_Mision *parametros, Resultados_Mision *resultados);
void mostrar_analisis(const Parametros_Mision *parametros, const Resultados_Mision *resultados);

/* Simulación con telemetría */
void ejecutar_simulacion(const Parametros_Mision *parametros, Historial_Sesion *historial);

/* Desafío y reportes */
void inicializar_historial(Historial_Sesion *historial);
void jugar_desafio(Historial_Sesion *historial);
void mostrar_reporte_sesion(const Historial_Sesion *historial);
void mostrar_ayuda(void);

#endif
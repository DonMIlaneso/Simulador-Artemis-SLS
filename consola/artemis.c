/*
 * artemis.c
 * Módulo de Lógica: cálculos físicos, simulación con telemetría,
 * Desafío del Director de Vuelo y reportes de desempeño.
 *
 * Modelo físico (MRUV por tramos, se desprecia el aire):
 *   Fase 1           0 <= t <= t1       a = a1   (núcleo + SRB)
 *   Fase 2           t1 < t <= t_meco   a = a2   (solo núcleo, hasta la altitud h)
 *   Vuelo balístico  t > t_meco         a = -g   (motores apagados)
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "artemis.h"
#include "validaciones.h"

/* Funciones internas del módulo (no se publican en artemis.h) */
void verificar_coherencia(const Parametros_Mision *parametros);
void estado_en_instante(const Parametros_Mision *parametros, const Resultados_Mision *resultados,
                        double t, Muestra_Telemetria *muestra);
void agregar_muestra(const Parametros_Mision *parametros, const Resultados_Mision *resultados,
                     double t, int evento, Telemetria *telemetria);
void generar_telemetria(const Parametros_Mision *parametros, const Resultados_Mision *resultados,
                        double dt, int con_balistica, Telemetria *telemetria);
void mostrar_telemetria(const Telemetria *telemetria, double altitud_referencia);
void reporte_simulacion(const Telemetria *telemetria, const Resultados_Mision *resultados,
                        Historial_Sesion *historial);
void imprimir_encabezado_telemetria(void);
void dibujar_barra(double valor, double maximo);
const char *codigo_fase(int fase);
const char *nombre_evento(int evento);
double numero_aleatorio(double minimo, double maximo, double paso);
void generar_mision_aleatoria(Parametros_Mision *parametros);
double error_relativo(double obtenido, double esperado);
int puntos_por_error(double error);
const char *calificacion(double porcentaje);
void mostrar_formulas(void);
void jugar_ronda(Ronda_Desafio *ronda, int numero, int total);
void reporte_partida(const Ronda_Desafio rondas[], int cantidad, Resumen_Partida *resumen);


/* ====================================================================== */
/*                               PARÁMETROS                               */
/* ====================================================================== */

void cargar_valores_nominales(Parametros_Mision *parametros)
{
    parametros->a1 = A1_NOMINAL;
    parametros->t1 = T1_NOMINAL;
    parametros->a2 = A2_NOMINAL;
    parametros->h  = H_NOMINAL;
    parametros->g  = G_NOMINAL;
}

void mostrar_parametros(const Parametros_Mision *parametros)
{
    printf("  Aceleración fase 1 (a1) ......... %12.2f m/s²\n", parametros->a1);
    printf("  Desacople de los SRB (t1) ....... %12.2f s\n",    parametros->t1);
    printf("  Aceleración fase 2 (a2) ......... %12.2f m/s²\n", parametros->a2);
    printf("  Altitud de apagado (h) .......... %12.2f m\n",    parametros->h);
    printf("  Gravedad (g) .................... %12.2f m/s²\n", parametros->g);
}

/* Avisa si la configuración es físicamente dudosa. No bloquea: solo informa. */
void verificar_coherencia(const Parametros_Mision *parametros)
{
    double y_desacople = 0.5 * parametros->a1 * parametros->t1 * parametros->t1;
    double factor_g = parametros->a1 / parametros->g;
    int cantidad_avisos = 0;

    if (y_desacople > parametros->h) {
        printf("  [AVISO] En t1 la nave estaría a %.0f m, por encima de h:\n", y_desacople);
        printf("          el MECO ocurre antes del desacople de los SRB.\n");
        cantidad_avisos++;
    }
    if (factor_g > LIMITE_G_TRIPULACION) {
        printf("  [AVISO] a1 equivale a %.2f g y supera el límite de %.1f g para la tripulación.\n",
               factor_g, LIMITE_G_TRIPULACION);
        cantidad_avisos++;
    }
    if (parametros->a2 > parametros->a1) {
        mostrar_aviso("a2 es mayor que a1: la fase 2 acelera más que la fase 1 (inusual en el SLS).");
        cantidad_avisos++;
    }
    if (cantidad_avisos == 0) {
        printf("  [OK] Configuración físicamente coherente.\n");
    }
}

/* Opción 1 del menú: submenú para cambiar cada parámetro con su rango */
void configurar_parametros(Parametros_Mision *parametros)
{
    int opcion;

    do {
        limpiar_pantalla();
        imprimir_titulo("CONFIGURACIÓN DE LA MISIÓN");
        printf("  1. Aceleración fase 1 (a1) ...... %12.2f m/s²\n", parametros->a1);
        printf("  2. Desacople de los SRB (t1) .... %12.2f s\n",    parametros->t1);
        printf("  3. Aceleración fase 2 (a2) ...... %12.2f m/s²\n", parametros->a2);
        printf("  4. Altitud de apagado (h) ....... %12.2f m\n",    parametros->h);
        printf("  5. Gravedad (g) ................. %12.2f m/s²\n", parametros->g);
        printf("  6. Restaurar los valores del enunciado\n");
        printf("  0. Volver al menú principal\n\n");
        verificar_coherencia(parametros);
        printf("\n");

        opcion = leer_entero("  Seleccione una opción: ", 0, 6);
        switch (opcion) {
            case 1:
                printf("  Rango válido: %.1f a %.1f m/s²\n", A_MIN, A_MAX);
                parametros->a1 = leer_real("  Nuevo valor de a1: ", A_MIN, A_MAX);
                break;
            case 2:
                printf("  Rango válido: %.0f a %.0f s\n", T1_MIN, T1_MAX);
                parametros->t1 = leer_real("  Nuevo valor de t1: ", T1_MIN, T1_MAX);
                break;
            case 3:
                printf("  Rango válido: %.1f a %.1f m/s²\n", A_MIN, A_MAX);
                parametros->a2 = leer_real("  Nuevo valor de a2: ", A_MIN, A_MAX);
                break;
            case 4:
                printf("  Rango válido: %.0f a %.0f m\n", H_MIN, H_MAX);
                parametros->h = leer_real("  Nuevo valor de h: ", H_MIN, H_MAX);
                break;
            case 5:
                printf("  Rango válido: %.0f a %.0f m/s²\n", G_MIN, G_MAX);
                parametros->g = leer_real("  Nuevo valor de g: ", G_MIN, G_MAX);
                break;
            case 6:
                cargar_valores_nominales(parametros);
                printf("  [OK] Se restauraron los valores del enunciado.\n");
                pausar();
                break;
        }
    } while (opcion != 0);
}


/* ====================================================================== */
/*                            CÁLCULOS FÍSICOS                            */
/* ====================================================================== */

/* Resuelve las preguntas a), b), c) y d) con las ecuaciones del MRUV */
void calcular_mision(const Parametros_Mision *parametros, Resultados_Mision *resultados)
{
    /* Altitud que tendría la nave en t1 si sigue en la fase 1: y = ½·a1·t1² */
    double y_desacople = 0.5 * parametros->a1 * parametros->t1 * parametros->t1;

    if (y_desacople <= parametros->h) {
        /* Caso normal: desacople en t1 y fase 2 hasta llegar a h */
        resultados->desacople_previo = 1;
        resultados->v1 = parametros->a1 * parametros->t1;                     /* v = v0 + a·t */
        resultados->y1 = y_desacople;
        resultados->dist_fase2 = parametros->h - resultados->y1;
        resultados->v2 = sqrt(resultados->v1 * resultados->v1               /* v² = v1² + 2·a2·Δy */
                              + 2.0 * parametros->a2 * resultados->dist_fase2);
        resultados->dt_fase2 = (resultados->v2 - resultados->v1) / parametros->a2;
        resultados->t_meco = parametros->t1 + resultados->dt_fase2;
    } else {
        /* Caso especial: la nave llega a h durante la fase 1, con los SRB acoplados */
        resultados->desacople_previo = 0;
        resultados->t_meco = sqrt(2.0 * parametros->h / parametros->a1);    /* h = ½·a1·t² */
        resultados->v2 = parametros->a1 * resultados->t_meco;
        resultados->v1 = resultados->v2;
        resultados->y1 = parametros->h;
        resultados->dist_fase2 = 0.0;
        resultados->dt_fase2 = 0.0;
    }

    /* c) Sin motores: sube frenando hasta v = 0  ->  0 = v2² - 2·g·Δy */
    resultados->subida_libre = resultados->v2 * resultados->v2 / (2.0 * parametros->g);
    resultados->h_max = parametros->h + resultados->subida_libre;
    resultados->t_apogeo = resultados->t_meco + resultados->v2 / parametros->g;

    /* d) Aceleración de la fase 1 en múltiplos de g */
    resultados->factor_g = parametros->a1 / parametros->g;
    resultados->carga_g = (parametros->a1 + parametros->g) / parametros->g;
}

/* Calcula posición, velocidad, aceleración y fase en un instante t */
void estado_en_instante(const Parametros_Mision *parametros, const Resultados_Mision *resultados,
                        double t, Muestra_Telemetria *muestra)
{
    double fin_fase1;
    double tau;     /* tiempo transcurrido desde el inicio de la fase actual */

    if (resultados->desacople_previo) {
        fin_fase1 = parametros->t1;
    } else {
        fin_fase1 = resultados->t_meco;
    }

    muestra->t = t;
    if (t <= fin_fase1 + EPSILON) {
        muestra->y = 0.5 * parametros->a1 * t * t;
        muestra->v = parametros->a1 * t;
        muestra->a = parametros->a1;
        muestra->fase = FASE_SRB;
    } else if (resultados->desacople_previo && t <= resultados->t_meco + EPSILON) {
        tau = t - parametros->t1;
        muestra->y = resultados->y1 + resultados->v1 * tau + 0.5 * parametros->a2 * tau * tau;
        muestra->v = resultados->v1 + parametros->a2 * tau;
        muestra->a = parametros->a2;
        muestra->fase = FASE_NUCLEO;
    } else {
        tau = t - resultados->t_meco;
        muestra->y = parametros->h + resultados->v2 * tau - 0.5 * parametros->g * tau * tau;
        muestra->v = resultados->v2 - parametros->g * tau;
        muestra->a = -parametros->g;
        muestra->fase = FASE_BALISTICA;
    }
    if (fabs(muestra->v) < 1e-6) {
        muestra->v = 0.0;    /* evita mostrar "-0.00" en el apogeo */
    }
}

/* Opción 2 del menú: desarrollo paso a paso de a), b), c) y d) */
void mostrar_analisis(const Parametros_Mision *parametros, const Resultados_Mision *resultados)
{
    limpiar_pantalla();
    imprimir_titulo("ANÁLISIS DEL ASCENSO DEL SLS (MRUV)");
    mostrar_parametros(parametros);
    printf("\n  Condiciones iniciales: v0 = 0 m/s, y0 = 0 m. Se desprecia el aire.\n");

    imprimir_separador();
    printf("  a) Estado al desacople de los SRB (t1 = %.2f s)\n\n", parametros->t1);
    if (resultados->desacople_previo) {
        printf("     v1 = v0 + a1·t1 = 0 + %.2f · %.2f = %.2f m/s\n",
               parametros->a1, parametros->t1, resultados->v1);
        printf("     y1 = y0 + v0·t1 + ½·a1·t1²\n");
        printf("        = ½ · %.2f · %.2f² = %.2f m (%.2f km)\n",
               parametros->a1, parametros->t1, resultados->y1, resultados->y1 / 1000.0);
    } else {
        printf("  [AVISO] La nave llega a h en T+%.2f s, antes del desacople (t1 = %.2f s).\n",
               resultados->t_meco, parametros->t1);
        printf("     Los motores se apagan con los SRB todavía acoplados.\n");
        printf("     Estado al apagado: v = %.2f m/s, y = %.2f m\n", resultados->v1, resultados->y1);
    }

    imprimir_separador();
    printf("  b) Rapidez máxima al apagar los motores (h = %.2f m)\n\n", parametros->h);
    if (resultados->desacople_previo) {
        printf("     Δy2 = h − y1 = %.2f − %.2f = %.2f m\n",
               parametros->h, resultados->y1, resultados->dist_fase2);
        printf("     v2² = v1² + 2·a2·Δy2\n");
        printf("     v2  = √(%.2f² + 2 · %.2f · %.2f) = %.2f m/s\n",
               resultados->v1, parametros->a2, resultados->dist_fase2, resultados->v2);
        printf("     Duración de la fase 2: Δt2 = (v2 − v1) / a2 = %.2f s\n", resultados->dt_fase2);
    } else {
        printf("     v2² = 2·a1·h  →  v2 = √(2 · %.2f · %.2f) = %.2f m/s\n",
               parametros->a1, parametros->h, resultados->v2);
    }
    printf("     MECO (apagado de motores) en T+%.2f s\n", resultados->t_meco);

    imprimir_separador();
    printf("  c) Falla de motores en h: caída libre (a = −g)\n\n");
    printf("     En el punto más alto v = 0  →  0 = v2² − 2·g·Δy\n");
    printf("     Δy   = v2² / (2·g) = %.2f² / (2 · %.2f) = %.2f m\n",
           resultados->v2, parametros->g, resultados->subida_libre);
    printf("     hmáx = h + Δy = %.2f + %.2f = %.2f m (%.2f km)\n",
           parametros->h, resultados->subida_libre, resultados->h_max, resultados->h_max / 1000.0);
    printf("     Tiempo de subida sin motores: v2 / g = %.2f s\n", resultados->t_apogeo - resultados->t_meco);

    imprimir_separador();
    printf("  d) Aceleración máxima de la fase 1 como múltiplo de g\n\n");
    printf("     a1 / g = %.2f / %.2f = %.2f g\n", parametros->a1, parametros->g, resultados->factor_g);
    printf("     Carga que siente la tripulación: (a1 + g) / g = %.2f g\n", resultados->carga_g);
    if (resultados->factor_g > LIMITE_G_TRIPULACION) {
        mostrar_aviso("La aceleración supera el límite recomendado para la tripulación.");
    }

    imprimir_separador();
    printf("  Cronología del vuelo:\n");
    printf("     T+%9.2f s  Despegue\n", 0.0);
    if (resultados->desacople_previo && resultados->dt_fase2 > EPSILON) {
        printf("     T+%9.2f s  Desacople de los SRB\n", parametros->t1);
    }
    printf("     T+%9.2f s  MECO (apagado de motores)\n", resultados->t_meco);
    printf("     T+%9.2f s  Apogeo (si los motores fallan en h)\n", resultados->t_apogeo);
}


/* ====================================================================== */
/*                       SIMULACIÓN CON TELEMETRÍA                        */
/* ====================================================================== */

const char *codigo_fase(int fase)
{
    if (fase == FASE_SRB) {
        return "SRB";
    }
    if (fase == FASE_NUCLEO) {
        return "NUC";
    }
    return "BAL";
}

const char *nombre_evento(int evento)
{
    switch (evento) {
        case EVENTO_DESPEGUE:  return "DESPEGUE desde el Centro Espacial Kennedy";
        case EVENTO_DESACOPLE: return "DESACOPLE de los cohetes SRB";
        case EVENTO_MECO:      return "MECO: apagado de los motores";
        case EVENTO_APOGEO:    return "APOGEO: altitud máxima";
        default:               return "";
    }
}

/* Dibuja |#######.......| proporcional a valor/maximo */
void dibujar_barra(double valor, double maximo)
{
    int i;
    int llenos = (int) (valor / maximo * ANCHO_BARRA + 0.5);

    printf("|");
    for (i = 0; i < ANCHO_BARRA; i++) {
        if (i < llenos) {
            printf("#");
        } else {
            printf(".");
        }
    }
    printf("|");
}

/* Calcula el estado en el instante t y lo guarda en la próxima posición libre del arreglo */
void agregar_muestra(const Parametros_Mision *parametros, const Resultados_Mision *resultados,
                     double t, int evento, Telemetria *telemetria)
{
    Muestra_Telemetria *nueva;

    if (telemetria->cantidad < MAX_MUESTRAS) {
        nueva = &telemetria->muestras[telemetria->cantidad];
        estado_en_instante(parametros, resultados, t, nueva);
        nueva->evento = evento;
        telemetria->cantidad++;
    }
}

/*
 * Llena el arreglo con una muestra cada dt segundos e intercala los eventos
 * (desacople, MECO y apogeo) en su instante exacto, aunque no coincidan con el paso.
 */
void generar_telemetria(const Parametros_Mision *parametros, const Resultados_Mision *resultados,
                        double dt, int con_balistica, Telemetria *telemetria)
{
    double tiempos_evento[3];
    int tipos_evento[3];
    int cant_eventos = 0;
    int k = 0;              /* próximo evento a insertar */
    int paso = 1;
    double t, t_fin;

    /* 1) Eventos del vuelo, en orden cronológico */
    if (resultados->desacople_previo && resultados->dt_fase2 > EPSILON) {
        tiempos_evento[cant_eventos] = parametros->t1;
        tipos_evento[cant_eventos] = EVENTO_DESACOPLE;
        cant_eventos++;
    }
    tiempos_evento[cant_eventos] = resultados->t_meco;
    tipos_evento[cant_eventos] = EVENTO_MECO;
    cant_eventos++;
    t_fin = resultados->t_meco;
    if (con_balistica) {
        tiempos_evento[cant_eventos] = resultados->t_apogeo;
        tipos_evento[cant_eventos] = EVENTO_APOGEO;
        cant_eventos++;
        t_fin = resultados->t_apogeo;
    }

    /* 2) Muestras regulares cada dt, con los eventos intercalados */
    telemetria->cantidad = 0;
    agregar_muestra(parametros, resultados, 0.0, EVENTO_DESPEGUE, telemetria);
    t = dt;
    while (t < t_fin - EPSILON) {
        while (k < cant_eventos && tiempos_evento[k] < t - EPSILON) {
            agregar_muestra(parametros, resultados, tiempos_evento[k], tipos_evento[k], telemetria);
            k++;
        }
        if (k < cant_eventos && fabs(tiempos_evento[k] - t) <= EPSILON) {
            agregar_muestra(parametros, resultados, t, tipos_evento[k], telemetria);  /* el evento cae justo en t */
            k++;
        } else {
            agregar_muestra(parametros, resultados, t, EVENTO_NINGUNO, telemetria);
        }
        paso++;
        t = paso * dt;      /* se multiplica en vez de sumar para no acumular error de redondeo */
    }

    /* 3) Eventos que quedan: el último coincide con el final del vuelo */
    while (k < cant_eventos) {
        agregar_muestra(parametros, resultados, tiempos_evento[k], tipos_evento[k], telemetria);
        k++;
    }
}

void imprimir_encabezado_telemetria(void)
{
    printf("\n      t (s) |  Alt. (km) |   v (m/s) | a (m/s²) | Fase | Perfil de altitud\n");
    printf("  ----------+------------+-----------+----------+------+---------------------\n");
}

void mostrar_telemetria(const Telemetria *telemetria, double altitud_referencia)
{
    int i;
    const Muestra_Telemetria *muestra;

    imprimir_encabezado_telemetria();
    for (i = 0; i < telemetria->cantidad; i++) {
        muestra = &telemetria->muestras[i];
        if (muestra->evento != EVENTO_NINGUNO) {
            printf("  >> T+%.2f s  %s\n", muestra->t, nombre_evento(muestra->evento));
        }
        printf("  %9.2f | %10.3f | %9.2f | %8.2f | %-4s ",
               muestra->t, muestra->y / 1000.0, muestra->v, muestra->a, codigo_fase(muestra->fase));
        dibujar_barra(muestra->y, altitud_referencia);
        printf("\n");

        if ((i + 1) % FILAS_POR_PAGINA == 0 && i + 1 < telemetria->cantidad) {
            pausar();
            imprimir_encabezado_telemetria();
        }
    }
    printf("\n  Fases: SRB = núcleo + cohetes sólidos | NUC = solo núcleo | BAL = vuelo balístico\n");
}

/* Busca los máximos registrados y actualiza los récords de la sesión */
void reporte_simulacion(const Telemetria *telemetria, const Resultados_Mision *resultados,
                        Historial_Sesion *historial)
{
    const Muestra_Telemetria *actual;
    const Muestra_Telemetria *mayor_v = telemetria->muestras;
    const Muestra_Telemetria *mayor_y = telemetria->muestras;
    const Muestra_Telemetria *fin = telemetria->muestras + telemetria->cantidad;  /* una posición después de la última */

    /* Recorrido del arreglo con aritmética de punteros */
    for (actual = telemetria->muestras; actual < fin; actual++) {
        if (actual->v > mayor_v->v) {
            mayor_v = actual;
        }
        if (actual->y > mayor_y->y) {
            mayor_y = actual;
        }
    }

    imprimir_titulo("REPORTE FINAL DE LA SIMULACIÓN");
    printf("  Muestras registradas ............ %d\n", telemetria->cantidad);
    printf("  Rapidez máxima registrada ....... %.2f m/s (T+%.2f s)\n", mayor_v->v, mayor_v->t);
    printf("  Altitud máxima registrada ....... %.3f km (T+%.2f s)\n", mayor_y->y / 1000.0, mayor_y->t);
    printf("  Duración del vuelo propulsado ... %.2f s\n", resultados->t_meco);
    printf("  Aceleración máxima fase 1 ....... %.2f g (carga sentida: %.2f g)\n",
           resultados->factor_g, resultados->carga_g);

    printf("\n  Estado de la misión: ");
    if (resultados->factor_g > LIMITE_G_TRIPULACION) {
        printf("ALERTA, aceleración excesiva para la tripulación.\n");
    } else if (!resultados->desacople_previo) {
        printf("ANOMALÍA, MECO antes del desacople programado.\n");
    } else {
        printf("NOMINAL, ascenso completado.\n");
    }

    historial->simulaciones_realizadas++;
    if (mayor_y->y > historial->apogeo_record) {
        historial->apogeo_record = mayor_y->y;
    }
    if (mayor_v->v > historial->rapidez_record) {
        historial->rapidez_record = mayor_v->v;
    }
}

/* Opción 3 del menú */
void ejecutar_simulacion(const Parametros_Mision *parametros, Historial_Sesion *historial)
{
    Telemetria telemetria;
    Resultados_Mision resultados;
    int con_balistica = 0;
    double t_fin, altitud_referencia, dt_minimo, dt;

    limpiar_pantalla();
    imprimir_titulo("SIMULACIÓN DE VUELO CON TELEMETRÍA");
    calcular_mision(parametros, &resultados);
    mostrar_parametros(parametros);
    printf("\n");

    if (leer_si_no("  ¿Simular la falla de motores en h y seguir hasta el apogeo? (S/N): ") == 'S') {
        con_balistica = 1;
    }
    if (con_balistica) {
        t_fin = resultados.t_apogeo;
        altitud_referencia = resultados.h_max;
    } else {
        t_fin = resultados.t_meco;
        altitud_referencia = parametros->h;
    }

    /* El paso mínimo garantiza que todas las muestras entren en el arreglo */
    dt_minimo = ceil(t_fin / (MAX_MUESTRAS - 5) * 100.0) / 100.0;
    if (dt_minimo < DT_MIN) {
        dt_minimo = DT_MIN;
    }
    printf("  Tiempo a simular: %.2f s. Paso permitido: de %.2f a %.0f s.\n", t_fin, dt_minimo, DT_MAX);
    dt = leer_real("  Paso de telemetría Δt (s): ", dt_minimo, DT_MAX);

    generar_telemetria(parametros, &resultados, dt, con_balistica, &telemetria);
    printf("\n  Cuenta regresiva: T-3 ... T-2 ... T-1 ... ¡IGNICIÓN!\n");
    mostrar_telemetria(&telemetria, altitud_referencia);
    reporte_simulacion(&telemetria, &resultados, historial);
    pausar();
}


/* ====================================================================== */
/*                     DESAFÍO DEL DIRECTOR DE VUELO                      */
/* ====================================================================== */

/* Valor al azar entre minimo y maximo, en múltiplos de paso */
double numero_aleatorio(double minimo, double maximo, double paso)
{
    int cantidad_pasos = (int) ((maximo - minimo) / paso + 0.5);

    return minimo + (rand() % (cantidad_pasos + 1)) * paso;
}

/* Sortea una misión en la que los SRB siempre se desacoplan antes de llegar a h */
void generar_mision_aleatoria(Parametros_Mision *parametros)
{
    double y_desacople;

    parametros->a1 = numero_aleatorio(15.0, 30.0, 0.5);
    parametros->t1 = numero_aleatorio(90.0, 150.0, 5.0);
    parametros->a2 = numero_aleatorio(8.0, 15.0, 0.5);
    parametros->g  = G_NOMINAL;
    y_desacople = 0.5 * parametros->a1 * parametros->t1 * parametros->t1;
    /* h queda entre 5 y 50 km por encima de y1, redondeado hacia arriba al km */
    parametros->h = ceil((y_desacople + numero_aleatorio(5000.0, 50000.0, 1000.0)) / 1000.0) * 1000.0;
}

/* Error relativo porcentual: |obtenido - esperado| / |esperado| · 100.
 * En el desafío los valores esperados siempre son positivos, así que no hay división por cero. */
double error_relativo(double obtenido, double esperado)
{
    return fabs(obtenido - esperado) / fabs(esperado) * 100.0;
}

int puntos_por_error(double error)
{
    if (error <= TOLERANCIA_EXACTA) {
        return PUNTOS_EXACTA;
    }
    if (error <= TOLERANCIA_BUENA) {
        return PUNTOS_BUENA;
    }
    if (error <= TOLERANCIA_ACEPTABLE) {
        return PUNTOS_ACEPTABLE;
    }
    return 0;
}

const char *calificacion(double porcentaje)
{
    if (porcentaje >= 90.0) {
        return "DIRECTOR/A DE VUELO, GO para la Luna";
    }
    if (porcentaje >= 70.0) {
        return "INGENIERO/A DE GUIADO, ascenso bajo control";
    }
    if (porcentaje >= 40.0) {
        return "ANALISTA DE TELEMETRÍA, conviene revisar algunas fórmulas";
    }
    return "CADETE, repasar MRUV y volver a intentar";
}

void mostrar_formulas(void)
{
    printf("\n  Fórmulas de apoyo:\n");
    printf("     v1 = a1·t1                  y1 = ½·a1·t1²\n");
    printf("     v2 = √(v1² + 2·a2·(h − y1))\n");
    printf("     hmáx = h + v2² / (2·g)      a1 / g\n");
}

/* Juega una ronda completa: sortea la misión, pide las 5 respuestas y las corrige */
void jugar_ronda(Ronda_Desafio *ronda, int numero, int total)
{
    const char *preguntas[CANT_PREGUNTAS] = {
        "  a) v1 al desacople (m/s) ........: ",
        "  a) y1 al desacople (m) ..........: ",
        "  b) v2 al apagar los motores (m/s): ",
        "  c) hmax tras la falla (m) .......: ",
        "  d) a1 / g (en g) ................: "
    };
    const char *nombres[CANT_PREGUNTAS] = { "a) v1 (m/s)", "a) y1 (m)", "b) v2 (m/s)", "c) hmax (m)", "d) a1 / g" };
    Resultados_Mision resultados;
    int i;

    generar_mision_aleatoria(&ronda->parametros);
    calcular_mision(&ronda->parametros, &resultados);
    ronda->correctas[0] = resultados.v1;
    ronda->correctas[1] = resultados.y1;
    ronda->correctas[2] = resultados.v2;
    ronda->correctas[3] = resultados.h_max;
    ronda->correctas[4] = resultados.factor_g;

    limpiar_pantalla();
    imprimir_titulo("DESAFÍO DEL DIRECTOR DE VUELO");
    printf("  Ronda %d de %d. Control de Misión transmite el perfil de lanzamiento:\n\n", numero, total);
    mostrar_parametros(&ronda->parametros);
    printf("\n  La nave parte del reposo (v0 = 0 m/s, y0 = 0 m). Se desprecia el aire.\n\n");

    printf("  Ver las fórmulas descuenta el %d%% del puntaje de esta ronda.\n", PENALIZACION_AYUDA);
    ronda->uso_ayuda = 0;
    if (leer_si_no("  ¿Desea verlas? (S/N): ") == 'S') {
        ronda->uso_ayuda = 1;
        mostrar_formulas();
    }

    printf("\n  Ingrese sus resultados (puede usar coma o punto decimal):\n");
    ronda->puntos = 0;
    for (i = 0; i < CANT_PREGUNTAS; i++) {
        ronda->respuestas[i] = leer_real(preguntas[i], 0.0, RESPUESTA_MAX);
        ronda->errores[i] = error_relativo(ronda->respuestas[i], ronda->correctas[i]);
        ronda->puntos = ronda->puntos + puntos_por_error(ronda->errores[i]);
    }
    if (ronda->uso_ayuda) {
        ronda->puntos = ronda->puntos * (100 - PENALIZACION_AYUDA) / 100;
    }

    printf("\n  %-14s %14s %14s %9s %5s\n", "Pregunta", "Su respuesta", "Correcto", "Error", "Pts");
    imprimir_separador();
    for (i = 0; i < CANT_PREGUNTAS; i++) {
        printf("  %-14s %14.2f %14.2f %8.2f%% %5d\n", nombres[i], ronda->respuestas[i],
               ronda->correctas[i], ronda->errores[i], puntos_por_error(ronda->errores[i]));
    }
    imprimir_separador();
    printf("  Puntaje de la ronda: %d / %d", ronda->puntos, CANT_PREGUNTAS * PUNTOS_EXACTA);
    if (ronda->uso_ayuda) {
        printf(" (con %d%% de descuento por usar las fórmulas)", PENALIZACION_AYUDA);
    }
    printf("\n");
    pausar();
}

/* Reporte final de la partida: puntaje, error promedio, mejor ronda y calificación */
void reporte_partida(const Ronda_Desafio rondas[], int cantidad, Resumen_Partida *resumen)
{
    int i, j;
    int mejor = 0;
    double error_ronda;
    double suma_errores = 0.0;
    double porcentaje;

    resumen->rondas = cantidad;
    resumen->puntaje = 0;
    resumen->puntaje_maximo = cantidad * CANT_PREGUNTAS * PUNTOS_EXACTA;

    imprimir_titulo("REPORTE FINAL DE DESEMPEÑO");
    printf("  Ronda |   a1   |   t1   |   a2   |  h (km) | Ayuda | Error medio | Puntos\n");
    printf("  ------+--------+--------+--------+---------+-------+-------------+-------\n");
    for (i = 0; i < cantidad; i++) {
        error_ronda = 0.0;
        for (j = 0; j < CANT_PREGUNTAS; j++) {
            error_ronda = error_ronda + rondas[i].errores[j];
        }
        error_ronda = error_ronda / CANT_PREGUNTAS;

        printf("  %5d | %6.2f | %6.1f | %6.2f | %7.0f |", i + 1, rondas[i].parametros.a1,
               rondas[i].parametros.t1, rondas[i].parametros.a2, rondas[i].parametros.h / 1000.0);
        if (rondas[i].uso_ayuda) {
            printf("  Sí   |");
        } else {
            printf("  No   |");
        }
        printf(" %10.2f%% | %6d\n", error_ronda, rondas[i].puntos);

        resumen->puntaje = resumen->puntaje + rondas[i].puntos;
        suma_errores = suma_errores + error_ronda;
        if (rondas[i].puntos > rondas[mejor].puntos) {
            mejor = i;
        }
    }
    resumen->error_promedio = suma_errores / cantidad;
    porcentaje = 100.0 * resumen->puntaje / resumen->puntaje_maximo;

    printf("\n  Puntaje total ............. %d / %d (%.1f %%)\n",
           resumen->puntaje, resumen->puntaje_maximo, porcentaje);
    printf("  Error relativo promedio ... %.2f %%\n", resumen->error_promedio);
    printf("  Mejor ronda ............... %d (%d puntos)\n", mejor + 1, rondas[mejor].puntos);
    printf("  Calificación .............. %s\n\n", calificacion(porcentaje));
}

/* Opción 4 del menú */
void jugar_desafio(Historial_Sesion *historial)
{
    Ronda_Desafio rondas[MAX_RONDAS];
    Resumen_Partida resumen;
    int cantidad, i;

    limpiar_pantalla();
    imprimir_titulo("DESAFÍO DEL DIRECTOR DE VUELO");
    printf("  En cada ronda se sortea un perfil de lanzamiento distinto.\n");
    printf("  Hay que calcular v1, y1, v2, hmáx y a1/g. Puntos por respuesta:\n\n");
    printf("     error hasta %4.1f %%  ->  %3d puntos\n", TOLERANCIA_EXACTA, PUNTOS_EXACTA);
    printf("     error hasta %4.1f %%  ->  %3d puntos\n", TOLERANCIA_BUENA, PUNTOS_BUENA);
    printf("     error hasta %4.1f %%  ->  %3d puntos\n", TOLERANCIA_ACEPTABLE, PUNTOS_ACEPTABLE);
    printf("     error mayor         ->    0 puntos\n\n");
    printf("  Se pueden jugar entre 1 y %d rondas.\n", MAX_RONDAS);
    cantidad = leer_entero("  Cantidad de rondas: ", 1, MAX_RONDAS);

    for (i = 0; i < cantidad; i++) {
        jugar_ronda(&rondas[i], i + 1, cantidad);
    }

    limpiar_pantalla();
    reporte_partida(rondas, cantidad, &resumen);

    if (historial->cantidad_partidas < MAX_PARTIDAS) {
        resumen.numero = historial->cantidad_partidas + 1;
        historial->partidas[historial->cantidad_partidas] = resumen;
        historial->cantidad_partidas++;
        printf("  [OK] Partida guardada en el historial de la sesión.\n");
    } else {
        mostrar_aviso("El historial está lleno: esta partida no se guardó.");
    }
    pausar();
}


/* ====================================================================== */
/*                           REPORTES Y AYUDA                             */
/* ====================================================================== */

void inicializar_historial(Historial_Sesion *historial)
{
    historial->cantidad_partidas = 0;
    historial->simulaciones_realizadas = 0;
    historial->apogeo_record = 0.0;
    historial->rapidez_record = 0.0;
}

/* Opción 5 del menú (también se muestra al salir) */
void mostrar_reporte_sesion(const Historial_Sesion *historial)
{
    const Resumen_Partida *actual;
    const Resumen_Partida *mejor = historial->partidas;
    const Resumen_Partida *fin = historial->partidas + historial->cantidad_partidas;
    double porcentaje;
    double porcentaje_mejor;
    double suma_porcentajes = 0.0;

    imprimir_titulo("REPORTE DE DESEMPEÑO DE LA SESIÓN");
    printf("  Simulaciones de vuelo realizadas ... %d\n", historial->simulaciones_realizadas);
    if (historial->simulaciones_realizadas > 0) {
        printf("  Mayor altitud simulada ............. %.3f km\n", historial->apogeo_record / 1000.0);
        printf("  Mayor rapidez simulada ............. %.2f m/s\n", historial->rapidez_record);
    }
    printf("  Partidas del desafío jugadas ....... %d\n\n", historial->cantidad_partidas);

    if (historial->cantidad_partidas == 0) {
        mostrar_aviso("Todavía no se jugó ninguna partida del Desafío del Director de Vuelo.");
        return;
    }

    printf("  Partida | Rondas |     Puntaje     |      %% | Error medio\n");
    printf("  --------+--------+-----------------+--------+------------\n");
    /* Recorrido del historial con aritmética de punteros */
    for (actual = historial->partidas; actual < fin; actual++) {
        porcentaje = 100.0 * actual->puntaje / actual->puntaje_maximo;
        porcentaje_mejor = 100.0 * mejor->puntaje / mejor->puntaje_maximo;
        suma_porcentajes = suma_porcentajes + porcentaje;
        printf("  %7d | %6d | %6d / %6d | %5.1f%% | %9.2f%%\n", actual->numero, actual->rondas,
               actual->puntaje, actual->puntaje_maximo, porcentaje, actual->error_promedio);
        if (porcentaje > porcentaje_mejor) {
            mejor = actual;
        }
    }
    porcentaje_mejor = 100.0 * mejor->puntaje / mejor->puntaje_maximo;
    printf("\n  Rendimiento promedio ... %.1f %%\n", suma_porcentajes / historial->cantidad_partidas);
    printf("  Mejor partida .......... N.º %d (%.1f %%), %s\n",
           mejor->numero, porcentaje_mejor, calificacion(porcentaje_mejor));
}

/* Opción 6 del menú */
void mostrar_ayuda(void)
{
    limpiar_pantalla();
    imprimir_titulo("REGLAS Y AYUDA");
    printf("  ESCENARIO: el SLS despega desde el reposo y sube en línea recta.\n");
    printf("   - Fase 1: núcleo + SRB con aceleración constante a1 hasta t1.\n");
    printf("   - Fase 2: se sueltan los SRB y el núcleo acelera con a2 hasta la altitud h.\n");
    printf("   - En h se apagan los motores (MECO). Si fallan, la nave sigue subiendo\n");
    printf("     en caída libre (a = −g) hasta su altitud máxima. Se desprecia el aire.\n\n");
    printf("  MENÚ:\n");
    printf("   1. Configurar: cambia a1, t1, a2, h y g dentro de rangos válidos.\n");
    printf("   2. Análisis: resuelve a), b), c) y d) paso a paso.\n");
    printf("   3. Simulación: telemetría cada Δt con los eventos del vuelo.\n");
    printf("   4. Desafío: misiones al azar; usted calcula y suma puntos.\n");
    printf("   5. Reporte: desempeño acumulado de la sesión.\n\n");
    printf("  INGRESO DE DATOS:\n");
    printf("   - Los decimales se escriben con coma o con punto (12,5 o 12.5).\n");
    printf("   - Letras, valores vacíos y números fuera de rango se rechazan\n");
    printf("     y se vuelve a pedir el dato.\n");
}
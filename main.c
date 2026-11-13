/*
 * main.c
 * Módulo Principal: inicialización y menú principal.
 * No hace cálculos: cada opción se deriva al módulo que corresponde.
 *
 * Trabajo Práctico Integrador Final - Algoritmos y Estructuras de Datos
 * UTN Facultad Regional Resistencia
 * Simulador Artemis - Situación - Problema 1: ascenso del SLS
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "artemis.h"
#include "validaciones.h"

int main(void)
{
    Parametros_Mision parametros;
    Resultados_Mision resultados;
    Historial_Sesion historial;
    int opcion;

    /* Inicialización */
    configurar_consola();
    srand((unsigned int) time(NULL));      /* semilla para las misiones al azar del desafío */
    cargar_valores_nominales(&parametros);
    inicializar_historial(&historial);

    do {
        limpiar_pantalla();
        imprimir_titulo("SIMULADOR ARTEMIS - ASCENSO DEL SLS (MRUV)");
        mostrar_parametros(&parametros);
        printf("\n");
        printf("  1. Configurar parámetros de la misión\n");
        printf("  2. Análisis del ascenso (preguntas a, b, c y d)\n");
        printf("  3. Simulación de vuelo con telemetría\n");
        printf("  4. Desafío del Director de Vuelo\n");
        printf("  5. Reporte de desempeño de la sesión\n");
        printf("  6. Reglas y ayuda\n");
        printf("  0. Salir\n\n");

        opcion = leer_entero("  Seleccione una opción: ", 0, 6);
        switch (opcion) {
            case 1:
                configurar_parametros(&parametros);
                break;
            case 2:
                calcular_mision(&parametros, &resultados);
                mostrar_analisis(&parametros, &resultados);
                pausar();
                break;
            case 3:
                ejecutar_simulacion(&parametros, &historial);
                break;
            case 4:
                jugar_desafio(&historial);
                break;
            case 5:
                limpiar_pantalla();
                mostrar_reporte_sesion(&historial);
                pausar();
                break;
            case 6:
                mostrar_ayuda();
                pausar();
                break;
            case 0:
                if (leer_si_no("  ¿Confirma que desea salir? (S/N): ") == 'N') {
                    opcion = -1;           /* se cancela la salida y se vuelve al menú */
                }
                break;
        }
    } while (opcion != 0);

    /* Reporte final de desempeño al cerrar */
    limpiar_pantalla();
    mostrar_reporte_sesion(&historial);
    printf("\n  Fin de la simulación. ¡Buen viaje, Orion!\n\n");
    return 0;
}
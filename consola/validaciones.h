/*
 * validaciones.h
 * Módulo de Validaciones e Interfaz.
 * Ingreso seguro de enteros, reales y caracteres, y mensajes de consola.
 */
#ifndef VALIDACIONES_H
#define VALIDACIONES_H

#define TAM_LINEA 128   /* una línea de entrada admite hasta 127 caracteres */

/* Consola */
void configurar_consola(void);
void limpiar_pantalla(void);
void pausar(void);
void imprimir_titulo(const char *titulo);
void imprimir_separador(void);

/* Mensajes */
void mostrar_error(const char *mensaje);
void mostrar_aviso(const char *mensaje);

/* Ingreso seguro: repiten la pregunta hasta recibir un dato válido */
int    leer_entero(const char *mensaje, int minimo, int maximo);
double leer_real(const char *mensaje, double minimo, double maximo);
char   leer_si_no(const char *mensaje);   /* devuelve 'S' o 'N' */

#endif
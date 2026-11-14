/*
 * validaciones.c
 * Ingreso seguro de datos y mensajes de consola.
 *
 * Idea central: nunca se usa scanf. Cada dato se lee como una línea completa
 * con fgets() y recién después se revisa y se convierte. Así una letra no
 * queda trabada en el buffer de entrada y el programa no entra en un bucle
 * infinito ante un error del usuario.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "validaciones.h"

#ifdef _WIN32
#include <windows.h>
#endif

/* Funciones internas del módulo (no se publican en validaciones.h) */
int  leer_linea(char *linea);
char *recortar_espacios(char *texto);
void cambiar_coma_por_punto(char *texto);
int  caracter_permitido(char c, int admite_decimales);
int  es_numero(const char *texto, int admite_decimales);


/* ======================= Funciones internas ======================= */

/*
 * Lee una línea del teclado en 'linea' (de tamaño TAM_LINEA).
 * Devuelve 1 si entró completa y 0 si era demasiado larga; en ese caso
 * descarta lo que sobró para que no afecte a la próxima lectura.
 */
int leer_linea(char *linea)
{
    int largo;
    int c;

    fflush(stdout);
    if (fgets(linea, TAM_LINEA, stdin) == NULL) {
        /* Se cerró la entrada estándar (Ctrl+D o Ctrl+Z): se termina en orden */
        printf("\n\n  Se cerró la entrada de datos. El programa finaliza.\n");
        exit(0);
    }

    largo = (int) strlen(linea);
    if (largo > 0 && linea[largo - 1] == '\n') {
        linea[largo - 1] = '\0';          /* se quita el salto de línea */
        return 1;
    }
    if (feof(stdin)) {                    /* última línea sin salto: es válida */
        return 1;
    }

    c = getchar();                        /* no llegó el '\n': sobran caracteres */
    while (c != '\n' && c != EOF) {
        c = getchar();
    }
    return 0;
}

/* Saltea los espacios iniciales, borra los finales y devuelve un puntero
 * al primer carácter útil del texto. */
char *recortar_espacios(char *texto)
{
    int ultimo;

    while (*texto == ' ' || *texto == '\t') {
        texto++;
    }
    ultimo = (int) strlen(texto) - 1;
    while (ultimo >= 0 && (texto[ultimo] == ' ' || texto[ultimo] == '\t' || texto[ultimo] == '\r')) {
        texto[ultimo] = '\0';
        ultimo--;
    }
    return texto;
}

/* Permite escribir "12,5" además de "12.5" */
void cambiar_coma_por_punto(char *texto)
{
    int i;

    for (i = 0; texto[i] != '\0'; i++) {
        if (texto[i] == ',') {
            texto[i] = '.';
        }
    }
}

/* Dígitos y signo siempre; punto y exponente (e/E) solo en los reales */
int caracter_permitido(char c, int admite_decimales)
{
    if (c >= '0' && c <= '9') {
        return 1;
    }
    if (c == '+' || c == '-') {
        return 1;
    }
    if (admite_decimales && (c == '.' || c == 'e' || c == 'E')) {
        return 1;
    }
    return 0;
}

/* Revisa carácter por carácter. Así se rechazan letras, "inf" y "nan". */
int es_numero(const char *texto, int admite_decimales)
{
    const char *p;

    for (p = texto; *p != '\0'; p++) {
        if (!caracter_permitido(*p, admite_decimales)) {
            return 0;
        }
    }
    return 1;
}


/* ======================= Consola ======================= */

void configurar_consola(void)
{
#ifdef _WIN32
    /* La consola de Windows pasa a UTF-8 para mostrar tildes, ², Δ y √ */
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif
}

void limpiar_pantalla(void)
{
#ifdef _WIN32
    system("cls");
#else
    printf("\033[2J\033[H");    /* secuencia de escape: borra la terminal y lleva el cursor al inicio */
#endif
}

void pausar(void)
{
    char linea[TAM_LINEA];

    printf("\n  Presione ENTER para continuar...");
    leer_linea(linea);
}

void imprimir_titulo(const char *titulo)
{
    printf("\n  ================================================================\n");
    printf("    %s\n", titulo);
    printf("  ================================================================\n\n");
}

void imprimir_separador(void)
{
    printf("  ----------------------------------------------------------------\n");
}


/* ======================= Mensajes ======================= */

void mostrar_error(const char *mensaje)
{
    printf("  [ERROR] %s\n", mensaje);
}

void mostrar_aviso(const char *mensaje)
{
    printf("  [AVISO] %s\n", mensaje);
}


/* ======================= Ingreso seguro ======================= */

/*
 * Pide un entero en [minimo, maximo]. Pasos:
 * 1) lee la línea, 2) recorta espacios, 3) revisa que sean dígitos,
 * 4) convierte con strtol() y 5) controla desborde y rango.
 */
int leer_entero(const char *mensaje, int minimo, int maximo)
{
    char linea[TAM_LINEA];
    char *texto;
    char *fin;              /* strtol() lo deja apuntando al primer carácter no convertido */
    long valor = 0;
    int valido = 0;

    while (!valido) {
        printf("%s", mensaje);
        if (!leer_linea(linea)) {
            mostrar_error("La entrada es demasiado larga.");
        } else {
            texto = recortar_espacios(linea);
            if (*texto == '\0') {
                mostrar_error("No se ingresó ningún valor.");
            } else if (!es_numero(texto, 0)) {
                mostrar_error("Debe ingresar un número entero, sin letras ni decimales.");
            } else {
                errno = 0;
                valor = strtol(texto, &fin, 10);
                if (*fin != '\0') {
                    mostrar_error("El formato del número no es válido.");
                } else if (errno == ERANGE || valor < minimo || valor > maximo) {
                    printf("  [ERROR] El valor debe estar entre %d y %d.\n", minimo, maximo);
                } else {
                    valido = 1;
                }
            }
        }
    }
    return (int) valor;
}

/*
 * Pide un número real en [minimo, maximo]. Igual que leer_entero(),
 * pero acepta coma o punto decimal y notación científica (1.6e5).
 */
double leer_real(const char *mensaje, double minimo, double maximo)
{
    char linea[TAM_LINEA];
    char *texto;
    char *fin;
    double valor = 0.0;
    int valido = 0;

    while (!valido) {
        printf("%s", mensaje);
        if (!leer_linea(linea)) {
            mostrar_error("La entrada es demasiado larga.");
        } else {
            texto = recortar_espacios(linea);
            cambiar_coma_por_punto(texto);
            if (*texto == '\0') {
                mostrar_error("No se ingresó ningún valor.");
            } else if (!es_numero(texto, 1)) {
                mostrar_error("Debe ingresar un número (por ejemplo: 20 | 12,5 | 1.6e5).");
            } else {
                errno = 0;
                valor = strtod(texto, &fin);
                if (*fin != '\0') {
                    mostrar_error("El formato del número no es válido.");
                } else if (errno == ERANGE || valor < minimo || valor > maximo) {
                    printf("  [ERROR] El valor debe estar entre %.2f y %.2f.\n", minimo, maximo);
                } else {
                    valido = 1;
                }
            }
        }
    }
    return valor;
}

/* Pide una respuesta de un solo carácter: S o N, en mayúscula o minúscula */
char leer_si_no(const char *mensaje)
{
    char linea[TAM_LINEA];
    char *texto;
    char respuesta = ' ';

    while (respuesta == ' ') {
        printf("%s", mensaje);
        if (leer_linea(linea)) {
            texto = recortar_espacios(linea);
            if (strlen(texto) == 1 && (texto[0] == 'S' || texto[0] == 's')) {
                respuesta = 'S';
            } else if (strlen(texto) == 1 && (texto[0] == 'N' || texto[0] == 'n')) {
                respuesta = 'N';
            }
        }
        if (respuesta == ' ') {
            mostrar_error("Responda S (sí) o N (no).");
        }
    }
    return respuesta;
}
/* Representacion grafica mediante GDI. La escena es ilustrativa:
   las nubes, los penachos y la perspectiva no alteran la fisica. */
#include "escena.h"

#include <math.h>
#include <stdio.h>

#define MARINO RGB(0, 41, 65)
#define AZUL RGB(182, 223, 237)
#define CORAL RGB(253, 110, 110)
#define TENUE RGB(126, 164, 183)
#define LINEA RGB(44, 80, 99)

static double limitar(double x, double minimo, double maximo)
{
    return x < minimo ? minimo : (x > maximo ? maximo : x);
}

static COLORREF mezclar(COLORREF a, COLORREF b, double fraccion)
{
    double k = limitar(fraccion, 0.0, 1.0);
    int rojo = (int)((double)GetRValue(a) + ((double)GetRValue(b) - (double)GetRValue(a)) * k + 0.5);
    int verde = (int)((double)GetGValue(a) + ((double)GetGValue(b) - (double)GetGValue(a)) * k + 0.5);
    int azul = (int)((double)GetBValue(a) + ((double)GetBValue(b) - (double)GetBValue(a)) * k + 0.5);
    return RGB(rojo, verde, azul);
}

static void relleno(HDC dc, int x, int y, int ancho, int alto, COLORREF color)
{
    RECT r = {x, y, x + ancho, y + alto};
    (void)SetDCBrushColor(dc, color);
    (void)FillRect(dc, &r, (HBRUSH)GetStockObject(DC_BRUSH));
}

static void elipse(HDC dc, double x, double y, double rx, double ry, COLORREF color)
{
    HGDIOBJ lapiz = SelectObject(dc, GetStockObject(NULL_PEN));
    HGDIOBJ brocha = SelectObject(dc, GetStockObject(DC_BRUSH));
    (void)SetDCBrushColor(dc, color);
    (void)Ellipse(dc, (int)(x - rx), (int)(y - ry), (int)(x + rx), (int)(y + ry));
    (void)SelectObject(dc, lapiz);
    (void)SelectObject(dc, brocha);
}

static void poligono(HDC dc, const POINT *puntos, int cantidad, COLORREF color)
{
    HGDIOBJ lapiz = SelectObject(dc, GetStockObject(NULL_PEN));
    HGDIOBJ brocha = SelectObject(dc, GetStockObject(DC_BRUSH));
    (void)SetDCBrushColor(dc, color);
    (void)Polygon(dc, puntos, cantidad);
    (void)SelectObject(dc, lapiz);
    (void)SelectObject(dc, brocha);
}

static void trazo(HDC dc, int x1, int y1, int x2, int y2, COLORREF color)
{
    HGDIOBJ previo = SelectObject(dc, GetStockObject(DC_PEN));
    (void)SetDCPenColor(dc, color);
    (void)MoveToEx(dc, x1, y1, NULL);
    (void)LineTo(dc, x2, y2);
    (void)SelectObject(dc, previo);
}

static void texto(HDC dc, HFONT fuente, int x, int y, int ancho, int alto,
                  COLORREF color, const char *cadena, UINT alineacion)
{
    RECT r = {x, y, x + ancho, y + alto};
    HGDIOBJ previa = SelectObject(dc, fuente);
    (void)SetTextColor(dc, color);
    (void)SetBkMode(dc, TRANSPARENT);
    (void)DrawTextA(dc, cadena, -1, &r,
                    DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | alineacion);
    (void)SelectObject(dc, previa);
}

static void burbuja(HDC dc, int x, int y, int ancho, int alto, COLORREF color)
{
    HGDIOBJ brocha = SelectObject(dc, GetStockObject(DC_BRUSH));
    HGDIOBJ lapiz = SelectObject(dc, GetStockObject(DC_PEN));
    (void)SetDCBrushColor(dc, color);
    (void)SetDCPenColor(dc, LINEA);
    (void)RoundRect(dc, x, y, x + ancho, y + alto, 22, 22);
    (void)SelectObject(dc, brocha);
    (void)SelectObject(dc, lapiz);
}

static void gradiente(HDC dc, int x, int y, int ancho, int alto,
                      COLORREF arriba, COLORREF abajo)
{
    if (alto <= 0) { return; }
    for (int fila = 0; fila < alto; fila += 3) {
        int paso = fila + 3 > alto ? alto - fila : 3;
        relleno(dc, x, y + fila, ancho, paso,
                mezclar(arriba, abajo, (double)fila / (double)alto));
    }
}

static COLORREF color_cielo(double altitud, int horizonte)
{
    const double alturas[6] = {0.0, 8000.0, 20000.0, 40000.0, 65000.0, 100000.0};
    const COLORREF cenit[6] = {RGB(60, 124, 204), RGB(40, 98, 180), RGB(21, 61, 131),
                              RGB(8, 28, 71), RGB(3, 11, 31), RGB(1, 4, 11)};
    const COLORREF borde[6] = {RGB(216, 235, 247), RGB(178, 213, 241), RGB(124, 176, 226),
                              RGB(60, 114, 187), RGB(22, 62, 126), RGB(8, 25, 61)};
    const COLORREF *colores = horizonte ? borde : cenit;
    for (int i = 1; i < 6; ++i) {
        if (altitud <= alturas[i]) {
            return mezclar(colores[i - 1], colores[i],
                           (altitud - alturas[i - 1]) / (alturas[i] - alturas[i - 1]));
        }
    }
    return horizonte ? mezclar(borde[5], RGB(3, 10, 27), (altitud - 100000.0) / 100000.0) : cenit[5];
}

static void dibujar_espacio(HDC dc, int ancho, int alto, double y, double reloj)
{
    double visible = limitar((y - 28000.0) / 50000.0, 0.0, 1.0);
    COLORREF fondo = color_cielo(y, 0);
    if (visible > 0.0) {
        for (int i = 0; i < 230; ++i) {
            int x = (i * 127 + 43) % ancho;
            int yy = (i * 193 + 71) % alto;
            double brillo = visible * (0.5 + 0.4 * sin((double)i * 1.7 + reloj * 0.35));
            COLORREF color = mezclar(fondo, RGB(236, 240, 255), brillo);
            (void)SetPixelV(dc, x, yy, color);
            if (i % 23 == 0) {
                trazo(dc, x - 3, yy, x + 4, yy, color);
                trazo(dc, x, yy - 3, x, yy + 4, color);
            }
        }
        if (y > 60000.0) {
            double k = limitar((y - 60000.0) / 40000.0, 0.0, 1.0);
            double x = (double)ancho * 0.19;
            double yy = (double)alto * 0.29;
            elipse(dc, x, yy, 15.0, 15.0, mezclar(fondo, RGB(225, 224, 217), k));
            elipse(dc, x - 4.0, yy - 3.0, 4.0, 3.5, mezclar(fondo, RGB(148, 148, 149), k));
            elipse(dc, x + 4.0, yy + 5.0, 3.0, 2.5, mezclar(fondo, RGB(148, 148, 149), k));
        }
    }
    {
        double sx = (double)ancho * 0.86;
        double sy = (double)alto * 0.29;
        double radio = 25.0 + 45.0 * (1.0 - limitar(y / 60000.0, 0.0, 1.0));
        COLORREF base = mezclar(color_cielo(y, 0), color_cielo(y, 1), 0.29);
        for (int i = 10; i >= 1; --i) {
            double r = radio * (double)i / 10.0;
            elipse(dc, sx, sy, r, r, mezclar(base, RGB(255, 248, 223), 0.13 * (11.0 - (double)i) / 10.0));
        }
        elipse(dc, sx, sy, 6.0, 6.0, RGB(255, 253, 246));
    }
}

static void dibujar_tierra(HDC dc, int ancho, int alto, double y)
{
    double visible = limitar((y - 28000.0) / 42000.0, 0.0, 1.0);
    double k = limitar((y - 60000.0) / 420000.0, 0.0, 1.0);
    double radio = (double)ancho * (3.6 - 2.3 * k);
    double cx = (double)ancho * 0.5;
    double cy = (double)alto * (0.73 + 0.12 * k) + radio;
    COLORREF cielo = color_cielo(y, 1);
    int guardado;
    HRGN region;
    if (visible <= 0.0) { return; }
    elipse(dc, cx, cy, radio + 15.0, radio + 15.0,
           mezclar(cielo, RGB(63, 133, 215), visible * 0.8));
    elipse(dc, cx, cy, radio + 4.0, radio + 4.0,
           mezclar(cielo, RGB(195, 232, 253), visible));
    guardado = SaveDC(dc);
    region = CreateEllipticRgn((int)(cx - radio), (int)(cy - radio),
                               (int)(cx + radio), (int)(cy + radio));
    if (region != NULL) {
        POINT origen = {0, 0};
        /* Las regiones usan coordenadas de dispositivo; la escena tiene
           un origen de ventana desplazado dentro de la interfaz. */
        (void)LPtoDP(dc, &origen, 1);
        (void)OffsetRgn(region, (int)origen.x, (int)origen.y);
        (void)ExtSelectClipRgn(dc, region, RGN_AND);
        gradiente(dc, 0, (int)(cy - radio), ancho, alto - (int)(cy - radio),
                  mezclar(cielo, RGB(70, 138, 203), visible),
                  mezclar(cielo, RGB(10, 53, 112), visible));
        for (int i = 0; i < 9; ++i) {
            double x = (double)((i * 157 + 35) % ancho);
            double borde = cy - sqrt(fmax(0.0, radio * radio - (x - cx) * (x - cx)));
            double yy = borde + (double)(alto - (int)borde) * (0.19 + (double)(i % 5) * 0.16);
            double rx = 18.0 + (double)((i * 23) % 61);
            elipse(dc, x, yy, rx, rx * 0.27, mezclar(cielo, RGB(95, 127, 72), visible));
            elipse(dc, x + rx * 0.6, yy + 5.0, rx * 0.6, rx * 0.21,
                   mezclar(cielo, RGB(116, 138, 79), visible));
        }
        for (int i = 0; i < 22; ++i) {
            double x = (double)((i * 89 + 12) % ancho);
            double borde = cy - sqrt(fmax(0.0, radio * radio - (x - cx) * (x - cx)));
            double yy = borde + (double)(alto - (int)borde) * (double)((i * 7) % 19) / 19.0;
            elipse(dc, x, yy, 14.0 + (double)(i % 5) * 6.0, 2.8,
                   mezclar(RGB(55, 110, 169), RGB(209, 229, 240), visible * 0.75));
        }
        (void)DeleteObject(region);
    }
    if (guardado != 0) { (void)RestoreDC(dc, guardado); }
}

static void dibujar_plataforma(HDC dc, int ancho, int alto, double base, double u, double y)
{
    double suelo = base + y * 0.35 * u;
    double cx = (double)ancho * 0.5;
    int nivel = (int)(suelo - 23.0 * u);
    if (suelo - 240.0 * u > (double)alto) { return; }
    if (nivel < alto) {
        gradiente(dc, 0, nivel, ancho, alto - nivel, RGB(126, 151, 104), RGB(43, 61, 32));
        relleno(dc, ancho * 3 / 5, nivel, ancho * 2 / 5, (int)(10.0 * u), RGB(117, 163, 192));
    }
    elipse(dc, cx, suelo + 7.0 * u, 117.0 * u, 11.0 * u, RGB(164, 169, 169));
    relleno(dc, (int)(cx - 60.0 * u), (int)suelo, (int)(120.0 * u), (int)(15.0 * u), RGB(91, 99, 109));
    {
        int izquierda = (int)(cx - 56.0 * u);
        int derecha = (int)(cx - 41.0 * u);
        int arriba = (int)(suelo - 202.0 * u);
        trazo(dc, izquierda, arriba, izquierda, (int)suelo, RGB(149, 156, 166));
        trazo(dc, derecha, arriba, derecha, (int)suelo, RGB(149, 156, 166));
        for (int i = 0; i < 18; ++i) {
            int yy = arriba + (int)((double)i * 11.0 * u);
            trazo(dc, izquierda, yy, derecha, yy + (int)(11.0 * u), RGB(138, 147, 158));
            trazo(dc, derecha, yy, izquierda, yy + (int)(11.0 * u), RGB(138, 147, 158));
        }
        trazo(dc, derecha, arriba + (int)(40.0 * u), (int)(cx - 9.0 * u),
              arriba + (int)(40.0 * u), RGB(174, 181, 191));
        trazo(dc, (int)(cx + 97.0 * u), (int)suelo,
              (int)(cx + 97.0 * u), (int)(suelo - 230.0 * u), RGB(122, 131, 142));
    }
}

static void dibujar_nubes(HDC dc, int ancho, int alto, double base, double u, double y)
{
    const double alturas[6] = {1700.0, 2600.0, 3900.0, 5600.0, 7800.0, 10500.0};
    for (int i = 0; i < 6; ++i) {
        double yy = base - (alturas[i] - y) * 0.05 * u;
        double cx = (double)ancho * (0.1 + (double)((i * 37) % 79) / 100.0);
        if (yy < -60.0 || yy > (double)alto + 60.0) { continue; }
        for (int j = 0; j < 7; ++j) {
            double r = (15.0 + (double)((i * 13 + j * 7) % 20)) * u;
            elipse(dc, cx + (double)(j - 3) * 13.0 * u,
                   yy + (double)((j * 11) % 15) * u, r * 1.3, r,
                   j % 2 ? RGB(238, 244, 250) : RGB(216, 229, 242));
        }
    }
}

static void dibujar_srb(HDC dc, double x, double base, double u)
{
    gradiente(dc, (int)(x - 6.0 * u), (int)(base - 112.0 * u),
              (int)(12.0 * u), (int)(101.0 * u), RGB(249, 247, 240), RGB(191, 197, 207));
    {
        POINT punta[3] = {{(int)(x - 6.0 * u), (int)(base - 112.0 * u)},
                          {(int)x, (int)(base - 124.0 * u)},
                          {(int)(x + 6.0 * u), (int)(base - 112.0 * u)}};
        poligono(dc, punta, 3, RGB(246, 245, 240));
    }
    relleno(dc, (int)(x - 6.0 * u), (int)(base - 107.0 * u),
            (int)(12.0 * u), (int)(3.0 * u) + 1, RGB(59, 65, 77));
    for (int i = 0; i < 4; ++i) {
        trazo(dc, (int)(x - 6.0 * u), (int)(base - (30.0 + 22.0 * (double)i) * u),
              (int)(x + 6.0 * u), (int)(base - (30.0 + 22.0 * (double)i) * u), RGB(125, 133, 144));
    }
    relleno(dc, (int)(x - 5.0 * u), (int)(base - 10.0 * u), (int)(10.0 * u), (int)(10.0 * u), RGB(75, 82, 94));
}

static void llama(HDC dc, double x, double base, double u, double largo, double reloj, COLORREF color)
{
    double l = largo * (0.94 + 0.06 * sin(reloj * 47.0 + x));
    POINT externa[5] = {{(int)(x - 7.0 * u), (int)base},
                        {(int)(x - 10.0 * u), (int)(base + l * u * 0.35)},
                        {(int)x, (int)(base + l * u)},
                        {(int)(x + 10.0 * u), (int)(base + l * u * 0.35)},
                        {(int)(x + 7.0 * u), (int)base}};
    POINT interna[3] = {{(int)(x - 3.0 * u), (int)base},
                        {(int)x, (int)(base + l * u * 0.64)},
                        {(int)(x + 3.0 * u), (int)base}};
    poligono(dc, externa, 5, color);
    poligono(dc, interna, 3, RGB(255, 251, 228));
}

static void dibujar_cohete(HDC dc, int ancho, int alto, double base, double u,
                           const Simulacion *s, double reloj)
{
    double x = (double)ancho * 0.5;
    int desacoplado = s->r.desacople_previo && s->r.dt2 > 1e-9 && s->t >= s->p.t1;
    int encendido = s->iniciado && s->t < s->r.t_meco;
    if (encendido) {
        if (!desacoplado) {
            llama(dc, x - 20.0 * u, base, u, 97.0, reloj, RGB(255, 193, 103));
            llama(dc, x + 20.0 * u, base, u, 97.0, reloj, RGB(255, 193, 103));
        }
        for (int i = -1; i <= 1; ++i) {
            llama(dc, x + (double)i * 7.0 * u, base, u, 64.0, reloj, RGB(150, 197, 247));
        }
    }
    /* Vapor decorativo acotado, reproducible al mover la linea de tiempo. */
    if (s->iniciado && s->t < 14.0 && s->estado.y < 3000.0) {
        for (int i = 0; i < 14; ++i) {
            double k = fmod(s->t * 0.35 + (double)i * 0.17, 1.0);
            double lado = i % 2 ? -1.0 : 1.0;
            elipse(dc, x + lado * (28.0 + 100.0 * k) * u,
                   base + s->estado.y * 0.035 * u + 13.0 * k * u,
                   (10.0 + 25.0 * k) * u, (10.0 + 12.0 * k) * u, RGB(224, 231, 236));
        }
    }
    relleno(dc, (int)(x - 13.0 * u), (int)(base - 128.0 * u), (int)(26.0 * u), (int)(119.0 * u), RGB(225, 124, 44));
    relleno(dc, (int)(x - 7.0 * u), (int)(base - 128.0 * u), (int)(8.0 * u), (int)(119.0 * u), RGB(250, 164, 86));
    for (int i = 0; i < 6; ++i) {
        int yy = (int)(base - (28.0 + (double)i * 16.0) * u);
        trazo(dc, (int)(x - 13.0 * u), yy, (int)(x + 13.0 * u), yy, RGB(166, 84, 30));
    }
    {
        POINT adaptador[4] = {{(int)(x - 13.0 * u), (int)(base - 128.0 * u)},
                              {(int)(x - 8.0 * u), (int)(base - 139.0 * u)},
                              {(int)(x + 8.0 * u), (int)(base - 139.0 * u)},
                              {(int)(x + 13.0 * u), (int)(base - 128.0 * u)}};
        POINT capsula[4] = {{(int)(x - 8.0 * u), (int)(base - 159.0 * u)},
                            {(int)(x - 3.0 * u), (int)(base - 171.0 * u)},
                            {(int)(x + 3.0 * u), (int)(base - 171.0 * u)},
                            {(int)(x + 8.0 * u), (int)(base - 159.0 * u)}};
        poligono(dc, adaptador, 4, RGB(235, 237, 242));
        relleno(dc, (int)(x - 8.0 * u), (int)(base - 159.0 * u), (int)(16.0 * u), (int)(20.0 * u), RGB(211, 216, 225));
        poligono(dc, capsula, 4, RGB(245, 246, 249));
        relleno(dc, (int)(x - 1.5 * u), (int)(base - 185.0 * u),
                (int)(3.0 * u) + 1, (int)(15.0 * u), RGB(220, 226, 235));
    }
    for (int lado = -1; lado <= 1; lado += 2) {
        double dx = 20.0 * u;
        double dy = 0.0;
        if (desacoplado) {
            double tau = s->t - s->p.t1;
            dx += u * (10.0 * (1.0 - exp(-tau / 1.2)) + 2.4 * tau);
            /* Separacion visual relativa; no altera el estado de Orion. */
            dy = u * 0.175 * (s->p.a2 + s->p.g) * tau * tau;
            if (tau > 15.0 || dy > (double)alto + 200.0) { continue; }
        }
        dibujar_srb(dc, x + (double)lado * dx, base + dy, u);
    }
}

static void dibujar_capas(HDC dc, int ancho, int alto, double base, double u,
                          double y, HFONT fuente)
{
    const double limites[4] = {12000.0, 50000.0, 85000.0, 100000.0};
    const char *nombres[4] = {"Tropopausa / 12 km", "Estratopausa / 50 km",
                             "Mesopausa / 85 km", "Linea de Karman / 100 km"};
    if (y < 3000.0) { return; }
    for (int i = 0; i < 4; ++i) {
        int yy = (int)(base - 90.0 * u - (limites[i] - y) * 0.012 * u);
        if (yy < 112 || yy > alto - 100) { continue; }
        for (int x = 0; x < ancho; x += 12) {
            trazo(dc, x, yy, x + 6, yy, mezclar(color_cielo(y, 0), AZUL, 0.55));
        }
        texto(dc, fuente, 13, yy - 24, ancho - 26, 22, AZUL, nombres[i], DT_LEFT);
    }
}

static void dibujar_hud(HDC dc, int ancho, int alto, const Simulacion *s,
                        HFONT fuente, HFONT mono)
{
    char linea[128];
    const char *titulos[4] = {"ALTITUD", "VELOCIDAD", "ACELERACION", "a1 / g"};
    const double valores[4] = {s->iniciado ? s->estado.y / 1000.0 : 0.0,
                               s->iniciado ? s->estado.v : 0.0,
                               s->iniciado ? s->estado.a : 0.0, s->r.factor_g};
    const char *unidades[4] = {"km", "m/s", "m/s^2", "g"};
    int tarjeta = (ancho - 35) / 4;
    burbuja(dc, 12, 12, ancho < 410 ? ancho - 24 : 300, 85, MARINO);
    (void)snprintf(linea, sizeof linea, "T+ %.2f s", s->t);
    texto(dc, mono, 26, 19, ancho - 55, 28, AZUL, linea, DT_LEFT);
    texto(dc, fuente, 26, 46, ancho - 55, 22, s->estado.fase == FASE_SRB ? CORAL : AZUL,
          s->iniciado ? nombre_fase(s->estado.fase) : "EN PLATAFORMA", DT_LEFT);
    texto(dc, fuente, 26, 69, ancho - 55, 20, TENUE,
          nombre_capa(s->iniciado ? s->estado.y : 0.0), DT_LEFT);
    for (int i = 0; i < 4; ++i) {
        int x = 10 + i * (tarjeta + 5);
        burbuja(dc, x, alto - 82, tarjeta, 70, MARINO);
        texto(dc, fuente, x + 9, alto - 77, tarjeta - 18, 21, TENUE, titulos[i], DT_LEFT);
        (void)snprintf(linea, sizeof linea, fabs(valores[i]) >= 1000000.0 ? "%.2e %s" : "%.2f %s", valores[i], unidades[i]);
        texto(dc, mono, x + 9, alto - 51, tarjeta - 18, 26, AZUL, linea, DT_LEFT);
    }
    if (s->iniciado && s->t >= s->t_fin) {
        int w = ancho - 44;
        burbuja(dc, 22, 111, w, 45, MARINO);
        texto(dc, fuente, 31, 115, w - 18, 36, CORAL,
              s->balistico ? "APOGEO / velocidad cero / a = -g" : "MECO / fin del ascenso propulsado", DT_CENTER);
    }
}

typedef struct { double altitud; int y; const char *nombre; COLORREF color; } Marca;

static int perfil_y(double altitud, double maxima, int arriba, int abajo)
{
    return abajo - (int)(limitar(altitud / maxima, 0.0, 1.0) * (double)(abajo - arriba));
}

static void dibujar_perfil(HDC dc, int x, int ancho, int alto, const Simulacion *s,
                           HFONT fuente, HFONT mono)
{
    const int arriba = 53;
    const int abajo = alto - 33;
    const int eje = x + 43;
    double maxima = (s->balistico ? s->r.h_max * 1.06 : s->p.h * 1.1);
    double altitud = s->iniciado ? s->estado.y : 0.0;
    Marca marcas[4];
    size_t cantidad = 0;
    char cadena[80];
    relleno(dc, x, 0, ancho, alto, MARINO);
    trazo(dc, x, 0, x, alto, LINEA);
    texto(dc, fuente, x + 12, 10, ancho - 24, 29, AZUL, "PERFIL / ALTITUD km", DT_LEFT);
    for (int i = 0; i < 4; ++i) {
        const double limites[5] = {0.0, 12000.0, 50000.0, 85000.0, maxima};
        int y1 = perfil_y(limites[i + 1], maxima, arriba, abajo);
        int y0 = perfil_y(limites[i], maxima, arriba, abajo);
        if (y0 > y1) {
            relleno(dc, x + 1, y1, ancho - 1, y0 - y1,
                    i % 2 ? RGB(9, 51, 74) : RGB(5, 45, 68));
        }
    }
    trazo(dc, eje, arriba, eje, abajo, LINEA);
    for (int i = 0; i <= 5; ++i) {
        double valor = maxima * (double)i / 5.0;
        int yy = perfil_y(valor, maxima, arriba, abajo);
        (void)snprintf(cadena, sizeof cadena, "%.0f", valor / 1000.0);
        texto(dc, mono, x + 2, yy - 9, 33, 20, TENUE, cadena, DT_RIGHT);
        trazo(dc, eje - 4, yy, eje, yy, TENUE);
    }
    {
        double tramos[4] = {0.0, s->r.desacople_previo ? s->r.y1 : s->p.h, s->p.h, s->r.h_max};
        for (int i = 0; i < (s->balistico ? 3 : 2); ++i) {
            int y0 = perfil_y(tramos[i], maxima, arriba, abajo);
            int y1 = perfil_y(tramos[i + 1], maxima, arriba, abajo);
            COLORREF color = i == 0 ? CORAL : (i == 1 ? AZUL : TENUE);
            int lleno = perfil_y(fmin(altitud, tramos[i + 1]), maxima, arriba, abajo);
            relleno(dc, eje + 6, y1, 10, y0 - y1, mezclar(MARINO, color, 0.25));
            if (altitud > tramos[i]) { relleno(dc, eje + 6, lleno, 10, y0 - lleno, color); }
        }
    }
    if (s->r.desacople_previo && s->r.dt2 > 1e-9) {
        marcas[cantidad++] = (Marca){s->r.y1, perfil_y(s->r.y1, maxima, arriba, abajo), "SRB", CORAL};
    }
    marcas[cantidad++] = (Marca){s->p.h, perfil_y(s->p.h, maxima, arriba, abajo), "MECO", AZUL};
    if (s->balistico) {
        marcas[cantidad++] = (Marca){s->r.h_max, perfil_y(s->r.h_max, maxima, arriba, abajo), "Apogeo", AZUL};
    }
    if (100000.0 < maxima) {
        int yy = perfil_y(100000.0, maxima, arriba, abajo);
        for (int xx = eje; xx < x + ancho; xx += 10) { trazo(dc, xx, yy, xx + 4, yy, TENUE); }
        marcas[cantidad++] = (Marca){100000.0, yy, "Karman", TENUE};
    }
    for (size_t i = 0; i < cantidad; ++i) {
        for (size_t j = i + 1; j < cantidad; ++j) {
            if (marcas[j].y < marcas[i].y) { Marca temp = marcas[i]; marcas[i] = marcas[j]; marcas[j] = temp; }
        }
        if (i > 0 && marcas[i].y < marcas[i - 1].y + 23) { marcas[i].y = marcas[i - 1].y + 23; }
    }
    if (cantidad > 0 && marcas[cantidad - 1].y > abajo) {
        int exceso = marcas[cantidad - 1].y - abajo;
        for (size_t i = 0; i < cantidad; ++i) { marcas[i].y -= exceso; }
    }
    for (size_t i = 0; i < cantidad; ++i) {
        trazo(dc, eje + 18, perfil_y(marcas[i].altitud, maxima, arriba, abajo),
              eje + 29, marcas[i].y, marcas[i].color);
        (void)snprintf(cadena, sizeof cadena, "%s %.0f", marcas[i].nombre, marcas[i].altitud / 1000.0);
        texto(dc, mono, eje + 33, marcas[i].y - 10, ancho - 81, 22, marcas[i].color, cadena, DT_LEFT);
    }
    elipse(dc, (double)(eje + 11), (double)perfil_y(altitud, maxima, arriba, abajo), 6.0, 6.0, CORAL);
    elipse(dc, (double)(eje + 11), (double)perfil_y(altitud, maxima, arriba, abajo), 2.5, 2.5, AZUL);
}

void dibujar_escena(HDC dc, RECT area, const Simulacion *sim,
                    double reloj_visual, HFONT fuente, HFONT mono)
{
    int ancho = (int)(area.right - area.left);
    int alto = (int)(area.bottom - area.top);
    int perfil;
    int vista;
    int guardado;
    double y;
    double u;
    double base;
    if (dc == NULL || sim == NULL || ancho < 250 || alto < 280) { return; }
    guardado = SaveDC(dc);
    if (guardado == 0) { return; }
    (void)IntersectClipRect(dc, (int)area.left, (int)area.top, (int)area.right, (int)area.bottom);
    /* Sumar al origen del padre conserva tambien su desplazamiento vertical. */
    (void)OffsetViewportOrgEx(dc, (int)area.left, (int)area.top, NULL);
    perfil = ancho < 600 ? 145 : ancho * 27 / 100;
    if (perfil > 245) { perfil = 245; }
    vista = ancho - perfil;
    y = sim->iniciado ? sim->estado.y : 0.0;
    u = fmin((double)alto / 600.0, (double)vista / 430.0);
    base = (double)alto * 0.75;
    gradiente(dc, 0, 0, vista, alto, color_cielo(y, 0), color_cielo(y, 1));
    dibujar_espacio(dc, vista, alto, y, reloj_visual);
    dibujar_tierra(dc, vista, alto, y);
    dibujar_plataforma(dc, vista, alto, base, u, y);
    dibujar_nubes(dc, vista, alto, base, u, y);
    dibujar_capas(dc, vista, alto, base, u, y, fuente);
    dibujar_cohete(dc, vista, alto, base, u, sim, reloj_visual);
    dibujar_hud(dc, vista, alto, sim, fuente, mono);
    dibujar_perfil(dc, vista, perfil, alto, sim, fuente, mono);
    (void)RestoreDC(dc, guardado);
}

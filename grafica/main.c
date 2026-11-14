/* Interfaz grafica nativa del simulador Artemis SLS (Win32/GDI). */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <string.h>

#include "fisica.h"
#include "escena.h"

#define ID_CAMPO 10
#define ID_LANZAR 100
#define ID_PAUSA 101
#define ID_REINICIAR 102
#define ID_NOMINAL 103
#define ID_BALISTICO 104
#define ID_LENTA 105
#define ID_VELOCIDAD 106
#define ID_TIEMPO 107
#define ID_DESACOPLE 120
#define ID_MECO 121
#define ID_APOGEO 122
#define ID_TIMER 1U
#define MARINO RGB(0, 41, 65)
#define AZUL RGB(182, 223, 237)
#define CORAL RGB(253, 110, 110)
#define PANEL RGB(13, 53, 75)
#define BORDE RGB(46, 83, 103)
#define TENUE RGB(139, 175, 193)

typedef struct {
    Simulacion sim;
    HWND ventana, campos[5], etiquetas[5], botones[6];
    HWND velocidad, etiqueta_velocidad, linea, salida, aviso, saltos[3];
    HFONT fuente, pequena, titulo, mono;
    HBRUSH brocha, brocha_panel;
    LARGE_INTEGER frecuencia, ultimo;
    double reloj_visual;
    int scroll_y, ancho, alto, contenido_alto;
} Aplicacion;

static void actualizar(Aplicacion *app);
static void colocar(Aplicacion *app);

static void escribir(HDC dc, HFONT fuente, COLORREF color, RECT rect,
                      const char *cadena, UINT formato)
{
    HGDIOBJ anterior = SelectObject(dc, fuente);
    (void)SetTextColor(dc, color);
    (void)SetBkMode(dc, TRANSPARENT);
    (void)DrawTextA(dc, cadena, -1, &rect, formato);
    (void)SelectObject(dc, anterior);
}

static void rotulo(HDC dc, HFONT fuente, COLORREF color, int x, int y,
                    int ancho, int alto, const char *cadena)
{
    RECT r = {x, y, x + ancho, y + alto};
    escribir(dc, fuente, color, r, cadena, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
}

static void capsula(HDC dc, RECT r, COLORREF fondo, COLORREF borde, int radio)
{
    HGDIOBJ lapiz = SelectObject(dc, GetStockObject(DC_PEN));
    HGDIOBJ brocha = SelectObject(dc, GetStockObject(DC_BRUSH));
    (void)SetDCPenColor(dc, borde);
    (void)SetDCBrushColor(dc, fondo);
    (void)RoundRect(dc, (int)r.left, (int)r.top, (int)r.right, (int)r.bottom, radio, radio);
    (void)SelectObject(dc, lapiz);
    (void)SelectObject(dc, brocha);
}

static RECT area_escena(const Aplicacion *app)
{
    RECT area = {328, 156, app->ancho - 24, app->contenido_alto - 180};
    return area;
}

static void pintar(HDC dc, const Aplicacion *app)
{
    RECT total = {0, 0, app->ancho, app->contenido_alto};
    RECT panel = {24, 156, 306, app->contenido_alto - 24};
    RECT escena = area_escena(app);
    RECT tiempo = {escena.left, escena.bottom + 16, escena.right, app->contenido_alto - 24};
    const char *fases[3] = {"FASE 1  /  NUCLEO + SRB", "FASE 2  /  SOLO NUCLEO", "BALISTICO  /  a = -g"};
    (void)FillRect(dc, &total, app->brocha);

    /* Estrellas discretas del fondo: posiciones estables, sin datos externos. */
    for (int i = 0; i < 90; ++i) {
        int x = (i * 157 + 39) % app->ancho;
        int y = (i * 239 + 17) % app->contenido_alto;
        (void)SetPixel(dc, x, y, RGB(43, 78, 99));
    }
    rotulo(dc, app->pequena, CORAL, 25, 19, app->ancho - 48, 21,
            "UTN FRRe  /  ALGORITMOS Y ESTRUCTURAS DE DATOS  /  SIMULADOR EN C");
    rotulo(dc, app->titulo, AZUL, 24, 44, app->ancho - 48, 51,
            "Simulador Artemis: ascenso del SLS");
    rotulo(dc, app->pequena, TENUE, 26, 96, app->ancho - 52, 23,
            "Del Centro Espacial Kennedy al apogeo. Movimiento vertical idealizado con MRUV por tramos.");
    for (int i = 0; i < 3; ++i) {
        RECT fase = {24 + i * 305, 125, 316 + i * 305, 147};
        capsula(dc, fase, PANEL, BORDE, 20);
        rotulo(dc, app->pequena, i == 0 ? CORAL : AZUL,
                (int)fase.left + 13, 125, 270, 22, fases[i]);
    }
    capsula(dc, panel, PANEL, BORDE, 30);
    rotulo(dc, app->fuente, AZUL, 44, 171, 245, 28, "Parametros de la mision");
    rotulo(dc, app->pequena, TENUE, 44, 197, 245, 21,
            "Se aplican al Lanzar o Reiniciar.");
    for (int i = 0; i < 5; ++i) {
        RECT campo = {44, 246 + i * 55, 286, 281 + i * 55};
        capsula(dc, campo, MARINO, BORDE, 28);
    }
    rotulo(dc, app->pequena, TENUE, 44, 822, 240, 21,
            "Coma o punto decimal. Sin separadores de miles.");
    capsula(dc, tiempo, PANEL, BORDE, 30);
    dibujar_escena(dc, escena, &app->sim, app->reloj_visual, app->fuente, app->mono);
}

static HWND crear_control(Aplicacion *app, const char *clase, const char *cadena,
                           DWORD estilo, int id, HFONT fuente)
{
    HWND h = CreateWindowExA(0, clase, cadena, WS_VISIBLE | WS_CHILD | estilo,
                             0, 0, 10, 10, app->ventana, (HMENU)(INT_PTR)id,
                             GetModuleHandleA(NULL), NULL);
    if (h != NULL) {
        (void)SendMessageA(h, WM_SETFONT, (WPARAM)fuente, TRUE);
    }
    return h;
}

static int crear_controles(Aplicacion *app)
{
    const char *etiquetas[5] = {"a1 - Aceleracion fase 1 [m/s^2]", "t1 - Desacople de SRB [s]",
                               "a2 - Aceleracion fase 2 [m/s^2]", "h - Altitud de apagado [m]", "g - Gravedad [m/s^2]"};
    const char *botones[6] = {"Lanzar", "Pausa", "Reiniciar", "Restaurar valores del enunciado",
                             "[x] Continuar hasta el apogeo", "[x] Camara lenta en eventos"};
    int correcto = 1;
    for (int i = 0; i < 5; ++i) {
        app->etiquetas[i] = crear_control(app, "STATIC", etiquetas[i], SS_LEFT,
                                          20 + i, app->pequena);
        app->campos[i] = crear_control(app, "EDIT", "", ES_AUTOHSCROLL | WS_TABSTOP,
                                       ID_CAMPO + i, app->mono);
        (void)SendMessageA(app->campos[i], EM_SETLIMITTEXT, 127, 0);
        correcto = correcto && app->campos[i] != NULL && app->etiquetas[i] != NULL;
    }
    for (int i = 0; i < 6; ++i) {
        app->botones[i] = crear_control(app, "BUTTON", botones[i], BS_OWNERDRAW | WS_TABSTOP,
                                        ID_LANZAR + i, app->fuente);
        correcto = correcto && app->botones[i] != NULL;
    }
    app->aviso = crear_control(app, "EDIT", "", ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL,
                                30, app->pequena);
    app->etiqueta_velocidad = crear_control(app, "STATIC", "Velocidad", SS_LEFT, 31, app->pequena);
    app->velocidad = crear_control(app, "COMBOBOX", "", CBS_DROPDOWNLIST | WS_TABSTOP | WS_VSCROLL,
                                    ID_VELOCIDAD, app->fuente);
    const char *velocidades[5] = {"1x tiempo real", "5x", "10x", "20x", "50x"};
    for (int i = 0; i < 5; ++i) {
        (void)SendMessageA(app->velocidad, CB_ADDSTRING, 0, (LPARAM)velocidades[i]);
    }
    (void)SendMessageA(app->velocidad, CB_SETCURSEL, 2, 0);
    app->linea = crear_control(app, TRACKBAR_CLASSA, "", TBS_HORZ | TBS_NOTICKS | WS_TABSTOP,
                                ID_TIEMPO, app->fuente);
    (void)SendMessageA(app->linea, TBM_SETRANGE, TRUE, MAKELPARAM(0, 10000));
    (void)SendMessageA(app->linea, TBM_SETLINESIZE, 0, 10);
    (void)SendMessageA(app->linea, TBM_SETPAGESIZE, 0, 100);
    app->salida = crear_control(app, "STATIC", "", SS_LEFT, 32, app->mono);
    for (int i = 0; i < 3; ++i) {
        app->saltos[i] = crear_control(app, "BUTTON", "", BS_OWNERDRAW | WS_TABSTOP,
                                       ID_DESACOPLE + i, app->pequena);
        correcto = correcto && app->saltos[i] != NULL;
    }
    return correcto && app->aviso != NULL && app->velocidad != NULL
           && app->etiqueta_velocidad != NULL && app->linea != NULL && app->salida != NULL;
}

static void mover(Aplicacion *app, HWND h, int x, int y, int w, int alto)
{
    (void)MoveWindow(h, x, y - app->scroll_y, w, alto, TRUE);
}

static void colocar(Aplicacion *app)
{
    RECT area;
    (void)GetClientRect(app->ventana, &area);
    app->ancho = (int)area.right;
    app->alto = (int)area.bottom;
    app->contenido_alto = app->alto < 870 ? 870 : app->alto;
    if (app->ancho < 100) { return; }
    for (int i = 0; i < 5; ++i) {
        mover(app, app->etiquetas[i], 44, 225 + i * 55, 244, 21);
        mover(app, app->campos[i], 58, 252 + i * 55, 212, 25);
    }
    mover(app, app->aviso, 44, 513, 242, 65);
    mover(app, app->botones[0], 44, 585, 242, 41);
    mover(app, app->botones[1], 44, 637, 116, 35);
    mover(app, app->botones[2], 170, 637, 116, 35);
    mover(app, app->etiqueta_velocidad, 44, 685, 79, 25);
    mover(app, app->velocidad, 127, 681, 159, 180);
    mover(app, app->botones[4], 44, 722, 242, 32);
    mover(app, app->botones[5], 44, 760, 242, 32);
    mover(app, app->botones[3], 44, 794, 242, 25);
    RECT escena = area_escena(app);
    int ancho = (int)(escena.right - escena.left);
    int base = (int)escena.bottom + 16;
    mover(app, app->salida, (int)escena.left + 16, base + 14, ancho - 32, 37);
    mover(app, app->linea, (int)escena.left + 14, base + 54, ancho - 28, 28);
    for (int i = 0; i < 3; ++i) {
        int campo = (ancho - 48) / 3;
        mover(app, app->saltos[i], (int)escena.left + 16 + i * (campo + 8),
               base + 92, campo, 30);
    }
}

static void ajustar_scroll(Aplicacion *app)
{
    RECT area;
    SCROLLINFO info = {0};
    (void)GetClientRect(app->ventana, &area);
    app->alto = (int)area.bottom;
    app->contenido_alto = app->alto < 870 ? 870 : app->alto;
    int maximo = app->contenido_alto - app->alto;
    if (app->scroll_y > maximo) { app->scroll_y = maximo; }
    info.cbSize = sizeof info;
    info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    info.nMin = 0;
    info.nMax = app->contenido_alto - 1;
    info.nPage = (UINT)app->alto;
    info.nPos = app->scroll_y;
    (void)SetScrollInfo(app->ventana, SB_VERT, &info, TRUE);
    colocar(app);
}

static void desplazar(Aplicacion *app, int posicion)
{
    int maximo = app->contenido_alto - app->alto;
    if (posicion < 0) { posicion = 0; }
    if (posicion > maximo) { posicion = maximo; }
    app->scroll_y = posicion;
    (void)SetScrollPos(app->ventana, SB_VERT, posicion, TRUE);
    colocar(app);
    (void)InvalidateRect(app->ventana, NULL, TRUE);
}

static void cargar_campos(Aplicacion *app)
{
    const Parametros *p = &app->sim.p;
    const double valores[5] = {p->a1, p->t1, p->a2, p->h, p->g};
    char cadena[80];
    for (int i = 0; i < 5; ++i) {
        (void)snprintf(cadena, sizeof cadena, "%.15g", valores[i]);
        for (size_t j = 0; cadena[j] != '\0'; ++j) {
            if (cadena[j] == '.') { cadena[j] = ','; }
        }
        (void)SetWindowTextA(app->campos[i], cadena);
    }
}

static void mostrar_avisos(Aplicacion *app)
{
    char cadena[500] = "";
    const Parametros *p = &app->sim.p;
    if (!app->sim.r.desacople_previo) {
        (void)strcat(cadena, "MECO anterior al desacople previsto.\r\n");
    }
    if (app->sim.r.factor_g > 4.0) {
        (void)strcat(cadena, "a1/g supera 4: referencia educativa, no limite de seguridad.\r\n");
    }
    if (p->a2 > p->a1) {
        (void)strcat(cadena, "a2 > a1: permitido en este modelo.\r\n");
    }
    if (cadena[0] == '\0') {
        (void)strcpy(cadena, "Configuracion valida.\r\nAceleraciones netas; sin rozamiento.");
    }
    (void)SetWindowTextA(app->aviso, cadena);
}

static void actualizar(Aplicacion *app)
{
    const Simulacion *s = &app->sim;
    char cadena[256];
    int terminado = s->t >= s->t_fin;
    (void)SetWindowTextA(app->botones[0], terminado ? "Lanzar de nuevo" : "Lanzar");
    (void)SetWindowTextA(app->botones[1], s->corriendo ? "Pausa" : "Continuar");
    (void)EnableWindow(app->botones[1], s->iniciado && !terminado);
    (void)SetWindowTextA(app->botones[4], s->balistico ? "[x] Continuar hasta el apogeo" : "[ ] Terminar en MECO");
    (void)SetWindowTextA(app->botones[5], s->camara_lenta ? "[x] Camara lenta en eventos" : "[ ] Camara lenta en eventos");
    (void)snprintf(cadena, sizeof cadena,
        "T+ %.2f / %.2f s  |  %.2f km  |  v: %.2f m/s\r\na: %.2f m/s^2  |  %s  |  %s",
        s->t, s->t_fin, s->estado.y / 1000.0, s->estado.v,
        s->iniciado ? s->estado.a : 0.0,
        !s->iniciado ? "EN PLATAFORMA" : nombre_fase(s->estado.fase),
        s->corriendo ? "EN VUELO" : (terminado ? "FINALIZADA" : "PAUSADA"));
    (void)SetWindowTextA(app->salida, cadena);
    int posicion = (int)(10000.0 * s->t / s->t_fin);
    (void)SendMessageA(app->linea, TBM_SETPOS, TRUE, (LPARAM)posicion);
    (void)InvalidateRect(app->ventana, NULL, FALSE);
}

static void actualizar_saltos(Aplicacion *app)
{
    char cadena[100];
    const Resultados *r = &app->sim.r;
    (void)snprintf(cadena, sizeof cadena, "Desacople  %.2f s", app->sim.p.t1);
    (void)SetWindowTextA(app->saltos[0], cadena);
    (void)ShowWindow(app->saltos[0], r->desacople_previo && r->dt2 > 1e-9 ? SW_SHOW : SW_HIDE);
    (void)snprintf(cadena, sizeof cadena, "MECO  %.2f s", r->t_meco);
    (void)SetWindowTextA(app->saltos[1], cadena);
    (void)snprintf(cadena, sizeof cadena, "Apogeo  %.2f s", r->t_apogeo);
    (void)SetWindowTextA(app->saltos[2], cadena);
    (void)ShowWindow(app->saltos[2], app->sim.balistico ? SW_SHOW : SW_HIDE);
}

static int leer_formulario(Aplicacion *app, Parametros *p)
{
    double valores[5];
    const double minimos[5] = {A_MIN, T_MIN, A_MIN, H_MIN, G_MIN};
    const double maximos[5] = {A_MAX, T_MAX, A_MAX, H_MAX, G_MAX};
    const char *nombres[5] = {"a1", "t1", "a2", "h", "g"};
    char entrada[128], error[256];
    for (int i = 0; i < 5; ++i) {
        int longitud = GetWindowTextLengthA(app->campos[i]);
        if (longitud < (int)sizeof entrada) {
            (void)GetWindowTextA(app->campos[i], entrada, (int)sizeof entrada);
        } else {
            entrada[0] = '\0';
        }
        if (!convertir_numero(entrada, minimos[i], maximos[i], &valores[i])) {
            (void)snprintf(error, sizeof error,
                "%s: ingrese un numero finito entre %.1f y %.1f.\nSe acepta coma o punto decimal, sin unidades.",
                nombres[i], minimos[i], maximos[i]);
            (void)SetWindowTextA(app->aviso, error);
            app->sim.corriendo = 0;
            actualizar(app);
            (void)MessageBoxA(app->ventana, error, "Entrada invalida", MB_OK | MB_ICONWARNING);
            desplazar(app, 180 + i * 55);
            (void)SetFocus(app->campos[i]);
            (void)SendMessageA(app->campos[i], EM_SETSEL, 0, -1);
            return 0;
        }
    }
    *p = (Parametros){valores[0], valores[1], valores[2], valores[3], valores[4]};
    return 1;
}

static int preparar(Aplicacion *app, const Parametros *p, int balistico)
{
    double velocidad = app->sim.velocidad;
    int lenta = app->sim.camara_lenta;
    char error[ERROR_CAPACIDAD];
    if (!preparar_simulacion(&app->sim, p, balistico, error, sizeof error)) {
        (void)MessageBoxA(app->ventana, error, "Configuracion invalida", MB_OK | MB_ICONWARNING);
        return 0;
    }
    app->sim.velocidad = velocidad;
    app->sim.camara_lenta = lenta;
    actualizar_saltos(app);
    mostrar_avisos(app);
    return 1;
}

static void procesar_orden(Aplicacion *app, int id)
{
    Parametros p;
    switch (id) {
        case ID_LANZAR:
        case ID_REINICIAR:
            app->sim.corriendo = 0;
            if (leer_formulario(app, &p) && preparar(app, &p, app->sim.balistico)) {
                if (id == ID_LANZAR) { lanzar_simulacion(&app->sim); }
            }
            break;
        case ID_PAUSA:
            alternar_pausa(&app->sim);
            break;
        case ID_NOMINAL:
            app->sim.corriendo = 0;
            p = parametros_nominales();
            if (preparar(app, &p, app->sim.balistico)) { cargar_campos(app); }
            break;
        case ID_BALISTICO:
            p = app->sim.p;
            (void)preparar(app, &p, !app->sim.balistico);
            break;
        case ID_LENTA:
            app->sim.camara_lenta = !app->sim.camara_lenta;
            break;
        case ID_DESACOPLE:
            (void)fijar_tiempo(&app->sim, app->sim.p.t1);
            break;
        case ID_MECO:
            (void)fijar_tiempo(&app->sim, app->sim.r.t_meco);
            break;
        case ID_APOGEO:
            (void)fijar_tiempo(&app->sim, app->sim.r.t_apogeo);
            break;
        default:
            break;
    }
    actualizar(app);
}

static void pintar_boton(Aplicacion *app, const DRAWITEMSTRUCT *item)
{
    char cadena[128];
    RECT r = item->rcItem;
    int primario = item->CtlID == ID_LANZAR;
    int inactivo = (item->itemState & ODS_DISABLED) != 0U;
    int seleccionado = (item->itemState & ODS_SELECTED) != 0U;
    int enlace = item->CtlID == ID_NOMINAL;
    COLORREF fondo = primario ? CORAL : RGB(31, 74, 95);
    COLORREF color = primario ? MARINO : AZUL;
    if (inactivo) { fondo = PANEL; color = RGB(73, 109, 128); }
    if (seleccionado) { fondo = RGB(91, 120, 135); }
    if (enlace) { fondo = PANEL; color = CORAL; }
    capsula(item->hDC, r, fondo, enlace ? PANEL : BORDE, 32);
    (void)GetWindowTextA(item->hwndItem, cadena, (int)sizeof cadena);
    escribir(item->hDC, item->CtlID >= ID_DESACOPLE || enlace ? app->pequena : app->fuente,
              color, r, cadena, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    if ((item->itemState & ODS_FOCUS) != 0U) {
        (void)InflateRect(&r, -5, -5);
        (void)DrawFocusRect(item->hDC, &r);
    }
}

static LRESULT CALLBACK procedimiento(HWND ventana, UINT mensaje, WPARAM wp, LPARAM lp)
{
    Aplicacion *app = (Aplicacion *)GetWindowLongPtrA(ventana, GWLP_USERDATA);
    if (mensaje == WM_NCCREATE) {
        const CREATESTRUCTA *crear = (const CREATESTRUCTA *)lp;
        app = (Aplicacion *)crear->lpCreateParams;
        app->ventana = ventana;
        (void)SetWindowLongPtrA(ventana, GWLP_USERDATA, (LONG_PTR)app);
    }
    if (app == NULL) { return DefWindowProcA(ventana, mensaje, wp, lp); }
    switch (mensaje) {
        case WM_CREATE:
            if (!crear_controles(app)) { return -1; }
            cargar_campos(app);
            ajustar_scroll(app);
            actualizar_saltos(app);
            mostrar_avisos(app);
            actualizar(app);
            (void)QueryPerformanceCounter(&app->ultimo);
            if (SetTimer(ventana, ID_TIMER, 33U, NULL) == 0U) { return -1; }
            return 0;
        case WM_SIZE:
            ajustar_scroll(app);
            (void)InvalidateRect(ventana, NULL, FALSE);
            return 0;
        case WM_GETMINMAXINFO: {
            MINMAXINFO *m = (MINMAXINFO *)lp;
            m->ptMinTrackSize.x = 980;
            m->ptMinTrackSize.y = 520;
            return 0;
        }
        case WM_COMMAND:
            if (LOWORD(wp) == ID_VELOCIDAD && HIWORD(wp) == CBN_SELCHANGE) {
                const double factores[5] = {1, 5, 10, 20, 50};
                LRESULT i = SendMessageA(app->velocidad, CB_GETCURSEL, 0, 0);
                if (i >= 0 && i < 5) { app->sim.velocidad = factores[(size_t)i]; }
            } else if (HIWORD(wp) == BN_CLICKED) {
                procesar_orden(app, (int)LOWORD(wp));
            }
            return 0;
        case WM_HSCROLL:
            if ((HWND)lp == app->linea) {
                LRESULT pos = SendMessageA(app->linea, TBM_GETPOS, 0, 0);
                (void)fijar_tiempo(&app->sim, app->sim.t_fin * (double)pos / 10000.0);
                actualizar(app);
            }
            return 0;
        case WM_VSCROLL: {
            SCROLLINFO info = {0};
            info.cbSize = sizeof info;
            info.fMask = SIF_TRACKPOS;
            (void)GetScrollInfo(ventana, SB_VERT, &info);
            int pos = app->scroll_y;
            switch (LOWORD(wp)) {
                case SB_LINEUP: pos -= 35; break;
                case SB_LINEDOWN: pos += 35; break;
                case SB_PAGEUP: pos -= app->alto - 40; break;
                case SB_PAGEDOWN: pos += app->alto - 40; break;
                case SB_THUMBTRACK:
                case SB_THUMBPOSITION: pos = info.nTrackPos; break;
                case SB_TOP: pos = 0; break;
                case SB_BOTTOM: pos = app->contenido_alto; break;
                default: break;
            }
            desplazar(app, pos);
            return 0;
        }
        case WM_MOUSEWHEEL:
            desplazar(app, app->scroll_y - (int)(short)HIWORD(wp) / WHEEL_DELTA * 70);
            return 0;
        case WM_LBUTTONDOWN:
            (void)SetFocus(ventana);
            return 0;
        case WM_TIMER:
            if (wp == ID_TIMER) {
                LARGE_INTEGER ahora;
                (void)QueryPerformanceCounter(&ahora);
                double dt = (double)(ahora.QuadPart - app->ultimo.QuadPart)
                            / (double)app->frecuencia.QuadPart;
                app->ultimo = ahora;
                if (dt > 0.05) { dt = 0.05; }
                if (dt < 0.0) { dt = 0.0; }
                app->reloj_visual += dt;
                (void)avanzar_reloj(&app->sim, dt);
                actualizar(app);
            }
            return 0;
        case WM_DRAWITEM:
            if (((const DRAWITEMSTRUCT *)lp)->CtlType == ODT_BUTTON) {
                pintar_boton(app, (const DRAWITEMSTRUCT *)lp);
                return TRUE;
            }
            return 0;
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX: {
            HDC dc = (HDC)wp;
            HWND h = (HWND)lp;
            int campo = 0;
            for (int i = 0; i < 5; ++i) { campo = campo || h == app->campos[i]; }
            (void)SetTextColor(dc, h == app->aviso ? CORAL : AZUL);
            (void)SetBkColor(dc, campo ? MARINO : PANEL);
            return (LRESULT)(campo ? app->brocha : app->brocha_panel);
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(ventana, &ps);
            if (app->ancho > 0 && app->alto > 0) {
                HDC buffer = CreateCompatibleDC(dc);
                HBITMAP bitmap = CreateCompatibleBitmap(dc, app->ancho, app->alto);
                if (buffer != NULL && bitmap != NULL) {
                    HGDIOBJ anterior = SelectObject(buffer, bitmap);
                    (void)SetViewportOrgEx(buffer, 0, -app->scroll_y, NULL);
                    pintar(buffer, app);
                    (void)SetViewportOrgEx(buffer, 0, 0, NULL);
                    (void)BitBlt(dc, 0, 0, app->ancho, app->alto, buffer, 0, 0, SRCCOPY);
                    (void)SelectObject(buffer, anterior);
                } else {
                    int guardado = SaveDC(dc);
                    (void)SetViewportOrgEx(dc, 0, -app->scroll_y, NULL);
                    pintar(dc, app);
                    (void)RestoreDC(dc, guardado);
                }
                if (bitmap != NULL) { (void)DeleteObject(bitmap); }
                if (buffer != NULL) { (void)DeleteDC(buffer); }
            }
            (void)EndPaint(ventana, &ps);
            return 0;
        }
        case WM_DESTROY:
            app->sim.corriendo = 0;
            (void)KillTimer(ventana, ID_TIMER);
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcA(ventana, mensaje, wp, lp);
    }
}

static HFONT fuente(int alto, int peso, const char *familia)
{
    return CreateFontA(alto, 0, 0, 0, peso, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, familia);
}

static int iniciar_datos(Aplicacion *app)
{
    Parametros p = parametros_nominales();
    char error[ERROR_CAPACIDAD];
    if (!preparar_simulacion(&app->sim, &p, 1, error, sizeof error)) { return 0; }
    app->fuente = fuente(-16, FW_NORMAL, "Segoe UI");
    app->pequena = fuente(-13, FW_NORMAL, "Segoe UI");
    app->titulo = fuente(-40, FW_NORMAL, "Georgia");
    app->mono = fuente(-14, FW_NORMAL, "Consolas");
    app->brocha = CreateSolidBrush(MARINO);
    app->brocha_panel = CreateSolidBrush(PANEL);
    return app->fuente != NULL && app->pequena != NULL && app->titulo != NULL
        && app->mono != NULL && app->brocha != NULL && app->brocha_panel != NULL
        && QueryPerformanceFrequency(&app->frecuencia) && app->frecuencia.QuadPart > 0;
}

static void liberar_datos(Aplicacion *app)
{
    (void)DeleteObject(app->fuente);
    (void)DeleteObject(app->pequena);
    (void)DeleteObject(app->titulo);
    (void)DeleteObject(app->mono);
    (void)DeleteObject(app->brocha);
    (void)DeleteObject(app->brocha_panel);
}

int WINAPI WinMain(HINSTANCE instancia, HINSTANCE anterior, LPSTR argumentos, int mostrar)
{
    Aplicacion app = {0};
    INITCOMMONCONTROLSEX comunes = {sizeof comunes, ICC_BAR_CLASSES};
    WNDCLASSA clase = {0};
    RECT escritorio = {0, 0, 1366, 960};
    MSG mensaje;
    BOOL recibido;
    int codigo = 1;
    (void)anterior;
    (void)argumentos;
    if (!InitCommonControlsEx(&comunes) || !iniciar_datos(&app)) {
        (void)MessageBoxA(NULL, "No se pudo inicializar la interfaz.", "Artemis SLS", MB_OK | MB_ICONERROR);
        liberar_datos(&app);
        return 1;
    }
    clase.lpfnWndProc = procedimiento;
    clase.hInstance = instancia;
    clase.hCursor = LoadCursorA(NULL, IDC_ARROW);
    clase.hIcon = LoadIconA(NULL, IDI_APPLICATION);
    clase.lpszClassName = "ArtemisSLSNativoC";
    /* La aplicacion posee sus brochas; la clase no debe liberarlas. */
    clase.hbrBackground = NULL;
    if (RegisterClassA(&clase) != 0U) {
        (void)SystemParametersInfoA(SPI_GETWORKAREA, 0, &escritorio, 0);
        int ancho = (int)(escritorio.right - escritorio.left) - 48;
        int alto = (int)(escritorio.bottom - escritorio.top) - 48;
        if (ancho > 1400) { ancho = 1400; }
        if (alto > 980) { alto = 980; }
        if (ancho < 980) { ancho = 980; }
        if (alto < 520) { alto = 520; }
        HWND ventana = CreateWindowExA(0, clase.lpszClassName,
            "Artemis SLS - Simulador grafico en C", WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_CLIPCHILDREN,
            (int)escritorio.left + ((int)(escritorio.right - escritorio.left) - ancho) / 2,
            (int)escritorio.top + ((int)(escritorio.bottom - escritorio.top) - alto) / 2,
            ancho, alto, NULL, NULL, instancia, &app);
        if (ventana != NULL) {
            (void)ShowWindow(ventana, mostrar);
            (void)UpdateWindow(ventana);
            while ((recibido = GetMessageA(&mensaje, NULL, 0, 0)) > 0) {
                if (mensaje.message == WM_KEYDOWN && mensaje.wParam == VK_SPACE
                    && GetFocus() == ventana) {
                    procesar_orden(&app, !app.sim.iniciado || app.sim.t >= app.sim.t_fin ? ID_LANZAR : ID_PAUSA);
                } else if (!IsDialogMessageA(ventana, &mensaje)) {
                    (void)TranslateMessage(&mensaje);
                    (void)DispatchMessageA(&mensaje);
                }
            }
            codigo = recibido == -1 ? 1 : (int)mensaje.wParam;
            if (IsWindow(ventana)) { (void)DestroyWindow(ventana); }
        }
        (void)UnregisterClassA(clase.lpszClassName, instancia);
    }
    liberar_datos(&app);
    return codigo;
}

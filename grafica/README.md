# Simulador Artemis SLS — aplicación gráfica en C

Aplicación de escritorio para **Windows**, desarrollada en **C11** con controles **Win32** y dibujo **GDI**. Integra cálculo físico, interacción y representación visual en una aplicación nativa. Su objetivo es simular el ascenso del SLS en el escenario 1 del Trabajo Práctico Integrador Final de Algoritmos y Estructuras de Datos (UTN-FRRe).

## Abrir y compilar

El ejecutable se genera al compilar el proyecto y queda en `build/Artemis-SLS-Nativo.exe`. No se incluye el directorio `build/`, porque contiene archivos generados que pueden reconstruirse desde el código fuente.

Para compilar desde la carpeta que contiene el Makefile, disponer de GCC MinGW-w64 y GNU Make en `PATH`:

```powershell
mingw32-make all
mingw32-make run
mingw32-make test
mingw32-make clean
```

| Objetivo | Acción |
|---|---|
| `all` | Construir el ejecutable nativo |
| `run` | Construir y abrir la ventana |
| `test` | Compilar y ejecutar las pruebas |
| `clean` | Eliminar los productos de compilación |

La construcción enlaza `user32`, `gdi32`, `comctl32` y la biblioteca matemática. No se requieren paquetes gráficos externos ni Python para usar la aplicación. Esta ventana es específica de Windows; no se presenta como ejecutable para Linux.

## Uso

1. Revisar o escribir los cinco parámetros de la misión.
2. Pulsar **Lanzar** para validar los campos y comenzar desde el lanzamiento.
3. Elegir una velocidad: **1×, 5×, 10×, 20× o 50×**.
4. Usar **Pausa / Continuar** para detener y reanudar conservando el tiempo.
5. Mover la **línea de tiempo** para inspeccionar un instante, incluso uno anterior. La búsqueda deja la misión pausada.
6. Usar los saltos a desacople, MECO y apogeo, cuando correspondan a la configuración.

**Reiniciar** valida los parámetros y vuelve a preparar la misión. **Restaurar los valores del enunciado** recupera `a1=20`, `t1=120`, `a2=12.5`, `h=160000` y `g=9.8`.

Lanzar aplica el formulario y reinicia el vuelo; para reanudar una pausa se utiliza **Continuar**. Los valores escritos se aplican al pulsar Lanzar o Reiniciar. Cambiar de campo no reinicia automáticamente la misión.

### Opciones

- **Seguir hasta el apogeo:** activada, reproduce el ascenso balístico después del apagado; desactivada, termina en MECO. Esta opción no modela averías adicionales ni cambios en el empuje.
- **Cámara lenta:** reduce la reproducción a un máximo de 2× cerca de los eventos. Cambia el ritmo visual, no las ecuaciones ni los resultados.

La ventana adapta la distribución al tamaño disponible y permite desplazarse cuando el contenido no cabe. La escena y los controles están dibujados mediante recursos nativos del sistema operativo.

## Parámetros y validación

Se acepta punto o coma decimal, espacios exteriores y notación científica: `12,5`, `12.5`, `1.6e5`. No agregar separadores de miles ni unidades dentro de los campos.

| Parámetro | Unidad | Mínimo | Máximo |
|---|---|---:|---:|
| `a1`, aceleración neta de fase 1 | m/s² | 0,1 | 60 |
| `t1`, tiempo previsto de desacople | s | 1 | 600 |
| `a2`, aceleración neta de fase 2 | m/s² | 0,1 | 60 |
| `h`, altitud de apagado | m | 1.000 | 2.000.000 |
| `g`, gravedad constante | m/s² | 1 | 25 |

Son límites operativos elegidos para este simulador, no restricciones físicas oficiales de la misión. Se rechazan campos vacíos, formatos incompletos, texto sobrante, números no finitos y valores fuera de rango. Una configuración inválida no sustituye la misión válida anterior.

Se conserva la posibilidad de que el apagado ocurra **antes** del desacople previsto. Si `h < 0.5*a1*t1²`, se calcula el MECO anticipado durante la primera fase y se omite la segunda. Si ambas altitudes son iguales, la fase 2 tiene duración cero. Se trata de un caso de borde del simulador: el apagado puede anticiparse al desacople previsto.

El aviso cuando `a1/g` supera 4 es un **umbral educativo de advertencia**. No certifica seguridad de la tripulación ni representa un cálculo biomecánico. Las advertencias no agregan restricciones físicas.

## Modelo físico

Condiciones iniciales: `y0 = 0 m`, `v0 = 0 m/s`. Movimiento rectilíneo vertical sin resistencia del aire. Las aceleraciones `a1` y `a2` son **netas**; no se resta nuevamente la gravedad en las fases propulsadas.

Para una misión con desacople anterior al MECO:

```text
v1 = a1*t1
y1 = 0.5*a1*t1²
v2 = sqrt(v1² + 2*a2*(h-y1))
dt2 = 2*(h-y1)/(v2+v1)
t_meco = t1 + dt2
```

La expresión de `dt2` es equivalente a `(v2-v1)/a2` y evita restar velocidades próximas. Si el MECO es anticipado:

```text
t_meco = sqrt(2*h/a1)
v2 = a1*t_meco
dt2 = 0
```

En el ascenso balístico:

```text
a = -g
t_apogeo = t_meco + v2/g
h_max = h + v2²/(2*g)
v_apogeo = 0
```

Los estados se obtienen con las ecuaciones analíticas del tramo correspondiente, sin integración de Euler. El buscador consulta esa solución tanto al avanzar como al retroceder. En las fronteras se muestra el estado posterior al cambio; en el apogeo la velocidad es cero y la aceleración sigue siendo `-g`.

### Referencias nominales

| Magnitud | Valor aproximado |
|---|---:|
| Rapidez al desacople | 2.400,000000 m/s |
| Altitud al desacople | 144.000,000000 m |
| Tiempo de MECO | 126,554778 s |
| Rapidez en MECO | 2.481,934729 m/s |
| Altitud máxima | 474.285,714286 m |
| Tiempo de apogeo | 379,813424 s |
| Cociente `a1/g` | 2,040816 |

## Arquitectura

| Archivo | Responsabilidad |
|---|---|
| `main.c` | Ventana, controles, formulario, temporizador y coordinación |
| `fisica.h` / `fisica.c` | Tipos, validación, solución analítica, estados, reloj y eventos |
| `escena.h` / `escena.c` | Dibujo derivado del estado |
| `tests/pruebas.c` | Pruebas sin framework externo |
| `Makefile` | Construcción, ejecución, pruebas y limpieza |

`Parametros` reúne los cinco datos físicos; `Resultados` conserva las fronteras; `Estado` describe un instante; `Evento` representa una frontera; `Simulacion` agrupa la misión, sus opciones y un arreglo fijo de hasta cuatro eventos. Las funciones actualizan la simulación mediante punteros y consultan parámetros mediante punteros de solo lectura.

El arreglo almacena despegue, desacople si existe, MECO y apogeo si corresponde. No almacena cada cuadro animado. La escena consulta el mismo estado físico que la interfaz; el dibujo no modifica las ecuaciones.

## Pruebas y límites

`mingw32-make test` ejecuta las pruebas disponibles. Los casos relevantes incluyen referencias nominales, MECO anticipado, fase 2 nula, continuidad, final exacto, búsqueda temporal, modos y entradas inválidas. Las ejecuciones reales se documentan en el registro de verificación de la entrega; esta descripción de cobertura no afirma por sí sola un resultado satisfactorio.

La reproducción y la cámara lenta no convierten el reloj de Windows en un instrumento físico de precisión. El avance por cuadro se acota a 0,05 s para evitar saltos excesivos después de una demora.

La Tierra, las capas atmosféricas, el humo y los chorros son ilustraciones. No hay modelo de combustible, masa, arrastre, órbita, cargas estructurales ni descenso. Las capas y rótulos no agregan fuerzas al vuelo.

## Uso de inteligencia artificial y autoría

En el desarrollo de esta aplicación se utilizó **Codex** como asistencia para la implementación en C, la organización del motor físico, los controles gráficos, las pruebas y la documentación. Todos los integrantes deben revisar y comprender el código antes de la entrega.

**Registro por completar por el equipo:** consignar las consultas y prompts efectivamente utilizados, qué fragmentos se aceptaron o adaptaron, cómo se revisaron las soluciones y qué pruebas ejecutó cada integrante. No atribuir a personas cambios ni revisiones que todavía no hayan realizado.

La aplicación se entrega con su código fuente para permitir la compilación y la revisión técnica.

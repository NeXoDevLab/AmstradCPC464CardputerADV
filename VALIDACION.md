# Verificación de FIX12 y versiones anteriores

## Controles sin Fn y galería actual (FIX12, 8 de noviembre de 2026)

Al cargar un CDT, DSK o SNA se activa el modo de juego. Las teclas físicas
`;` / `.` / `,` / `/` envían arriba / abajo / izquierda / derecha a los
cursores CPC y al joystick 0. Espacio envía FIRE y mantiene el espacio del
teclado CPC. Fn+J alterna con el teclado de BASIC, con un aviso breve.
En el arranque de BASIC el modo de juego está desactivado, por lo que los
signos se escriben normalmente. Los controles previos con Fn se conservan.

Las pruebas ASan/UBSan comprueban cada dirección por separado, la matriz
exacta de cursores y joystick, FIRE, los modos con/sin Fn, los signos de
BASIC y los límites de volumen. Pasaron también las pruebas del escalador,
FDC, cinta, audio, RLE y navegación por carpetas. Compilaron las variantes
normal y `CPC_USB_RECORDING`.

En el Cardputer se verificó `controls=0` al arrancar y `controls=1` tras
elegir la cinta. Se cargó Fernando Martín completo y se capturaron de nuevo
BASIC, la cinta y el menú mediante USB RGB565. Las tres imágenes son de
240×135 y conservan todo el área visible, sin retocar sus píxeles. El
registro público de las capturas incluye la versión, el estado y sus hashes.

Después de las capturas, una ventana de 8,405 s en el menú de Basket Master
registró 50,09 fps de emulación, 21,42 fps LCD, cero intervalos de audio
mayores de 26 ms, máximo 21,66 ms y 19.708 bytes de heap libre.
El BIN normal ocupa 742.704 bytes. Se verificó su escritura por hash en
amstra (0x6f0000), conservando Launcher y el volumen inicial del 30%.

Se montaron los dos vídeos FIX11 sobre la fotografía del Cardputer ADV
facilitada por el usuario. La LCD ocupa exactamente 240×135 píxeles en
un lienzo de 880×580. Se verificó que todos los paquetes AAC son idénticos
a los de los archivos originales, incluyendo el último paquete. El README
muestra las tres capturas en una fila y las dos portadas debajo; la galería
HTML mantiene los dos reproductores en una fila y ofrece tamaño original
o ajuste a la ventana.

## Legibilidad de la pantalla CPC (FIX11, 8 de noviembre de 2026)

La reducción anterior de 320×200 a 240×135 elegía un único píxel de origen
por píxel de destino. Descartaba filas y columnas completas y cortaba los
trazos de las letras. FIX11 combina la cobertura de los píxeles de origen
con corrección de luminosidad y salida RGB565. Conserva el área completa
seleccionada por el CRTC, el borde y los colores planos del CPC.

La tarea de LCD recibe una copia inmutable de los 320×200 píxeles, compactada
sin pérdida de color a 40.001 bytes. El filtrado se ejecuta en el otro núcleo,
reutiliza filas iguales y permite preparar la siguiente imagen durante el
envío a la pantalla. No depende de PSRAM.

Las pruebas con ASan/UBSan comprueban los 64 códigos de color de entrada,
el empaquetado, la equivalencia entre la imagen original y la compactada,
los colores uniformes, los extremos del área visible y que ninguna fila
o columna con un trazo brillante desaparezca. Se comprobaron también la
codificación RLE de RGB565 y la compatibilidad del receptor con CPCREC1 y
CPCREC2. Compilaron tanto el firmware normal como el modo opcional de grabación.

En el Cardputer ADV se verificaron por USB BASIC y el arranque automático
de `Pac-Man_1986_T_and_T.dsk` hasta su menú. Las capturas RGB565 muestran
los trazos completos y el fondo azul correcto. La comparación de BASIC usa
el mismo framebuffer para ambos filtros; no compara momentos distintos.

En ventanas de 8,408 segundos, después del arranque o montaje y sin hacer
capturas durante la medición:

| Pantalla | Emulación | LCD | Intervalos de audio >26 ms | Mayor intervalo | Heap libre |
| --- | --- | --- | --- | --- | --- |
| BASIC | 50,07 fps | 24,50 fps | 0 | 21,69 ms | 24.452 bytes |
| Menú de Pac-Man DSK | 50,07 fps | 24,14 fps | 0 | 21,72 ms | 19.724 bytes |

El BIN normal ocupa 742.320 bytes. Se verificó su escritura por hash en la
partición amstra (0x6f0000), conservando Launcher, con volumen al 30%.
Las capturas USB detienen temporalmente la emulación y se excluyeron de
las mediciones de continuidad. La pantalla física sigue siendo de 240×135;
el filtro suaviza los bordes de las letras y no añade resolución física.

## Regrabación con texto FIX11 (8 de noviembre de 2026)

Se regrabó Fernando Martín con el mismo filtro RGB565 que utiliza la LCD.
La toma completa conserva el reinicio en BASIC, todos los bloques de cinta
y la transición real al menú de Basket Master, sin añadir una imagen fija.
La entrada al menú se envió por USB dentro de la grabación a los 283,326 s.
Duración: 291,273 s; 14.587 cuadros CPC y 7.294 imágenes recibidas, con
secuencias y desplazamientos PCM continuos. El MP4 completo se decodificó
sin errores y se revisó visualmente el menú final.

El tono de cinta entre 25 y 26 segundos tiene las mismas 1.693 transiciones
que la captura previa, con una diferencia máxima de dos muestras. En el MP4,
su RMS por intervalos de 20 ms está entre 10.608 y 10.855: no se insertan los
silencios que se oyen en el altavoz al esperar transferencias USB.
Las esperas de USB ralentizan y entrecortan el aparato durante este modo de
grabación; no forman parte del tiempo ni del audio del archivo.

Después se grabó `Ole Toro (S).cdt` desde el reinicio en BASIC hasta su menú
de teclado/joystick, que aparece automáticamente. Duración: 357,228 s;
17.890 cuadros CPC, 8.945 imágenes recibidas y 8.930 fotogramas MP4
decodificados sin errores, junto con todo el audio. Se revisó el menú final
en el vídeo codificado. No se aceleró ni recortó ninguna de las dos cintas.
Al terminar se reinstaló el BIN normal FIX11 de 742.320 bytes y se verificó
su escritura por hash en la partición amstra, conservando Launcher.

## Primer vídeo de Fernando Martín, FIX10 (8 de noviembre de 2026)

Se compiló y probó un modo opcional `CPC_USB_RECORDING`, desactivado en el
firmware normal. Transmite imágenes comprimidas de 240×135 y PCM mono de
16 kHz desde el propio Cardputer. Las pruebas de RLE verifican ida/vuelta,
límites de longitud y cancelación; el receptor comprueba la secuencia de
fotogramas y el desplazamiento de todas las muestras de audio.

Se capturó desde el reinicio del CPC en BASIC y el montaje de
`Fernando_Martin_Basket_Master.cdt`, sin modo rápido, hasta terminar la cinta.
El registro contiene 22.078 cuadros de emulación y 11.039 imágenes, sin
huecos de secuencia. Se verificó después la entrada al menú de Basket Master
por USB. El vídeo entregado conserva toda la carga, elimina únicamente la
espera posterior y termina con una captura fija de cinco segundos del menú.
Duración final: 285,16 segundos, H.264 a 1920×1080 y 25 fps, con sonido AAC.
Se decodificaron completos los 7.129 fotogramas del MP4 sin errores y se
revisaron imágenes de BASIC, carga y menú final.

La temporización usa el reloj emulado del CPC y sus muestras PCM, excluyendo
la espera de transferencia USB. No es una filmación externa del aparato.
Al terminar se reinstaló el BIN normal FIX10 de 736.528 bytes, con hash de
escritura verificado. El dispositivo volvió a BASIC con volumen 30%, memoria
libre 35.612 bytes y sin intervalos de audio mayores de 26 ms en la consulta
posterior al arranque.

## Arranque DSK y navegación por páginas (FIX10, 8 de noviembre de 2026)

La captura inicial mostraba BASIC tras CAT, con PAC-MAN en el catálogo y
sin arrancar. El DSK aportado contiene un ejecutable llamado `PAC-MAN`, sin
extensión, con cabecera AMSDOS válida y ejecución en 0x7f00. FIX9 solo
buscaba BAS/BIN y terminaba en CAT. FIX10 comprueba también las cabeceras
de otros archivos y escribe el nombre y la extensión exactos; un punto final
selecciona explícitamente un archivo sin extensión. Se conserva la prioridad
de los cargadores DISC, DISK, LOADER y MENU y se selecciona USER si es necesario.

Los ensayos sintéticos con ASan/UBSan cubren archivos sin extensión y con
extensiones no habituales, espacios interiores, atributos del directorio,
checksum inválido, binarios sin dirección de ejecución, archivos BASIC,
usuarios distintos de cero, prioridad del cargador y sectores entrelazados
en DSK estándar y Extended, con formatos Data y System.
En el host, el arranque automático de Pac-Man llegó a su menú y el de
Profanation 2 permitió entrar en partida; BASIC también pasó sus comprobaciones.
También se verificó que P seguido de FIRE inicia una partida de Pac-Man.
Se corrigió Fn+espacio: la biblioteca no rellena `keys.space` en la capa Fn,
por lo que el disparo lee ahora la tecla física.
Se corrigió el envío de teclas del ensayo del host para liberar Shift junto
con la letra, igual que la cola del dispositivo, antes del siguiente signo.

Arriba/abajo mueve una entrada y conserva la vuelta entre los extremos de
la lista. Izquierda/derecha cambia una página de seis entradas; en la primera
y última página se detiene. La posición visible se agrupa en páginas completas.
Se comprobaron páginas parciales, carpetas vacías y la fila de vuelta al padre.

En el Cardputer ADV se verificó por USB la secuencia de selección 0→1→0,
las páginas 0→6→12→6→0, los límites de página y la vuelta 0→26→0 en CDT.
Después se seleccionó `/CPC464/GAMES/DSK/Pac-Man_1986_T_and_T.dsk`:
arranque automático completado y modo 0. La
[captura LCD final de FIX10](docs/images/pacman-fix10-lcd.png) muestra el
laberinto y el mensaje READY al iniciar la partida. No se simularon
pulsaciones físicas; el ensayo USB usa el mismo movimiento del selector.

El BIN ocupa 736.528 bytes. Se verificó su escritura por hash en la partición
amstra (0x6f0000), conservando Launcher, con volumen al 30% y Pac-Man ejecutándose.

## Volumen inicial y tecla Fn (FIX9, 8 de noviembre de 2026)

El volumen inicial pasa a 77/255, que muestra 30%. Fn+= sube y Fn+- baja
en pasos del 5%, tanto en el selector como durante la emulación. La tecla
del signo + comparte posición con =; no hace falta Mayús para este atajo.
Sin Fn, los signos +, - y = se envían al CPC. Estos controles sustituyen
a los de FIX8; no se requiere Opt.

Las pruebas ASan/UBSan verifican los atajos con Fn, los signos sin Fn,
el volumen inicial, 30→35→30 y los límites de 0 y 100%.
En el Cardputer ADV se verificaron por USB el arranque FIX9 al 30%, esos
pasos y límites mediante el mismo ajuste que usa el teclado, y el control
dentro del selector. No se simularon pulsaciones físicas del teclado.
El binario de aplicación ocupa 735.920 bytes y su escritura se verificó
por hash en la partición amstra (0x6f0000), conservando Launcher.
El dispositivo quedó con el selector abierto y el volumen al 30%.

## Volumen directo (FIX8, 8 de noviembre de 2026)

Los signos + y - ajustan el volumen en pasos del 5%, con repetición al mantener
la tecla y límites de 0 a 100%. Se conservan Fn+= y Fn+-. Opt permite enviar
los signos al CPC; el signo = sin Fn conserva la asignación de BASIC. Los
modificadores de un atajo de volumen aislado no se envían a la matriz CPC.

El selector muestra el porcentaje, y la emulación muestra un indicador
temporal. Las pruebas con sanitizadores verifican atajos, paso 50→55→50,
límites inferior/superior y envío de signos mediante Opt.

En el Cardputer ADV se comprobaron por USB, usando el mismo ajuste que el
teclado, 50→55→50, saturación en 0 y 100%, retorno a 50%, ajuste dentro del
selector y lectura del volumen del altavoz al salir. La aplicación ocupa
735.904 bytes. La escritura se verificó por hash; se conservó Launcher y
se dejó el selector abierto con el volumen restaurado al 50%.

## Carpetas de juegos (FIX7, 8 de noviembre de 2026)

El selector muestra carpetas antes que archivos, permite entrar con Enter y
volver con Retroceso o `[ .. ] Volver`. Conserva los archivos de la raíz y
oculta los nombres que empiezan por punto. La navegación queda dentro de
`/CPC464/GAMES/` y recuerda la última carpeta visitada durante la sesión.

Se usa una sola lista de hasta 64 entradas, reservada en el heap y reutilizada
al cambiar de carpeta, liberando los nombres anteriores. Las pruebas con ASan
y UBSan cubren carpetas anidadas, extensiones con mayúsculas mezcladas, nombres
UTF-8, límite de entradas, lotes de más de 64 movimientos, colisiones,
idempotencia y conservación de los archivos ante errores.

En el Cardputer ADV se crearon `CDT/` y `DSK/`: 22 cintas se movieron a `CDT/`,
sin colisiones ni fallos; no había discos DSK para mover. Una segunda ejecución
de la organización no movió más archivos. Se verificaron entrada en CDT,
listado de sus 22 archivos, vuelta a GAMES, entrada en DSK vacía y vuelta.
También se montó Profanation desde `/CPC464/GAMES/CDT/` y comenzó la lectura.
Se escribió únicamente la partición del emulador y se conservó Launcher.

Los ajustes de vídeo y los marcos propios de los juegos conservan el
comportamiento aprobado de FIX6.

## Textos y área de juego (FIX6, 8 de noviembre de 2026)

La lista utiliza la fuente original ampliada de 8 a 10 píxeles, con seis filas.
El título amarillo de arranque «Amstrad CPC 464» pasa de 16 a 19 píxeles; el
título del selector conserva 16. Los textos de carga de 8 píxeles pasan a 10,
y «Montando imagen» pasa de 16 a 18. Se ajustan las posiciones para que los
textos no se solapen y se conserva la extensión de los nombres abreviados.

La composición LCD usa el área activa declarada por el CRTC. En Profanation
es 256×128, centrada horizontalmente dentro del framebuffer de 320×200:
ahora esa área se amplía a 240×135. El BASIC conserva su muestreo y la carga
original mantiene su composición con bordes. No se recortan regiones por
detectar píxeles negros: el negro puede formar parte de los gráficos del juego.

Las pruebas con AddressSanitizer y UndefinedBehaviorSanitizer comprueban
las cuatro esquinas visibles, ausencia de márgenes añadidos en el juego,
límites del framebuffer, geometrías vacías/excesivas y bordes de cinta.
También pasan las pruebas completas anteriores de BASIC, DSK, CDT, teclado,
paleta, motor, FDC y audio con los archivos externos de Profanation 2.

Se comprueba además el final de cinta: una pausa pendiente con el motor ya
detenido no mantiene el marco de carga cuando todos los bytes se han leído.
Las pausas intermedias y la reproducción de los últimos pulsos conservan el borde.

FIX6 ocupa 727.648 bytes y se instaló en la partición `amstra` del Cardputer
ADV, conservando Launcher. La escritura terminó con verificación del hash y
el arranque leyó OS/BASIC/AMSDOS correctamente. Volumen inicial: 50%.

En placa se volvió a cargar Profanation 2 desde CDT: 73.221/73.221 bytes sin
error y ejecución de su pantalla gráfica. Se capturó la composición LCD
de 240×135 ampliada durante la prueba en modo rápido:
[captura de FIX6](docs/images/profanation-fix6-lcd.png). Las pruebas de DSK
de esta revisión se ejecutaron en el host con los sanitizadores indicados.

## Arranque con particiones de M5Launcher (FIX5, 8 de noviembre de 2026)

El BIN FIX4 instalado por Launcher se verificó por digest: coincidía con el
archivo entregado. El diagnóstico leyó las tres ROMs correctamente; el fallo
posterior era `malloc(65536)`: devolvía nulo con 110.672 bytes libres en total,
pero un bloque máximo de solo 65.524 bytes. El mensaje anterior de lectura de
ROMs permanecía en pantalla y el error de memoria no tenía una ubicación propia.

FIX5 reserva los 64 KiB del CPC en memoria estática, lee las ROMs una sola vez
y vuelve a enlazar sus bancos después de `initz80`, sin acceder otra vez a SD.
El arranque muestra cada paso y los errores de SD/ROM tienen un mensaje visible.
Si falla la preparación, no se ejecuta el núcleo CPC.

Se actualizó únicamente la partición `amstra` del dispositivo probado
(`0x6f0000`, tamaño `0xc0000`), conservando su tabla, Launcher y los otros
firmwares. Esta dirección es específica de esa tabla; no es una dirección
genérica para instalar por USB. El BIN ocupa 726.112 bytes y cabe en ese espacio.

El arranque completó la lectura de OS/BASIC/AMSDOS y llegó a BASIC. Captura USB
de la composición LCD: [BASIC de FIX5](docs/images/basic-fix5-launcher.png),
con papel azul y texto amarillo. Registro en funcionamiento: 36.708 bytes
libres, volumen 50%, 1.171 paquetes de audio, ningún intervalo mayor de 26 ms
y máximo de 21.746 microsegundos antes de la captura USB.

Se repitió el arranque tres veces: siempre llegó a BASIC, con una única
lectura de cada ROM por arranque. El DSK de Profanation 2 se montó y ejecutó
su pantalla gráfica y menú. El CDT se montó sin error y comenzó la lectura
a velocidad original: avance de 1.131 a 6.894/73.221 bytes, motor activo,
bordes de carga y volumen 50%. Esta comprobación de FIX5 verifica el inicio de la carga CDT;
la prueba completa de ese archivo descrita más abajo corresponde a FIX4.

La compilación informa de 284.620 bytes de variables globales (86% del límite
que presenta Arduino). Los 64 KiB antes dinámicos se incluyen ahora en ese
total. Se mantiene PSRAM desactivada y se comprueba el funcionamiento en placa;
las pruebas sintéticas de teclado, paleta, vídeo, cinta, FDC y audio pasan.

## Continuidad de audio y volumen (FIX4, 6 de noviembre de 2026)

El refresco SPI de la LCD se ejecuta en otra tarea/núcleo, con una instantánea
inmutable de 32.400 bytes y exclusión mutua para los menús. La emulación publica
audio sin esperar a la pantalla. El altavoz I2S tiene prioridad superior y la
LCD se limita a 25 actualizaciones por segundo. Los cambios del borde ya no
recalculan la tabla de píxeles del área central.

Prueba real durante la carga CDT, con vídeo y sonido activados: 1.792 paquetes
de audio en 35,788 segundos (aproximadamente 50 por segundo), cero intervalos
mayores de 26 ms y máximo de 22.074 microsegundos. Registros de volumen AY:
0,0,0, por lo que en ese tramo se reproducía únicamente la señal de cinta.
Memoria libre estable: 34.984 bytes. El contador se reinició antes de medir;
las capturas USB de pantalla, que detienen la emulación durante su transmisión,
no se incluyeron en esta ventana.

El volumen inicial solicitado queda en 128/255 (50%). Las pruebas automáticas
de núcleo, DSK/CDT, borde y audio siguen pasando con los sanitizadores activados.
La ausencia subjetiva de ruido residual debe confirmarse al escuchar el altavoz.

## Pruebas automáticas

Ejecutadas con las ROM OS/BASIC/AMSDOS del usuario y sus imágenes
`Profanation 2-Espaคol.dsk` y `Profanation 2-Espaคol.cdt`.

- C++ compilado con AddressSanitizer y UndefinedBehaviorSanitizer.
- Matriz de letras, números y símbolos; correcciones específicas A, I, Y,
  Enter, comillas, barra vertical y barra invertida.
- Decodificación de píxeles CPC en modos 0/1/2 y colores RGB de tres niveles.
- El motor detenido no consume cinta; las longitudes TZX se convierten al
  reloj CPC de 4 MHz; pausas y metadatos se procesan sin perder bloques.
- Un READ ID tras buscar una pista fuera del disco devuelve error y no accede
  fuera de los índices.
- Audio: tono AY de 625 Hz medido sobre las muestras PCM y amplitud de cinta
  comprobada. Tres búferes protegen el audio en reproducción asíncrona.
- Arranque BASIC: papel azul, texto amarillo, imagen completa.
- Profanation DSK y CDT: carga, menú y entrada a partida mediante la tecla 3.
  Se comprueba geometría y dibujo para detectar borrados del framebuffer.
- Captura de cinta en reproducción: 28 códigos de borde distintos registrados
  desde el Gate Array durante la carga, sin generar una animación artificial.

El comando reproducible está en `tests/run_host.sh`. Los PPM se guardan en
un directorio temporal anunciado al finalizar.

## Pruebas con el Cardputer ADV conectado

ESP32-S3, flash de 8 MB, conectado por USB al ordenador de pruebas.
Se escribió únicamente la aplicación en `0x10000`;
esptool verificó el hash tras cada instalación.

- Capturado el framebuffer del BASIC por USB: azul marino, tinta amarilla,
  texto correctamente ordenado. La captura no es una fotografía del panel.
- DSK montado directamente desde su SD. Capturados el menú de Profanation y la
  partida después de pulsar 3 mediante la matriz CPC.
- CDT: lectura completa de los 73.221 bytes sin error, seguida de ejecución
  de la introducción gráfica. Registro al finalizar: `tape=0`,
  `tape_bytes=73221/73221`, sin mensaje de error.
- Antes de separar la LCD en FIX4, la memoria libre permaneció alrededor de 72 KB;
  con la instantánea adicional de FIX4 quedó alrededor de 35 KB,
  sin reservas grandes para almacenar imágenes completas ni reinicios.
- El hardware informa de cero bytes de PSRAM. Se compiló con PSRAM desactivada.
- FIX3 expone una captura adicional de 240×135 (`v`) usando la misma función
  de composición que la LCD, incluido el borde de carga.

Paquete M5Stack ESP32 3.3.10; M5Cardputer 1.1.1; M5Unified 0.2.28;
M5GFX 0.2.35. El FQBN y el procedimiento están documentados en README.

## Límites conocidos

Estas pruebas cubren los dos archivos aportados de Profanation 2 y los casos
indicados; no equivalen a compatibilidad universal con todo el catálogo CPC.
No se implementan ZIP, escritura/formateo de discos, SNA de 128 KB ni bloques
CSW/generalizados de cinta. La LCD y el altavoz reproducen una versión reducida
de la imagen y audio del CPC; no se ha medido la respuesta acústica frente a un CPC real.

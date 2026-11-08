# Guía completa · Amstrad CPC 464 en Cardputer ADV

Esta guía corresponde a **FIX12**, con carga desde microSD, modos de cinta
original y rápido, audio AY y volumen inicial del 30%.

## 1. Qué necesitas

- M5Stack **Cardputer ADV**. Esta versión se ha probado con este modelo.
- Cable USB-C de datos y un ordenador con macOS, Linux o Windows.
- microSD en FAT32, con una copia de seguridad de lo que ya tengas guardado.
- ROMs de CPC 464 de 16 KiB y tus propios juegos. No se distribuyen aquí.
- Arduino IDE 2 o Arduino CLI si vas a compilar.

La placa probada tiene 8 MB de flash e informa de 0 bytes de PSRAM. El proyecto
no necesita PSRAM: mantiene los 64 KiB del CPC en SRAM y lee discos y cintas
por bloques. Información del dispositivo: [documentación oficial de M5Stack](https://docs.m5stack.com/en/core/Cardputer-Adv).

## 2. Preparar la microSD

La raíz de la tarjeta debe contener esta estructura:

```text
CPC464/
├── OS.ROM            # sistema del CPC 464, exactamente 16384 bytes
├── BASIC.ROM         # BASIC del CPC 464, exactamente 16384 bytes
├── AMSDOS.ROM        # exactamente 16384 bytes; necesario para DSK
└── GAMES/
    ├── CDT/
    │   └── juego.cdt
    ├── DSK/
    │   └── juego.dsk
    └── instantanea.sna
```

Respeta los nombres y las mayúsculas de las carpetas y ROMs. Evita mezclar el
sistema de un CPC 6128 con el BASIC de un CPC 464. `AMSDOS.ROM` no es
`MSDOS.ROM`: se trata de la ROM del sistema de discos del CPC.

El selector permite entrar en subcarpetas con Enter. Para volver utiliza
Retroceso o selecciona `[ .. ] Volver` y pulsa Enter. La navegación se mantiene
dentro de `GAMES/`. También se siguen mostrando los juegos sueltos en esa raíz.
Las carpetas aparecen antes que los archivos; se muestran hasta 64 entradas
por carpeta y se ocultan los nombres que empiezan por punto.

Las ROMs y juegos quedan fuera de este repositorio y de sus binarios. Las
capturas de juegos sirven para documentar las pruebas; no contienen las imágenes
DSK/CDT que necesitarías para ejecutarlos.

macOS puede crear archivos `._nombre`. El selector los oculta automáticamente;
no es necesario borrarlos para utilizar el emulador. Expulsa la tarjeta desde el
sistema antes de desconectarla.

## 3. Instalar con Arduino IDE

1. Descarga el código y conserva el nombre de carpeta
   `AmstradCPC464CardputerADV`, igual que el archivo `.ino`.
2. Instala el paquete de placas **M5Stack** siguiendo su
   [guía de instalación](https://docs.m5stack.com/en/arduino/arduino_board).
3. Instala las bibliotecas M5Cardputer, M5Unified y M5GFX. Las versiones
   utilizadas se indican en la tabla de la sección siguiente.
4. Abre `AmstradCPC464CardputerADV.ino`.
5. Selecciona **M5Cardputer**, el puerto USB del ADV y estas opciones:

   | Opción | Valor |
   | --- | --- |
   | PSRAM | Disabled |
   | Flash Mode | QIO |
   | Flash Size | 8 MB |
   | Partition Scheme | Default 8 MB |
   | USB Mode | Hardware CDC and JTAG |
   | USB CDC On Boot | Enabled |

6. Compila y pulsa Subir. La instalación por Arduino escribe también los
   componentes de arranque necesarios para una placa recién preparada.
7. Inserta la microSD y reinicia. Debe aparecer BASIC con papel azul marino y
   texto amarillo.

![BASIC ejecutado en el Cardputer](images/basic.png)

Si no entra en modo de descarga, la guía oficial del ADV indica: interruptor
lateral en OFF, mantener **G0** pulsado al conectar el cable USB de datos y
soltarlo cuando el dispositivo se encienda.
[Procedimiento oficial](https://docs.m5stack.com/en/core/Cardputer-Adv#download-mode).

## 4. Compilar con Arduino CLI

Configuración usada para FIX12:

| Componente | Versión probada |
| --- | --- |
| M5Stack ESP32 | 3.3.10 |
| M5Cardputer | 1.1.1 |
| M5Unified | 0.2.28 |
| M5GFX | 0.2.35 |

Tras configurar el índice de placas M5Stack, instala las dependencias:

```sh
arduino-cli config add board_manager.additional_urls https://static-cdn.m5stack.com/resource/arduino/package_m5stack_index.json
arduino-cli core update-index
arduino-cli core install m5stack:esp32@3.3.10
arduino-cli lib install "M5Cardputer@1.1.1" "M5Unified@0.2.28" "M5GFX@0.2.35"
```

Desde la carpeta del proyecto:

```sh
sh scripts/build.sh
```

El script utiliza este identificador de placa:

```text
m5stack:esp32:m5stack_cardputer:PSRAM=disabled,FlashMode=qio,FlashSize=8M,PartitionScheme=default_8MB,USBMode=hwcdc,CDCOnBoot=cdc
```

Deja los resultados en `.build-verified/`. Si el ejecutable no está en PATH,
puedes indicar su ruta:

```sh
CPC_ARDUINO_CLI="/ruta/a/arduino-cli" sh scripts/build.sh
```

Para subirlo, cierra antes cualquier monitor serie:

```sh
arduino-cli board list
arduino-cli upload \
  --fqbn 'm5stack:esp32:m5stack_cardputer:PSRAM=disabled,FlashMode=qio,FlashSize=8M,PartitionScheme=default_8MB,USBMode=hwcdc,CDCOnBoot=cdc' \
  --input-dir .build-verified --port PUERTO .
```

Sustituye `PUERTO` por `/dev/cu.usbmodem…` en macOS, `/dev/ttyACM…` en Linux o
`COM…` en Windows. Comprueba que corresponde al Cardputer ADV si hay varias
placas conectadas.

## 5. Actualizar con el BIN

Usa `firmware/AmstradCPC464-CardputerADV-FIX12-app.bin`, que contiene **solo la
aplicación**, sin ROMs ni juegos.

### Con M5Launcher

1. Copia el BIN de aplicación FIX12 a la microSD.
2. Entra en el explorador de SD de Launcher, selecciona ese BIN e instálalo.
3. Abre el emulador desde Launcher. Las ROMs siguen en `/CPC464/`.

Launcher asigna la partición de destino. No utilices una dirección fija por
USB para actualizar una placa con Launcher: `0x10000` puede contener el propio
Launcher. No necesitas un BIN completo de 8 MB para este procedimiento.

### Por USB, instalación independiente sin Launcher

Con el cargador de arranque y la tabla `default_8MB` de las secciones 3 o 4,
la aplicación está en **0x10000**. El siguiente comando solo corresponde a
esa instalación independiente. En una placa con Launcher hay que consultar
primero su tabla y la partición real del emulador.

Con esptool 5 instalado y el puerto identificado:

```sh
python3 -m esptool --chip esp32s3 --port PUERTO --baud 460800 \
  write-flash --flash-mode keep --flash-freq keep --flash-size keep \
  0x10000 firmware/AmstradCPC464-CardputerADV-FIX12-app.bin
```

Este comando actualiza la aplicación y reinicia el dispositivo. No modifica los
archivos de la microSD. La escritura debe terminar con la verificación del hash.
La suma SHA-256 del BIN publicado está en `firmware/SHA256SUMS`.

## 6. Controles

| Tecla | Acción |
| --- | --- |
| Tab | Abrir selector de juegos |
| Botón A / G0, si está disponible | Acceso alternativo al selector |
| Arriba / abajo; también `;` / `.` | Entrada anterior / siguiente en el selector |
| Izquierda / derecha; también `,` / `/` | Página anterior / siguiente, seis entradas |
| Enter | Entrar en una carpeta o cargar el juego seleccionado |
| Retroceso / `[ .. ] Volver` | Volver a la carpeta anterior |
| Tecla de acento grave | Salir del selector |
| O, en el selector | Carga original: bordes y sonido de cinta |
| R, en el selector | Modo rápido, con audio silenciado |
| S, en el selector | Activar / desactivar el sonido de cinta |
| Fn+F | Alternar modo rápido durante la ejecución |
| Fn+B | Mostrar / ocultar el borde fuera de la carga |
| Fn+= / Fn+- | Subir / bajar volumen un 5%, con porcentaje en pantalla |
| + / - / = sin Fn | Escribir los signos dentro del CPC |
| Flechas impresas `;` / `.` / `,` / `/`, sin Fn | Arriba / abajo / izquierda / derecha en juegos; cursores CPC y joystick 0 |
| Espacio, sin Fn | Disparo del joystick 0 y espacio del teclado CPC |
| Fn+J | Alternar controles de juego y teclado de BASIC |

Las letras, números, símbolos, Enter, retroceso, Escape, Shift, Ctrl y las teclas
de función disponibles se traducen a la matriz CPC. La tecla A escribe A;
no abre el selector. Los controles dentro de cada juego dependen del propio
juego: puede ser necesario escoger teclado o joystick en su menú.

Desde FIX12, cargar un CDT, DSK o SNA activa automáticamente los controles de
juego. Usa las cuatro teclas con las flechas impresas, sin mantener Fn, y espacio
para disparar. Selecciona joystick en el menú del juego si lo requiere. Fn+J
alterna este modo con el teclado de BASIC; aparece brevemente «Juego: ON/OFF».
En el arranque de BASIC se pueden escribir los signos normalmente. Los
atajos antiguos con Fn siguen funcionando y el volumen conserva Fn+= y Fn+-.

## 7. Cargar DSK

1. Copia el DSK en `/CPC464/GAMES/DSK/` y comprueba que exista `AMSDOS.ROM`.
2. Abre el selector con Tab, entra en DSK con Enter, elige el disco y pulsa Enter.
3. El emulador monta la imagen, reinicia el CPC y escribe `|DISC`.
4. Busca un cargador en el directorio del disco. Conserva la prioridad de
   BAS/BIN y reconoce otros nombres mediante una cabecera AMSDOS válida:
   BASIC o binario con dirección de ejecución. Admite archivos sin extensión,
   como `PAC-MAN`, y conserva la extensión exacta de otros archivos.
   Da prioridad a nombres
   como `DISC`, `DISK`, `LOADER` o `MENU` y escribe el comando RUN correspondiente.
5. Si no encuentra un cargador reconocido, escribe `CAT` para mostrar el
   directorio. No significa que todos los discos puedan arrancarse automáticamente.

También puedes escribir desde BASIC, pulsando Enter al final de cada línea:

```text
|DISC
CAT
RUN"DISC
```

`DISC` es un ejemplo: usa el nombre del programa que aparezca en tu disco.
No basta con montar una imagen para que cualquier juego arranque. Los discos
están protegidos contra escritura; guardar partidas en el DSK no está implementado.

En la edición probada de Pac-Man, pulsa **P** para seleccionar un jugador y
después **espacio** para enviar FIRE e iniciar la partida en FIX12. FIX10 corrigió
la lectura de ese atajo del teclado.

![Profanation 2 desde DSK](images/profanation-partida.png)

## 8. Cargar CDT como una cinta original

1. Copia el CDT en `/CPC464/GAMES/CDT/`.
2. Abre el selector, entra en CDT y pulsa O para elegir el modo original.
3. Selecciona la cinta y pulsa Enter.
4. El emulador monta la cinta, reinicia el CPC y escribe `|TAPE`, `RUN"` y
   una pulsación de espacio para comenzar.
5. Espera. El motor, las pausas y los pulsos se ejecutan durante la emulación;
   una carga puede tardar varios minutos. Algunas pantallas centrales se ponen
   negras mientras entran los bloques.

Los cambios de color del borde proceden del Gate Array y la señal audible de
los pulsos del CDT. La imagen incluye el área central completa y un borde
durante la carga original. Su aspecto depende del cargador de cada juego:
no todas las cintas muestran las mismas franjas o tienen el mismo sonido.

![Turrican durante la carga, capturado del dispositivo](images/carga-turrican.png)

Para acelerar una carga usa R en el selector o Fn+F. Ese modo reduce el trabajo
de vídeo y silencia el audio; no sustituye la cinta por una instantánea. La
aceleración depende del coste de emulación del juego. Desactívalo para jugar
a velocidad normal y escuchar el sonido.

## 9. Vídeo y sonido

La pantalla física es de 240×135. FIX6 ajusta el área activa del juego a toda
la LCD, conservando sus cuatro extremos. Por ejemplo, los juegos que usan
256×128 aprovechan la pantalla sin los márgenes del framebuffer de 320×200.
El BASIC conserva su imagen completa. Durante la cinta se compone una vista
de 384×240 que incluye el borde. Fuera
de la carga, Fn+B permite mostrarlo para ver los cambios de `BORDER` desde BASIC.
El BASIC arranca con azul marino y amarillo. FIX11 utiliza un filtro de
cobertura con corrección de brillo para conservar los trazos finos que
antes se perdían al reducir de 320×200 a 240×135. Los bordes de las letras
quedan más suaves; se mantienen los colores planos y la imagen completa.
La pantalla física sigue siendo de 240×135: el filtro mejora la lectura
sin aumentar ni recortar los caracteres.

La lista muestra seis juegos con letras de 10 píxeles. El título amarillo del
arranque tiene 19 píxeles; el título del selector conserva su tamaño anterior.

El volumen inicial es 30%. `Fn` + `=` lo sube y `Fn` + `-` lo baja en pasos del 5%, con
límites de 0 a 100%; mantener pulsada la tecla repite el ajuste. En el teclado
del Cardputer, `=` comparte tecla con `+`: para subir el volumen basta Fn+=,
sin Mayús. Para escribir los signos en el CPC, pulsa sin Fn: Mayús+= para `+`,
`-` para restar y `=` para asignar en BASIC. El selector muestra el
porcentaje y, durante la emulación, aparece un indicador temporal.

El audio mezcla los
canales AY y, si está activada, la señal de cinta. S afecta a la cinta; el modo
rápido silencia la salida. Una pausa de cinta debe distinguirse de un bloqueo:
puede haber silencio o pantalla negra durante partes de la carga.

FIX4 mueve las transferencias de LCD a otra tarea y mantiene el audio prioritario.
Esto corrige los huecos periódicos que podían oírse como un zumbido añadido al
sonido de cinta. Las mediciones y sus límites están en [VALIDACION.md](../VALIDACION.md).

## 10. Capturar imágenes por USB

El script incluido utiliza Python estándar y las interfaces serie de
macOS/Linux. No necesita pyserial. Cierra otros monitores serie y ejecuta:

```sh
python3 tests/device_probe.py PUERTO '?'
python3 tests/device_probe.py PUERTO v carga.ppm
```

`v` obtiene la composición filtrada de 240×135, incluido el borde cuando esté
activo. Desde FIX11 utiliza RGB565; el script reconoce también las capturas
indexadas de versiones anteriores.
`s` obtiene el framebuffer CPC central de 320×200:

```sh
python3 tests/device_probe.py PUERTO s pantalla-cpc.ppm
```

En macOS puedes convertir una captura a PNG:

```sh
sips -s format png carga.ppm --out carga.png
```

Las capturas del repositorio se han obtenido así desde el dispositivo. Son
capturas digitales, no fotografías. Capturar interrumpe brevemente la emulación
mientras transmite los píxeles; no lo hagas si estás midiendo continuidad de audio.
Las capturas `v` y `s` requieren salir del selector. Dentro del selector,
`?` informa de la carpeta y selección, y `l` lista su contenido. Los comandos
USB `j`/`k` mueven una fila, `[`/`]` cambian de página y `e`, `b` y `q`
permiten probar entrada, vuelta y salida.

Otros comandos técnicos: `+`/`-` cambian el volumen; `m` abre el selector; `l` lista la carpeta actual;
`d` / `t` buscan un DSK/CDT de prueba en la raíz y en `DSK/` o `CDT/`
(con preferencia por Profanation si está presente); `3` pulsa esa tecla; `f`
alterna modo rápido y `c` reinicia los contadores de audio. No los uses durante
una partida que quieras conservar: los de montaje reinician el CPC.

El comando USB `o`, fuera del selector y antes de montar un juego, crea
`CDT/` y `DSK/` y mueve por formato los archivos sueltos de la raíz. No
sobrescribe archivos existentes: en una colisión deja el original en la raíz
y la informa como omitida. El resultado indica archivos movidos y fallos.

### Grabación digital de vídeo y sonido

El modo opcional de compilación `CPC_USB_RECORDING` permite grabar desde el
arranque del CPC en BASIC hasta la carga de una cinta. No está activado en el
BIN normal. Se compila con la propiedad adicional
`compiler.cpp.extra_flags=-DCPC_USB_RECORDING` y una carpeta de compilación
distinta para conservar el BIN normal.

`scripts/record-device.py` recibe fotogramas comprimidos y PCM mono de 16 kHz
del dispositivo y genera un MP4 con FFmpeg. El protocolo CPCREC2 de FIX11
conserva los colores RGB565 del filtro; el receptor también acepta CPCREC1.
La imagen es la composición de 240×135 del emulador, ampliada sin suavizado
a 1920×1080. No es una filmación del aparato ni incluye los menús de
M5Launcher o el selector. La grabación
comienza reiniciando el CPC, monta el CDT y utiliza la carga original con sonido.

```sh
python3 scripts/record-device.py PUERTO \
  /CPC464/GAMES/CDT/Fernando_Martin_Basket_Master.cdt \
  Fernando-Martin-carga-completa.mp4 --seconds 900 \
  --stop-file /tmp/cpc-detener-grabacion
```

Necesita FFmpeg en PATH o el paquete Python `imageio-ffmpeg`. El archivo de
salida debe ser nuevo. Para terminar antes del límite indicado, crea el archivo
especificado en `--stop-file`; el script finaliza y sincroniza el MP4.
La opción `--continue-file RUTA` envía una vez la tecla 3 cuando se crea ese
archivo. Permite salir de una presentación que pide cualquier tecla y grabar
la transición real al menú, sin detener la captura. Úsala después de terminar
la carga y con el firmware de grabación actualizado.
Mientras graba, deja avanzar la carga sin pulsar teclas. El envío USB puede
ralentizar el dispositivo y entrecortar su altavoz: el vídeo conserva el tiempo
del reloj CPC y todas las muestras PCM, sin incorporar esa espera de
transferencia. No se debe medir la continuidad del altavoz durante la captura.
Al terminar,
puedes reinstalar el BIN normal con el procedimiento de la sección 5.

## 11. Solución de problemas

| Síntoma | Qué comprobar |
| --- | --- |
| No aparece un puerto USB | Cable de datos, puerto correcto y modo G0 de descarga |
| SD no disponible | Tarjeta FAT32, inserción correcta y lectura desde el ordenador |
| FIX4 queda en «Leyendo ROMs de SD» o muestra «Sin memoria CPC» | Instalar FIX12: incluye la reserva fija de los 64 KiB del CPC introducida en FIX5 |
| Faltan ROMs | Rutas exactas y tamaño de 16384 bytes de OS/BASIC |
| Falta AMSDOS | Copiar `AMSDOS.ROM`; no renombrar una ROM de MS-DOS |
| Lista vacía | Carpeta actual, extensiones DSK/CDT/SNA y máximo 64 entradas por carpeta; `[ .. ] Volver` permite salir de una carpeta vacía |
| DSK vuelve a BASIC | Revisar `CAT`, nombre del cargador y compatibilidad del disco |
| CDT negro y con ruido | Puede estar cargando; consultar avance `tape_bytes` por USB |
| No hay sonido | Modo original, S activado y volumen; el modo rápido es silencioso |
| Sonido bajo | Fn+=; el volumen inicial es 30% |
| Se ven archivos duplicados en macOS | El selector oculta los `._`; el Finder puede mostrarlos igualmente |
| ZIP no aparece | Descomprimir en el ordenador y copiar DSK/CDT/SNA compatibles |
| TAP no aparece | Formato no implementado; utiliza una versión CDT de Amstrad CPC |
| SNA inválido | Solo se contemplan instantáneas clásicas de 64 KiB; no 128 KiB ni chunks comprimidos |
| Reinicios al habilitar PSRAM | Usar el perfil probado con PSRAM desactivada |

Al informar de un fallo indica FIX12, formato y mensaje, si estaba activo O/R,
y los datos de diagnóstico. Una captura ayuda a distinguir el cargador, BASIC
y el menú del emulador.

## 12. Pruebas y compatibilidad real

Las pruebas completas utilizan los dos archivos de Profanation 2 aportados
durante el desarrollo y las ROMs del usuario. No se incluyen en el repositorio.

```sh
sh tests/run_host.sh /ruta/a/roms /ruta/a/Profanation.dsk /ruta/a/Profanation.cdt
```

Ese ensayo es específico de la edición probada, cuyo cargador se llama DISC.
No es un comprobador universal para cualquier DSK/CDT. Ejecuta el mismo núcleo
con sanitizadores, llega al menú y entra en la partida. Las pruebas sintéticas
de CI no necesitan ROMs y comprueban teclado, colores, cinta, errores FDC y audio.

Se han comprobado DSK y CDT de Profanation 2; la captura de Turrican muestra una
carga en curso, no certifica que el juego completo haya pasado las mismas pruebas.
Faltan escritura de discos, ZIP, RAM de 128 KiB, bloques de cinta CSW/generalizados
y parte de las protecciones y efectos CRTC. Consulta también
[VALIDACION.md](../VALIDACION.md) y [los créditos](../THIRD_PARTY_NOTICES.md).

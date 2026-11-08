# Verificación de FIX5 y versiones anteriores

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

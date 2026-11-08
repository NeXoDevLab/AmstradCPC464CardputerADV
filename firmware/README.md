# Firmware FIX12

`AmstradCPC464-CardputerADV-FIX12-app.bin` es el binario de aplicación que se
instaló y verificó en el Cardputer ADV durante las pruebas. No contiene las ROMs
del CPC ni juegos: deben estar en la microSD.

- Modelo: M5Stack Cardputer ADV, ESP32-S3 y flash de 8 MB.
- PSRAM desactivada; compilado con el perfil `default_8MB`.
- Volumen inicial: 30%.
- M5Launcher: seleccionar el BIN desde SD e instalarlo; Launcher asigna la partición.
- Instalación independiente con `default_8MB`: aplicación en **0x10000**.
- Suma de integridad: [SHA256SUMS](SHA256SUMS).

Este archivo no sustituye al cargador de arranque ni a la tabla de particiones.
No escribas en `0x10000` sobre una placa con Launcher sin comprobar su tabla:
esa dirección puede contener Launcher.
Para una primera instalación utiliza Arduino IDE/CLI como explica
[la guía](../docs/GUIA.md#3-instalar-con-arduino-ide). Para una actualización
compatible utiliza [las instrucciones del BIN](../docs/GUIA.md#5-actualizar-con-el-bin).

FIX12 activa las flechas impresas y espacio para jugar sin mantener Fn tras
cargar desde SD. Fn+J alterna con el teclado de BASIC.

Conserva el filtro de FIX11 para la lectura de los textos, los trazos finos
y la imagen completa, con salida RGB565. Mantiene el arranque
DSK de ejecutables sin extensión o con extensiones no habituales mediante su
cabecera AMSDOS. Arriba/abajo mueve una entrada
e izquierda/derecha cambia de página de seis entradas. También se corrige
el envío del disparo del joystick, ahora también con espacio sin Fn.

Arranca al 30% y ajusta el volumen con Fn+= y Fn+- en pasos del 5%,
también en el selector. Sin Fn, los signos +, - y = se escriben en el CPC.
Conserva las carpetas CDT/DSK de FIX7, el vídeo y texto de FIX6 y la corrección
de memoria de FIX5. Para instalar, utiliza el BIN actual;
las versiones publicadas anteriores siguen disponibles en
[Releases](https://github.com/NeXoDevLab/AmstradCPC464CardputerADV/releases).

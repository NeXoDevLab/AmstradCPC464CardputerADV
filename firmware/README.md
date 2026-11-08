# Firmware FIX5

`AmstradCPC464-CardputerADV-FIX5-app.bin` es el binario de aplicación que se
instaló y verificó en el Cardputer ADV durante las pruebas. No contiene las ROMs
del CPC ni juegos: deben estar en la microSD.

- Modelo: M5Stack Cardputer ADV, ESP32-S3 y flash de 8 MB.
- PSRAM desactivada; compilado con el perfil `default_8MB`.
- Volumen inicial: 50%.
- M5Launcher: seleccionar el BIN desde SD e instalarlo; Launcher asigna la partición.
- Instalación independiente con `default_8MB`: aplicación en **0x10000**.
- Suma de integridad: [SHA256SUMS](SHA256SUMS).

Este archivo no sustituye al cargador de arranque ni a la tabla de particiones.
No escribas en `0x10000` sobre una placa con Launcher sin comprobar su tabla:
esa dirección puede contener Launcher.
Para una primera instalación utiliza Arduino IDE/CLI como explica
[la guía](../docs/GUIA.md#3-instalar-con-arduino-ide). Para una actualización
compatible utiliza [las instrucciones del BIN](../docs/GUIA.md#5-actualizar-con-el-bin).

FIX5 corrige el fallo de reserva de RAM que podía dejar FIX4 en «Leyendo ROMs
de SD». El BIN FIX4 se conserva como versión anterior.

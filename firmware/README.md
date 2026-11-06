# Firmware FIX4

`AmstradCPC464-CardputerADV-FIX4-app.bin` es el binario de aplicación que se
instaló y verificó en el Cardputer ADV durante las pruebas. No contiene las ROMs
del CPC ni juegos: deben estar en la microSD.

- Modelo: M5Stack Cardputer ADV, ESP32-S3 y flash de 8 MB.
- PSRAM desactivada; esquema de particiones `default_8MB`.
- Volumen inicial: 50%.
- Dirección para esta aplicación: **0x10000**.
- Suma de integridad: [SHA256SUMS](SHA256SUMS).

Este archivo no sustituye al cargador de arranque ni a la tabla de particiones.
Para una primera instalación utiliza Arduino IDE/CLI como explica
[la guía](../docs/GUIA.md#3-instalar-con-arduino-ide). Para una actualización
compatible utiliza [las instrucciones del BIN](../docs/GUIA.md#5-actualizar-con-el-bin).

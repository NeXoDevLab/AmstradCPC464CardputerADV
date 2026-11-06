# FIX4 · Amstrad CPC 464 para Cardputer ADV

Primera publicación de esta adaptación para M5Stack Cardputer ADV.

- Código C++/Arduino, guía completa en español y capturas digitales del dispositivo.
- DSK estándar/Extended por sectores y CDT por streaming desde microSD.
- Carga original con bordes del Gate Array y señal audible de cinta.
- Audio AY por muestras, volumen inicial del 50% y controles Fn+= / Fn+-.
- LCD en tarea separada y audio prioritario para evitar huecos periódicos.
- Pruebas sintéticas sin ROMs y ensayo de Profanation 2 con medios externos.

## Archivo de firmware

`AmstradCPC464-CardputerADV-FIX4-app.bin` es **solo la aplicación**, verificada
con M5Stack ESP32 3.3.10, M5Cardputer 1.1.1, M5Unified 0.2.28 y M5GFX 0.2.35.
Se escribe en **0x10000** sobre un arranque y particiones `default_8MB`
compatibles. No es un binario completo para escribir en 0x0.

Para una primera instalación utiliza Arduino IDE o compila y sube el proyecto
como explica [la guía](https://github.com/NeXoDevLab/AmstradCPC464CardputerADV/blob/main/docs/GUIA.md).
`SHA256SUMS` permite verificar la integridad del archivo adjunto.

## Medios y compatibilidad

No se incluyen ROMs ni juegos. Debes aportar las ROMs del CPC 464 en la microSD.
Profanation 2 se ha probado por DSK y CDT; la captura de Turrican documenta una
carga en curso y no certifica una prueba completa del juego.

No están implementados ZIP, escritura de discos, RAM de 128 KiB ni todos los
tipos de bloque/protección de cinta. Véanse los resultados en
[VALIDACION.md](https://github.com/NeXoDevLab/AmstradCPC464CardputerADV/blob/main/VALIDACION.md).

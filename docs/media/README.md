# Vídeos con la plantilla del Cardputer ADV

| Vídeo | Grabación | Duración | Final |
| --- | --- | --- | --- |
| `fernando-martin-cardputer-adv.mp4` | USB real, FIX11 | 4:51 | Transición real al menú de Basket Master |
| `ole-toro-cardputer-adv.mp4` | USB real, FIX11 | 5:57 | Menú de teclado/joystick mostrado automáticamente |

Las dos grabaciones mantienen toda la cinta, sus pausas y el sonido original
de la emulación. Se grabaron el 8 de noviembre de 2026. FIX12 conserva el
renderizador RGB565 de FIX11 y cambia los controles; las tres imágenes de la
fila superior de la galería se volvieron a capturar con FIX12.

La fotografía frontal facilitada del Cardputer ADV se guarda en
`../images/cardputer-adv-template.png`. Procede de la documentación de
[M5Stack](https://docs.m5stack.com/en/core/Cardputer-Adv). Se recortan sus
márgenes blancos para obtener un lienzo de 880×580; el vídeo de la LCD conserva
240×135 píxeles en la posición (231,54). Los `*-poster.png` son fotogramas de
estos montajes. Es una composición de una fotografía del equipo y su captura
digital, con las teclas y la carcasa inmóviles.

El audio AAC se copia completo, sin recodificar. Se compararon los hashes de
todos sus paquetes con los de las grabaciones originales: son idénticos.
Los JSON incluyen hashes del vídeo, del audio, de la grabación de origen y
de la fotografía, y el área exacta de la LCD. Los vídeos completos se
decodificaron para comprobar su integridad.

Para reconstruir los montajes, conserva los MP4 originales y sus JSON
generados por `scripts/record-device.py` y ejecuta desde la raíz:

```sh
python3 scripts/build-gallery-media.py Fernando-Martin.mp4 Ole-Toro.mp4
```

Se necesita FFmpeg en PATH o el paquete Python `imageio-ffmpeg`. Los archivos
de salida deben ser nuevos. El script conserva el audio y comprueba su hash.

[Galería y reproductores](https://nexodevlab.github.io/AmstradCPC464CardputerADV/).
Las ROMs y las cintas CDT no se distribuyen con estos vídeos.

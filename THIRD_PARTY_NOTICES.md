# Créditos y componentes

## Núcleo CPC

El Z80, Gate Array, PPI/PSG y la base del CRTC proceden de
[ESP32TinyCPC de rpsubc8/Ackerman](https://github.com/rpsubc8/ESP32TinyCPC),
un port para ESP32 del emulador de CPC de Tom Walker. El proyecto de origen
declara WTFPL. Se conserva el texto en [LICENSE](LICENSE).

Esta adaptación sustituye la salida VGA/PS2, añade el backend del Cardputer ADV,
el lector DSK por sectores, la cinta CDT por streaming, la composición LCD,
la mezcla de audio por muestras y las pruebas. El firmware actual no es el
binario de ESP32TinyCPC y no utiliza su interfaz VGA.

## Bibliotecas y plataforma

Se utilizan como dependencias de compilación, no se copian sus repositorios:

| Proyecto | Versión probada | Enlace |
| --- | --- | --- |
| M5Cardputer | 1.1.1 | https://github.com/m5stack/M5Cardputer |
| M5Unified | 0.2.28 | https://github.com/m5stack/M5Unified |
| M5GFX | 0.2.35 | https://github.com/m5stack/M5GFX |
| Arduino ESP32 / paquete M5Stack | 3.3.10 | https://github.com/m5stack/M5Stack |
| Arduino ESP32 upstream | 3.3.10 | https://github.com/espressif/arduino-esp32 |

M5Unified y M5GFX incluyen licencia MIT, copyright M5Stack. M5Cardputer identifica
sus fuentes principales con SPDX MIT. El framework Arduino ESP32 declara
LGPL-2.1-or-later y contiene componentes de terceros con sus propias licencias.
Las condiciones y avisos completos se encuentran en los proyectos anteriores;
los binarios se construyen enlazando esas dependencias.

Texto MIT de M5Unified/M5GFX, conservado para los componentes de M5Stack
enlazados en el firmware:

```text
MIT License

Copyright (c) 2021 M5Stack

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## ROMs, juegos y capturas

Las ROMs CPC y las imágenes de juegos no forman parte de esta publicación.
La licencia del código no concede derechos sobre ellas. Los nombres y las
pantallas de Amstrad, Profanation 2 y Turrican pertenecen a sus respectivos
titulares. Las capturas se incluyen para documentar el emulador y sus pruebas.

Las imágenes de `docs/images/` se capturaron del dispositivo o de su framebuffer;
su descripción y procedencia están en [la galería](docs/images/README.md).
No se ha utilizado la fotografía del equipo original como si fuera una captura
del Cardputer.

# MASHINA — Manual de usuario para FM1

MASHINA convierte el M-VAVE FM1 en un módulo de sonido de seis pistas para teclados, pads y controladores MIDI externos. Esta guía explica cómo configurarlo y tocar en vivo.

## 1. Controles y navegación

**HOME** vuelve a la pantalla principal; mantenelo presionado para abrir el menú de configuración. **GLO** abre el mezclador y los ajustes globales. **EDIT** edita el motor de sonido; **ENV**, **LFO** y **FX** controlan envolventes, modulación y efectos. **SAVE** administra sonidos y proyectos. **SEQ** abre el mapa MIDI de batería, **PLAY** sus ajustes de interpretación, y **REC** las macros MIDI. No hay secuenciador de canciones.

Girando **SELECT** se cambia de página; **ALGORITHM** selecciona una pista cuando no tiene una función específica. **K1–K4** modifican los valores visibles. **OCT−/OCT+** cambian la octava del teclado del FM1.

Para abrir el menú mantené **HOME**; sus páginas son **SCREEN**, **AUDIO**, **MIDI**, **USB**, **CONTROLS** y **SYSTEM**. En **AUDIO → TRACKS** elegís cuatro o seis pistas activas. **MIDI → LEARN** permite aprender controles; **LEARN CLEAR** borra asignaciones. **SYSTEM → ABOUT** presenta información del equipo.

## 2. MIDI externo, mezclador y capas

Conectá un controlador USB o TRS MIDI compatible. Abrí **GLO → MIX → MIXER**. Elegí la pista T1–T6 con **ALGORITHM**.

| Control | Función |
| --- | --- |
| **K1** | Canal MIDI de recepción RX: **1–16** |
| **K2** | Volumen de la pista |
| **K3** | Panorama izquierdo/derecho |
| **PRESETS** | Filtro DJ maestro solo en MIXER |

En **GLO → SYSTEM → ROUTE**, elegí **1-4** para las primeras cuatro pistas, **1-6** para las seis o **SEL** para tocar solo la pista seleccionada. La cantidad de pistas activas se configura en AUDIO.

Para superponer dos instrumentos, asigná T1 y T2 al **mismo canal RX**. Para controlarlos por separado, usá canales distintos. Guardá el proyecto para conservar la configuración.

## 3. Zonas de teclado y afinación

Abrí **GLO → MIX → ZONES**. Estos valores filtran las notas MIDI externas antes de la transposición:

| Perilla | Ajuste | Rango |
| --- | --- | --- |
| **K1 MIN** | Nota más baja | 0–127 |
| **K2 MAX** | Nota más alta | 0–127 |
| **K3 V.MIN** | Velocidad mínima | 1–127 |
| **K4 V.MAX** | Velocidad máxima | 1–127 |

Usá rangos de notas sin superposición para crear un **split**; rangos superpuestos para crear **capas**. Con el mismo rango de notas y distintas velocidades podés alternar sonidos suaves y fuertes. Las zonas corresponden al MIDI externo; el teclado integrado toca la pista seleccionada.

Abrí **GLO → MIX → TUNING**: **K1 TRN** transpone entre **−36 y +36 semitonos**; **K2 FINE** ajusta entre **−50 y +50 cents**. Los motores de batería conservan su mapeo de notas y no usan estos ajustes.

## 4. Motores de sonido y batería

Elegí una pista y un motor, luego abrí **EDIT**. Girá **SELECT** para recorrer sus páginas y modificá los controles con K1–K4.

**ATMOS** crea pads armónicos evolutivos; **BREATH**, **MOTION**, **FLOW** y **SPREAD** controlan movimiento y textura. **BRASS** crea vientos sintéticos: **TYPE 0** es saxofón, **64** un híbrido y **127** trompeta. Sus páginas EDIT ofrecen **PRESS, BUZZ, BREATH, BODY, FORMNT, GROWL, ATTACK, FAT, GRIND, METAL y CRUSH**. **INDUSTRIAL** genera bajos y leads agresivos con **WAVE, FAT, GRIND, METAL, CRUSH, FEED, CUT, RES, WAVE2, MIX, SUB y MODE**.

**DRUM-X** tiene doce sonidos independientes: BD 36, SD 38, CP 39, CH 42, OH 46, TM 45, RM 37, BL 56, B2 41, S2 40, O2 43 y T2 44. **DRUM-LAB** tiene doce pads sintetizados, inicialmente en MIDI **36–47**. Abrí EDIT para editar cada pad y sus niveles.

En una pista de batería, **SEQ** configura la nota MIDI (**K1, 0–127**), panorama (**K2**) y afinación del pad (**K3, ±24 semitonos**). **PLAY** configura curva de velocidad (**K1**), intensidad (**K2**), grupo de choke (**K3**) y superposición/reinicio (**K4**). Elegí el pad con SELECT y cambiá el banco con OCT−/OCT+.

Los ocho canales de voz de síntesis se **comparten entre todas las pistas**; BRASS tiene un máximo de cuatro voces por pista. Muchas capas y acordes pueden agotar las voces disponibles.

## 5. Macros MIDI y comandos CC

Abrí **REC**. Cada proyecto guarda hasta **32 macros**. Para crear una: presioná **OCT+**, enviá un CC desde el controlador, elegí la pista y el parámetro, girá su perilla y regresá a REC. Guardá el proyecto.

| Control | Acción |
| --- | --- |
| **SELECT** | Elegir macro |
| **ALGORITHM** | Activar/desactivar |
| **K1** | **RNG** rango continuo o **TGL** conmutador |
| **K2 / K3** | Extremos A/B, **0–127** |
| **K4** | Invertir el sentido |
| **OCT−** | Borrar; confirmar con OCT+ |

**Y** significa activa, **N** desactivada, **S** suspendida si falta el motor. Para cambiar número/canal CC o destino, borrá y recreá la macro.

**CC88/89/90** seleccionan temporalmente RX 1 (**valor 64–127**) o RX 2 (**0–63**) para T1/T2/T3. **CC85** aleatoriza suavemente, **CC86** más intensamente y **CC87** restaura el sonido melódico guardado. Para CC85–87 usá canales MIDI **13–15**, valor **64–127** para ejecutar y **0–63** para rearmar.

## 6. Efectos, proyectos y actuación

Abrí **GLO → MIX → EQ**. El ecualizador global, antes de efectos, tiene **LOW 50 Hz**, **MID 700 Hz** y **HIGH 10 kHz**, cada banda con **±15 dB**. No hay uno por pista. Usá FX para los efectos y envíos.

En **SAVE** guardá sonidos y proyectos. Un proyecto conserva pistas, canales MIDI, zonas, afinación, macros y efectos. **SAVE → LIBRARY → TOOLS** ofrece: **K1 INIT SOUND**, **K2 CLEAR SLOT** (borra permanentemente después de confirmar), **K3 NEW PROJECT** y **K4 CPU TEST**.

Para **Live Recall**, mantené presionado **REC**. Después tocá **FX, SEL, ENV o LFO** para cargar los proyectos **1, 2, 3 o 4**. Mantené REC otra vez para salir.

Para detener inmediatamente las notas y los efectos (**PANIC**), mantené **GLO** y presioná **OCT−**. No borra el proyecto.

Antes de instalar otra versión, guardá una copia completa de proyectos, presets y configuración desde el [editor MASHINA](../webapp/editor/). La [página del instalador](../webapp/installer/) es independiente.

## 7. Ejemplos para tocar en vivo

**Capas:** T1 y T2 con RX 1 y zonas superpuestas, sonidos distintos y panorama separado.

**Split:** T1 y T2 con RX 1 y rangos MIN/MAX de ZONES que no se superponen.

**Cambio por velocidad:** T1 1–80 y T2 81–127 con el mismo RX y el mismo rango de notas.

**Un pedal para varios parámetros:** creá varias macros REC con el mismo CC y canal y distintos destinos.

**Pads externos:** asigná DRUM-X o DRUM-LAB a una pista, definí RX en MIXER, notas en SEQ y respuesta en PLAY.

---

MASHINA v0.9 · M-VAVE FM1

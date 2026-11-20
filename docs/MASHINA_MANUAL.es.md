# MASHINA — Manual de usuario para FM1

MASHINA convierte el M-VAVE FM1 en un módulo de sonido de seis pistas para teclados, pads y controladores MIDI externos. Esta guía explica cómo configurarlo y tocar en vivo.

## 1. Controles y navegación

**HOME** vuelve a la pantalla principal; mantenelo presionado para abrir el menú de configuración. **GLO** abre el mezclador y los ajustes globales. **EDIT** edita el motor de sonido; **ENV**, **LFO** y **FX** controlan envolventes, modulación y efectos. **SAVE** administra sonidos y proyectos. **SEQ** abre el mapa MIDI de batería, **PLAY** sus ajustes de interpretación, y **REC** las macros MIDI. No hay secuenciador de canciones.

Girando **SELECT** se cambia de página; **ALGORITHM** selecciona una pista cuando no tiene una función específica. **K1–K4** modifican los valores visibles. **OCT−/OCT+** cambian la octava del teclado del FM1.

Para abrir el menú mantené **HOME**; sus páginas son **SCREEN**, **AUDIO**, **MIDI**, **USB**, **CONTROLS** y **SYSTEM**. En **AUDIO → TRACKS** elegís cuatro o seis pistas activas. **MIDI → LEARN** permite aprender controles; **LEARN CLEAR** borra asignaciones. **SYSTEM → ABOUT** presenta información del equipo.

### Referencia completa de controles

| Control | Función |
| --- | --- |
| HOME | Pantalla principal; mantenelo presionado para abrir el menú. |
| GLO | Mezclador, zonas MIDI, afinación, envíos, EQ y sistema. |
| EDIT | Edición del motor de sonido de la pista seleccionada. |
| ENV | Envolventes y destinos de modulación. |
| LFO | LFO, modulación y movimiento del sonido. |
| FX | Efectos por pista y buses de efectos globales. |
| SCL / ARP | Escalas, acordes y arpegiador. |
| SAVE | Sonidos, presets, proyectos y biblioteca. |
| SEQ | Mapa MIDI de notas, panorama y afinación de batería; no es un secuenciador de canciones. |
| PLAY | Curva de velocidad de batería, choke y retrigger. |
| REC | Hasta 32 macros MIDI CC por proyecto. |
| ALGORITHM | Selección de pista, salvo en páginas con otra función. |
| SELECT | Recorrer páginas; elegir el pad en páginas de batería. |
| K1–K4 | Cambiar los cuatro parámetros mostrados en pantalla. |
| PRESETS | Seleccionar sonidos, salvo cuando otra página lo utiliza de otro modo. |
| OCT− / OCT+ | Cambiar la octava del teclado del FM1; presionados juntos reinician el desplazamiento. |

Generalmente una pulsación abre la sección, SELECT recorre sus páginas y la perilla K correspondiente cambia el parámetro. Los cambios se aplican de inmediato, pero **guardá el proyecto** para conservarlos.

### Menú de configuración de HOME

Mantené HOME y girá SELECT:

| Página | Opciones |
| --- | --- |
| SCREEN | Osciloscopio, suspensión de pantalla, brillo LED, LEDs de escala |
| AUDIO | EQ de parlante, pistas activas 4/6, carga, BPM LOCK |
| MIDI | Salida MIDI, MIDI Learn, borrar controles aprendidos |
| USB | Audio USB y nivel USB |
| CONTROLS | Tiempo de pulsación larga, aceleración de perillas, FX latch, calibración |
| SYSTEM | Restaurar último estado, núcleos y frecuencia CPU, ABOUT |

HOME sale del menú. **LEARN CLEAR** elimina asignaciones MIDI: confirmá solamente cuando quieras borrarlas.

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

### MIXER: mandos y enrutamiento

| Control | Función |
| --- | --- |
| K1 | Canal MIDI RX de la pista, **1–16** |
| K2 | Volumen de la pista |
| K3 | Panorama |
| K4 | Sin asignar en MIXER |
| PRESETS | Filtro DJ maestro, solamente en MIXER |

Podés asignar los canales de T5/T6 aunque haya cuatro pistas activas. Para elegir cuatro o seis pistas: **HOME (mantener) → AUDIO → TRACKS**.

En **GLO → SYSTEM → ROUTE**, **1-4** usa las pistas 1–4 cuyos canales coincidan; **1-6** usa las pistas 1–6 según las activas; **SEL** dirige las notas solamente a la pista seleccionada. Por ejemplo, T1 y T2 con RX 1 y ROUTE 1-6 responden juntas a un teclado por el canal 1, cada una con su sonido y panorama.

### Los cuatro límites de ZONES

| Perilla | Valor | Rango |
| --- | --- | --- |
| K1 MIN | Nota MIDI más baja permitida | 0–127 |
| K2 MAX | Nota MIDI más alta permitida | 0–127 |
| K3 V.MIN | Velocidad mínima permitida | 1–127 |
| K4 V.MAX | Velocidad máxima permitida | 1–127 |

**Ambos extremos son inclusivos.** Con el mismo RX, T1 puede cubrir las notas bajas y T2 las altas (split), o cubrir notas superpuestas (capas). Si las dos usan el mismo rango de notas, pero T1 acepta velocidad **1–80** y T2 **81–127**, la fuerza de ejecución elige el timbre.

Las zonas se aplican al **MIDI externo**, no al mini-teclado del FM1, que toca la pista seleccionada. La zona se evalúa **antes de transponer**.

### TRN y FINE durante la interpretación

En **GLO → MIX → TUNING**, **K1 TRN** ajusta **−36 a +36 semitonos**; **K2 FINE**, **−50 a +50 cents**. **K3/K4** están reservados. TRN desplaza una capa; FINE puede engrosar dos sonidos al desafinarlos apenas.

Si cambiás TRN mientras suena una nota, **esa nota mantiene su altura original**; las siguientes usan el valor nuevo. Note Off y sustain siguen liberando las voces correctas. DRUM-X y DRUM-LAB no cambian ni su afinación ni su mapa de notas por TRN/FINE.

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

### ATMOS, BRASS e INDUSTRIAL: parámetros

Elegí pista y sonido con PRESETS, abrí **EDIT** y recorré las páginas girando SELECT. K1–K4 modifican sus controles; ENV, LFO, FX, SCL y ARP permiten otros ajustes.

**ATMOS** genera pads armónicos en evolución. **BREATH** mueve la amplitud; **MOTION**, la velocidad de ese movimiento; **FLOW**, el timbre; **SPREAD**, la separación de los osciladores. Empezá desde un preset de pad e incrementá los movimientos gradualmente.

**BRASS:** **TYPE 0** se aproxima al saxofón; alrededor de **64**, a un híbrido; **127**, a la trompeta.

| BRASS | K1 | K2 | K3 | K4 |
| --- | --- | --- | --- | --- |
| EDIT 1 | TYPE | PRESS | BUZZ | BREATH |
| EDIT 2 | BODY | FORMNT | GROWL | ATTACK |
| EDIT 3 | FAT | GRIND | METAL | CRUSH |

EDIT 3 introduce desafinación, distorsión y coloración digital. Dejá esos controles cerca de cero para un sonido de viento más limpio.

| INDUSTRIAL | K1 | K2 | K3 | K4 |
| --- | --- | --- | --- | --- |
| EDIT 1 | WAVE | FAT | GRIND | METAL |
| EDIT 2 | CRUSH | FEED | CUT | RES |
| EDIT 3 | WAVE2 | MIX | SUB | MODE |

WAVE/MIX mezclan osciladores; CUT/RES modelan el filtro; GRIND/METAL/CRUSH producen texturas más agresivas.

Las **ocho voces de síntesis son compartidas** por todas las pistas activas y motores de batería. BRASS tiene además un máximo de **cuatro voces por pista**. Si algunas notas se cortan, reducí capas simultáneas.

## 4. Motores de sonido y batería

Elegí una pista y un motor, luego abrí **EDIT**. Girá **SELECT** para recorrer sus páginas y modificá los controles con K1–K4.

**ATMOS** crea pads armónicos evolutivos; **BREATH**, **MOTION**, **FLOW** y **SPREAD** controlan movimiento y textura. **BRASS** crea vientos sintéticos: **TYPE 0** es saxofón, **64** un híbrido y **127** trompeta. Sus páginas EDIT ofrecen **PRESS, BUZZ, BREATH, BODY, FORMNT, GROWL, ATTACK, FAT, GRIND, METAL y CRUSH**. **INDUSTRIAL** genera bajos y leads agresivos con **WAVE, FAT, GRIND, METAL, CRUSH, FEED, CUT, RES, WAVE2, MIX, SUB y MODE**.

**DRUM-X** tiene doce sonidos independientes: BD 36, SD 38, CP 39, CH 42, OH 46, TM 45, RM 37, BL 56, B2 41, S2 40, O2 43 y T2 44. **DRUM-LAB** tiene doce pads sintetizados, inicialmente en MIDI **36–47**. Abrí EDIT para editar cada pad y sus niveles.

En una pista de batería, **SEQ** configura la nota MIDI (**K1, 0–127**), panorama (**K2**) y afinación del pad (**K3, ±24 semitonos**). **PLAY** configura curva de velocidad (**K1**), intensidad (**K2**), grupo de choke (**K3**) y superposición/reinicio (**K4**). Elegí el pad con SELECT y cambiá el banco con OCT−/OCT+.

Los ocho canales de voz de síntesis se **comparten entre todas las pistas**; BRASS tiene un máximo de cuatro voces por pista. Muchas capas y acordes pueden agotar las voces disponibles.

### DRUM-X: todas las notas

B2, S2, O2 y T2 son instancias independientes con sus propios ajustes, aunque emplean las mismas opciones de síntesis de las voces originales correspondientes.

| Pad | Nota MIDI | Pad | Nota MIDI |
| --- | --- | --- | --- |
| BD | 36 | RM | 37 |
| SD | 38 | BL | 56 |
| CP | 39 | B2 | 41 |
| CH | 42 | S2 | 40 |
| OH | 46 | O2 | 43 |
| TM | 45 | T2 | 44 |

En EDIT del DRUM-X, **LEVELS 1–3** ajusta los niveles individuales de los doce pads.

### DRUM-LAB: páginas de edición

| Página | Controles |
| --- | --- |
| STRIKE | PITCH, SWEEP, BODY y SNAP globales del kit |
| BODY | NOISE, TONE, DRIVE y WIRES globales |
| SOUND 1–4 | Tipo de pad y parámetros de síntesis individual |
| LEVELS 1–3 | Niveles individuales de los doce pads |

Los multiplicadores del kit escalan los valores individuales sin reemplazarlos. Al **100%** son neutros; valores menores reducen el aspecto correspondiente del sonido.

### SEQ: mapa MIDI de batería

En una pista de batería pulsá SEQ. ALGORITHM elige la pista, SELECT el pad y OCT−/OCT+ cambia de banco.

| Perilla | Función |
| --- | --- |
| K1 | Nota MIDI del pad, **0–127** |
| K2 | Panorama del pad |
| K3 | Transposición del pad, **−24 a +24 semitonos** |
| K4 | Sin uso |

### PLAY: respuesta de los pads

| Perilla | Función |
| --- | --- |
| K1 | Curva de velocidad **LIN / SOFT / HARD / FIX** |
| K2 | Respuesta de velocidad **0–127** |
| K3 | Grupo de choke: ninguno o **G1–G4** |
| K4 | **OVR** (superponer) / **RST** (reiniciar) |

Los pads del mismo grupo de choke pueden silenciarse entre sí. Elegí RST para que cada golpe reinicie su pad en lugar de superponerse al anterior.

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

### Crear y administrar macros REC

1. Abrí **REC**, pulsá **OCT+**.
2. Enviá un CC desde el controlador: quedan capturados el número y el canal.
3. Seleccioná la pista y el parámetro de destino.
4. Girá una vez la perilla K correspondiente.
5. Regresá a REC, ajustá valores y guardá el proyecto.

| Control REC | Acción |
| --- | --- |
| SELECT | Seleccionar macro, con cuatro filas por página |
| ALGORITHM | Activar/desactivar |
| K1 | **RNG** (rango continuo) / **TGL** (dos valores) |
| K2/K3 | Límites mínimo/máximo o A/B, **0–127** |
| K4 | Invertir dirección |
| OCT− | Solicitar borrado: **OCT+** confirma, OCT− cancela |

**Y** indica activa; **N**, desactivada; **S**, suspendida temporalmente porque el motor de destino no está seleccionado. La macro vuelve a funcionar cuando regresa el motor. **Número de CC, canal y destino no se editan**: borrá y creá nuevamente. Varias macros pueden compartir CC/canal y controlar parámetros distintos.

### MIDI Learn es independiente de REC

**MIDI Learn** es una función **global** con **32 destinos aprendidos adicionales**. Abrí **HOME (mantener) → MIDI → LEARN**, elegí un parámetro y mové el controlador. Sus asignaciones no son las macros REC guardadas en proyectos.

### Atajos CC dedicados

| CC | Acción |
| --- | --- |
| CC88 / CC89 / CC90 | Cambiar temporalmente RX de T1 / T2 / T3 |
| CC85 | Aleatorización suave de parámetros melódicos |
| CC86 | Aleatorización intensa |
| CC87 | Restaurar parámetros del último sonido guardado |

Para **CC88–90**, el valor **0–63 selecciona RX 2** y **64–127 RX 1**, desde cualquier canal MIDI entrante. No sobrescribe el canal guardado en el proyecto; cargar un proyecto restaura su RX.

Para **CC85–87**, utilizá **canal 13 para T1, 14 para T2, 15 para T3**. Un valor **64–127 ejecuta**, **0–63 rearma**. Solo afecta sonidos melódicos; no a los motores de batería. CC87 restaura parámetros sin cortar notas sostenidas si no cambió el motor.

## 6. Efectos, proyectos y actuación

Abrí **GLO → MIX → EQ**. El ecualizador global, antes de efectos, tiene **LOW 50 Hz**, **MID 700 Hz** y **HIGH 10 kHz**, cada banda con **±15 dB**. No hay uno por pista. Usá FX para los efectos y envíos.

En **SAVE** guardá sonidos y proyectos. Un proyecto conserva pistas, canales MIDI, zonas, afinación, macros y efectos. **SAVE → LIBRARY → TOOLS** ofrece: **K1 INIT SOUND**, **K2 CLEAR SLOT** (borra permanentemente después de confirmar), **K3 NEW PROJECT** y **K4 CPU TEST**.

Para **Live Recall**, mantené presionado **REC**. Después tocá **FX, SEL, ENV o LFO** para cargar los proyectos **1, 2, 3 o 4**. Mantené REC otra vez para salir.

Para detener inmediatamente las notas y los efectos (**PANIC**), mantené **GLO** y presioná **OCT−**. No borra el proyecto.

Antes de instalar otra versión, guardá una copia completa de proyectos, presets y configuración desde el [editor MASHINA](../webapp/editor/). La [página del instalador](../webapp/installer/) es independiente.

### EQ global: bandas exactas

| Banda | Frecuencia | Ganancia |
| --- | --- | --- |
| LOW | **50 Hz** | −15 a +15 dB |
| MID | **700 Hz** | −15 a +15 dB |
| HIGH | **10 kHz** | −15 a +15 dB |
| ON | Activar / omitir EQ | — |

Abrí **GLO → MIX → EQ**. Este EQ es **global y está antes de los efectos**, no es uno por pista. FX y los envíos permiten otros ajustes. Guardá el proyecto para retener el EQ.

### SAVE → LIBRARY → TOOLS

| Control | Acción |
| --- | --- |
| K1 | **INIT SOUND** — reiniciar el sonido |
| K2 | **CLEAR SLOT** — borrar definitivamente el slot seleccionado, previa confirmación |
| K3 | **NEW PROJECT** — crear proyecto |
| K4 | **CPU TEST** — prueba de CPU |

El proyecto incluye asignaciones MIDI, zonas, afinación, macros, sonidos seleccionados y efectos. Al cargar otro proyecto sin guardar podés perder cambios.

### Live Recall

Mantené pulsado **REC** para entrar. Luego tocá **FX / SEL / ENV / LFO** para cargar slots **1 / 2 / 3 / 4**. Mantené REC otra vez para salir. Las notas anteriores se detienen al cambiar de proyecto, aunque las colas de delay/reverb pueden terminar naturalmente.

### PANIC y respaldo

**Mantené GLO y pulsá OCT−** para detener todas las voces, los efectos, las notas retenidas y sustain. La pantalla indica PANIC MODE mientras sostenés GLO. **No borra el proyecto**.

Antes de cambiar o retroceder la versión, guardá una **copia completa de proyectos, presets de usuario y ajustes**. Exportar un solo preset **no equivale a un respaldo completo del dispositivo**. Usá el [editor MASHINA](https://mishamashinamm.github.io/mashina-fm1-fw/webapp/editor/) para copias y el [instalador MASHINA](https://mishamashinamm.github.io/mashina-fm1-fw/webapp/installer/) para instalar.

## 7. Ejemplos para tocar en vivo

**Capas:** T1 y T2 con RX 1 y zonas superpuestas, sonidos distintos y panorama separado.

**Split:** T1 y T2 con RX 1 y rangos MIN/MAX de ZONES que no se superponen.

**Cambio por velocidad:** T1 1–80 y T2 81–127 con el mismo RX y el mismo rango de notas.

**Un pedal para varios parámetros:** creá varias macros REC con el mismo CC y canal y distintos destinos.

**Pads externos:** asigná DRUM-X o DRUM-LAB a una pista, definí RX en MIXER, notas en SEQ y respuesta en PLAY.

---

MASHINA v0.9 · M-VAVE FM1

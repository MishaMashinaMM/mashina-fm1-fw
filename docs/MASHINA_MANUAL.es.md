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
| K3 VMN | Velocidad mínima permitida | 1–127 |
| K4 VMX | Velocidad máxima permitida | 1–127 |

**Ambos extremos son inclusivos.** Con el mismo RX, T1 puede cubrir las notas bajas y T2 las altas (split), o cubrir notas superpuestas (capas). Si las dos usan el mismo rango de notas, pero T1 acepta velocidad **1–80** y T2 **81–127**, la fuerza de ejecución elige el timbre.

Las zonas se aplican al **MIDI externo**, no al mini-teclado del FM1, que toca la pista seleccionada. La zona se evalúa **antes de transponer**.

### TRN y FINE durante la interpretación

En **GLO → MIX → TUNING**, **K1 TRN** ajusta **−36 a +36 semitonos**; **K2 FINE**, **−50 a +50 cents**. **K3 DLY** añade de 0 a 50 ms a cualquier pista, incluidos todos los motores melódicos. En batería se suma al ajuste K4 del pad; K4 sigue reservado en TUNING. Para sonidos melódicos, Note On y Note Off se desplazan por el mismo tiempo y conservan la duración original de la nota. La asignación MIDI del pedal de sustain no cambia. TRN desplaza una capa; FINE puede engrosar dos sonidos al desafinarlos apenas.

Si cambiás TRN mientras suena una nota, **esa nota mantiene su altura original**; las siguientes usan el valor nuevo. Note Off y sustain siguen liberando las voces correctas. DRUM-X y DRUM-LAB no cambian ni su afinación ni su mapa de notas por TRN/FINE.

## 3. Zonas de teclado y afinación

Abrí **GLO → MIX → ZONES**. Estos valores filtran las notas MIDI externas antes de la transposición:

| Perilla | Ajuste | Rango |
| --- | --- | --- |
| **K1 MIN** | Nota más baja | 0–127 |
| **K2 MAX** | Nota más alta | 0–127 |
| **K3 VMN** | Velocidad mínima | 1–127 |
| **K4 VMX** | Velocidad máxima | 1–127 |

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

### Sonidos de fábrica en v0.15

El banco de fábrica renovado contiene 20 sonidos ANALOG, 12 INDUSTRIAL, 5 BRASS y 8 ATMOS, además de bancos actualizados de PHASE, WHEEL, NOISE, FM6, VOICE y LOFI. Muchos presets llevan nombres de filósofos y pensadores de distintas épocas de la historia. DRUM-X: BUTTLER-1…9; DRUM-LAB: ZIZEK-1…3; DRUM-AC: KRIPKE-1…3; DRUM-IND: BADIOU-1…3.


## 4. Motores de sonido y batería

Elegí una pista y un motor, luego abrí **EDIT**. Girá **SELECT** para recorrer sus páginas y modificá los controles con K1–K4.

### Motores de sonido melódicos

Estos ocho motores siguen disponibles junto a los nuevos. Elegí un preset para escuchar su carácter y ajustá sus parámetros desde EDIT.

| Motor | Sonido y controles principales |
| --- | --- |
| **ANALOG** | Síntesis sustractiva para bajos cálidos, leads, pulsos y pads; osciladores, desafinación, filtro y drive. |
| **PHASE** | Distorsión de fase para metales sintéticos, campanas, teclas y bajos expresivos; DCW y ENV transforman la forma de onda. |
| **LOFI** | Sonidos de chip, tablas de ondas, bytebeat y texturas de videojuegos; CHIP, WAVE, DUTY, SWEEP y reducción digital. |
| **VOICE** | Vocales cantadas, coros, voces robóticas y frases; VOWEL, VOWEL2 y TALK transforman los formantes. |
| **WHEEL** | Órgano de ruedas tonales con registros, percusión, clic de tecla, saturación y efecto rotatorio. |
| **NOISE** | Viento, lluvia, chasquidos, ruidos metálicos y registros de desplazamiento; MODE, COLOR, FREQ y PEAK. |
| **FM6** | FM de seis operadores: pianos eléctricos, campanas, bajos y timbres digitales; algoritmos y envolventes por operador. |

**DRUM-X** también procede del motor de batería original, ampliado a doce sonidos; aparece junto a las otras baterías más abajo.

### Motores melódicos adicionales de MASHINA

**ATMOS — pads y texturas en movimiento.** Probá **KANT**, **BURKE** o **JEFFERSON**. **WAVE** selecciona la onda; **MORPH**, **DENSITY** y **DRIFT** transforman sus capas; **CUT/RES** controlan el filtro, y **TEXTURE/SHIMMER** agregan color. **BREATH** crea cambios de volumen, **MOTION** determina la velocidad, **FLOW** modifica el timbre en el tiempo y **SPREAD** separa los osciladores. Para acordes superpuestos, comenzá con movimientos sutiles.

**BRASS — saxofones y metales electrónicos.** No intenta reproducir perfectamente instrumentos acústicos: su sonido es deliberadamente sintético. **TYPE 0** da un carácter de saxo y **127** se acerca a la trompeta, con híbridos en medio. **PRESS** cambia la intensidad, **BUZZ** el borde áspero, **BREATH** el aire y **BODY/FORMNT** la resonancia. **GROWL** agrega rugosidad y **ATTACK** define el comienzo. En la tercera página, **FAT, GRIND, METAL, CRUSH** permiten sonidos exagerados. Probá **HUME**, **DIDEROT** o **HOLBACH**.

**INDUSTRIAL — bajos y leads melódicos agresivos.** A diferencia de DRUM-IND, sigue las notas tocadas. **WAVE** elige la onda principal; **FAT** la engrosa; **GRIND, METAL, CRUSH y FEED** generan distorsión y aspereza; **CUT/RES** moldean el filtro. La tercera página ofrece **WAVE2, MIX, SUB y MODE** para el segundo oscilador y graves adicionales. Empezá con **PASCAL**, **NEWTON**, **CAVENDISH** o **LOCKE**.

**DRUM-X** tiene doce sonidos independientes: BD 36, SD 38, CP 39, CH 42, OH 46, TM 45, RM 37, BL 47, B2 41, S2 40, O2 43 y T2 44. **DRUM-LAB** tiene doce pads sintetizados, inicialmente en MIDI **36–47**. Abrí EDIT para editar cada pad y sus niveles.

En una pista de batería, **SEQ** configura la nota MIDI (**K1, 0–127**), panorama (**K2**) y afinación del pad (**K3, ±24 semitonos**). **PLAY** configura curva de velocidad (**K1**), intensidad (**K2**), grupo de choke (**K3**) y superposición/reinicio (**K4**). Elegí el pad con SELECT y cambiá el banco con OCT−/OCT+.

Los ocho canales de voz de síntesis se **comparten entre todas las pistas**; BRASS tiene un máximo de cuatro voces por pista. Muchas capas y acordes pueden agotar las voces disponibles.

### DRUM-X: todas las notas

Los pads B2, S2, O2 y T2 tienen ajustes individuales y comparten los controles de síntesis de sus correspondientes sonidos de batería.

| Pad | Nota MIDI | Pad | Nota MIDI |
| --- | --- | --- | --- |
| BD | 36 | RM | 37 |
| SD | 38 | BL | 47 |
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

### DRUM-AC: kit de 12 pads de estilo acústico

**DRUM-AC** es el tercer motor de batería y sintetiza todos sus sonidos **sin muestras**. Combina golpes con ruido, resonancia de membrana en los toms y bandas metálicas amortiguadas para los platos. Ofrece los presets **KRIPKE-1**, **KRIPKE-2** y **KRIPKE-3**, sin modificar DRUM-X ni DRUM-LAB.

**La distribución cromática se repite en todas las octavas.** Los números MIDI corresponden al ejemplo **36–47**; la misma nota musical en cualquier octava reproduce el mismo pad:

| Tecla (MIDI) | Instrumento | Tecla (MIDI) | Instrumento |
| --- | --- | --- | --- |
| C (36) | Bombo | F♯ (42) | Hi-hat cerrado |
| C♯ (37) | Crash | G (43) | Tom grave |
| D (38) | Redoblante | G♯ (44) | Ride splash |
| D♯ (39) | Palmas (clap) | A (45) | Tom medio |
| E (40) | Campana de ride | A♯ (46) | Hi-hat abierto |
| F (41) | Tom de piso | B (47) | Tom agudo |

El hi-hat cerrado apaga la cola del abierto, incluso entre octavas, pero no interrumpe crash ni ride. 

**Páginas EDIT de DRUM-AC:**

| Páginas | Función |
| --- | --- |
| **Las dos primeras páginas globales** | Ocho controles independientes para todo el kit: **TONE, DAMP, BODY, ATTACK, BRIGHT, TEXTURE, RING, LEVEL**. Rango **0–200%**, **100% neutro**. No reescriben los parámetros individuales. |
| **SOUND 1–3** | Con **K1** elegís uno de los 12 pads; **K2–K4** editan sus ocho parámetros **0–127** repartidos en tres páginas: TUNE, DAMP, BODY, HIT, golpe específico, BRIGHT, carácter específico y RING/DECAY/TAIL. |
| **LEVELS 1–3** | Volumen independiente **0–127** de los doce pads. |

Al editar un pad, los nombres se adaptan al instrumento: **BEATER / TEXTURE** (bombo), **WIRES / TEXTURE** (redoblante), **SPLASH / METAL** (crash), **CHICK / DECAY** (hi-hats), **STICK / SHELL** (toms), **SNAP / TEXTURE / TAIL** (palmas), **STRIKE** (campana de ride) y **SPLASH** (ride splash). A diferencia de los ocho parámetros globales, los parámetros individuales no siempre tienen la misma función entre instrumentos.

**Afinación de toms:** cada tom se regula por separado. **TUNE = 0** elimina la resonancia afinada de la membrana y deja el golpe sin tono; **1–127** agrega una resonancia de altura progresivamente mayor sin alterar el cuerpo de ruido. **SPLASH** modifica el ataque ruidoso y el lavado metálico de crash y ride splash.

Los sonidos individuales, los volúmenes y los ocho controles globales se guardan en cada proyecto. DRUM-AC comparte las **ocho voces de síntesis** del firmware; no suma doce voces independientes. Los platos son aproximaciones sintetizadas, no grabaciones acústicas.


### DRUM-IND: percusión industrial sin afinación musical

**DRUM-IND** es el cuarto motor de 12 pads sintetizados, **sin muestras**, independiente del sintetizador melódico **INDUSTRIAL**. Utiliza ruido, filtros resonantes móviles, golpes irregulares y pulsos mecánicos en vez de osciladores afinados. Está pensado para superponerlo con otras baterías mediante el mismo canal MIDI.

**Distribución cromática fija, repetida en todas las octavas.** Los números MIDI 36–47 son un ejemplo.

| Tecla (MIDI) | Sonido / carácter | Tecla (MIDI) | Sonido / carácter |
| --- | --- | --- | --- |
| C (36) | **CLANG** — choque metálico | F♯ (42) | **BUZZ** — zumbido eléctrico |
| C♯ (37) | **ANVIL** — golpe de martillo | G (43) | **LIGHTNING** — descarga y crujidos |
| D (38) | **METAL** — resonancia de chapa | G♯ (44) | **ENGINE** — máquina irregular |
| D♯ (39) | **SCRAPE** — metal raspado | A (45) | **WHOOSH** — ráfaga de aire |
| E (40) | **WHISTLE** — silbido de aire | A♯ (46) | **MOTOR** — motor giratorio |
| F (41) | **SCREECH** — chirrido estridente | B (47) | **EXPLOSION** — explosión y cola turbulenta |

**Páginas EDIT:** las dos primeras ofrecen ocho controles independientes para todo el kit: **TONE, DAMP, BODY, ATTACK, BRIGHT, TEXTURE, RING, LEVEL** (cada uno **0–200%**, con **100% neutro**). ATTACK modifica aproximadamente los primeros 80 ms; BRIGHT modifica el balance de agudos después de limitar el sonido. En **SOUND 1–3**, **K1** selecciona el pad y **K2–K4** editan ocho parámetros individuales (**0–127**): **GRAIN, DECAY, WEIGHT, STRIKE, MOTION, AIR, CRUNCH, TAIL**. **LEVELS 1–3** permiten doce niveles independientes (**0–127**). Los controles pueden actuar de forma distinta en cada sonido.

Presets: **BADIOU-1**, **BADIOU-2**, **BADIOU-3**. Los sonidos, niveles y controles globales se guardan en el proyecto. DRUM-IND comparte las **ocho voces totales** del firmware. 


### SEQ: mapa MIDI de batería

En una pista de batería pulsá SEQ. ALGORITHM elige la pista, SELECT el pad y OCT−/OCT+ cambia de banco.

| Perilla | Función |
| --- | --- |
| K1 | Nota MIDI del pad, **0–127** |
| K2 | Panorama del pad |
| K3 | Transposición del pad, **−24 a +24 semitonos** |
| K4 | DLY: retraso del pad, 0–50 ms |

**DEFAULT / CUSTOM — los cuatro motores de batería.** En **DEFAULT**, DRUM-X, DRUM-LAB, DRUM-AC y DRUM-IND repiten los mismos 12 pads en todas las octavas: C en MIDI 36, 48, 60 y 72 dispara el mismo sonido sin transponerlo. 

Cambiar la nota de cualquier pad con **K1** activa **CUSTOM solamente en la pista seleccionada**. Los 12 pads responden entonces solo a sus notas MIDI asignadas (0–127). Los pads no modificados conservan sus notas originales de 36–47, pero ya no se repiten en otras octavas. Varios pads pueden compartir una nota y sonar juntos. La indicación **DEFAULT / CUSTOM** aparece arriba a la derecha.

Para volver a DEFAULT, mantené pulsado el **botón físico SEL** (no la perilla giratoria SELECT) unos **0,7 segundos**: aparece **RESET MIDI MAP?**. **OCT− cancela** y **OCT+ restaura solamente esa pista**, recuperando las octavas repetidas sin modificar sonidos, afinación, paneo ni otras pistas. Devolver las notas manualmente a sus valores originales no desactiva CUSTOM; usá RESET. Los mapas se guardan con el proyecto. En los cuatro motores, **K2 PAN** ajusta la posición estéreo del pad (L64–C–R64) y **K3 PITCH** modifica su afinación (−24 a +24 semitonos). En DRUM-AC y DRUM-IND, los sonidos no tonales cambian sus frecuencias resonantes y su carácter espectral, sin transformarse en notas melódicas.

**K4 DLY** añade de 0 a 50 ms al disparo del pad seleccionado. **GLO → MIX → TUNING → K3 DLY** añade otros 0 a 50 ms a toda la pista: ambos valores se suman (por ejemplo, 12 + 7 = 19 ms). El valor inicial es cero. Los ajustes se guardan con el proyecto y funcionan con los cuatro motores de batería. La variación aleatoria de tiempo queda para una versión posterior.


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

**CC88/89/90** seleccionan temporalmente RX 1 (**valor 64–127**) o RX 2 (**0–63**) para T1/T2/T3. **CC85** aleatoriza suavemente, **CC86** más intensamente y **CC87** recupera el sonido actual tal como estaba antes de aleatorizarlo. Para CC85–87 usá canales MIDI **13–15** (T1/T2/T3), valor **64–127** para ejecutar y **0–63** para rearmar. Funciona con el sonido actualmente cargado, sea un preset de fábrica, uno de usuario o un sonido de proyecto, sin necesidad de guardarlo antes. CC87 recupera sus parámetros originales sin cortar las notas. Cambiar de preset establece una referencia nueva. Compatible con todos los motores melódicos, incluidos ATMOS, BRASS, INDUSTRIAL y FM6; no se aplica a DRUM-X, DRUM-LAB, DRUM-AC ni DRUM-IND. Los selectores de onda y de patch quedan protegidos.

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

Para **Live Recall**, mantené presionado **REC**. Después tocá **FX, SEL, ENV o LFO** para cargar los proyectos **A, B, C o D**. Mantené REC otra vez para salir.

Para detener inmediatamente las notas y los efectos (**PANIC**), mantené **GLO** y presioná **OCT−**. No borra el proyecto.


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

Mantené pulsado **REC** para entrar. Luego tocá **FX / SEL / ENV / LFO** para cargar slots **A / B / C / D**. Mantené REC otra vez para salir. Las notas anteriores se detienen al cambiar de proyecto, aunque las colas de delay/reverb pueden terminar naturalmente.

### PANIC y respaldo

**Mantené GLO y pulsá OCT−** para detener todas las voces, los efectos, las notas retenidas y sustain. La pantalla indica PANIC MODE mientras sostenés GLO. **No borra el proyecto**.



## 7. Gestión de presets y biblioteca web

### 128 posiciones fijas de presets

**USER 001–032** (Program Change 0–31): sonidos editables que se guardan en FM1. **LIBRARY 033–128** (PC 32–127): 96 sonidos de fábrica reemplazables desde la biblioteca web; se pueden restablecer. Abrí **SAVE → PRESETS**; K4 elige BANK: ALL / USER / LIB, K1 y el encoder PRESETS recorren posiciones, K2 selecciona el motor; K3 marca o desmarca el favorito. Las posiciones USER vacías conservan su número. Guardá cualquier sonido editado primero en USER. El catálogo offline incluye 107 sonidos completos, también kits y voces FM6.

### Guardar un preset editado en FM1

Elegí una pista y editá el sonido. Abrí **SAVE → USER** (SOUND). K1 elige 001–032, K2 LOAD carga, K3 ERASE borra con confirmación y K4 SAVE guarda el sonido. Elegí la acción y pulsá OCT+; introducí el nombre y confirmá. Una posición ocupada pide confirmar la sustitución. Guardar asigna USER a la pista. **T1: ASSIGN PRESET** pide cargar un preset numerado; **T1: SAVE PRESET** pide guardar primero el sonido editado. El número identifica la pista.

### Ocho proyectos con secciones A–D

Cada proyecto tiene cuatro secciones, hasta seis referencias a presets numerados y ajustes independientes (MIDI, zonas, afinación, retardos, mezcla, efectos, macros). Los sonidos no se duplican en los proyectos: reemplazar un preset modifica todos los proyectos que lo referencian. Para conservar variantes, guardalas en posiciones USER distintas. No se guardan cambios de sonido sin guardar primero el preset. **REC (mantener) + FX / SEL / ENV / LFO** llama A / B / C / D del proyecto elegido. No se permite borrar USER si deja una referencia sin sonido.

### Biblioteca web y copias completas

[MASHINA Web Library](https://mishamashinamm.github.io/mashina-fm1-fw/web-library/) — Chrome / Edge, USB MIDI.

En BACKUP pulsá **Connect FM1**. El diálogo muestra la lectura y verificación automáticas. Esperá a que termine. No se descarga ningún archivo salvo que pulses **Save complete backup**. Después de escribir, los datos se actualizan automáticamente. Usá **Refresh device data** si modificaste el FM1 fuera del navegador.

Usá **Add factory sounds** o **Add device sounds** para añadir sonidos a Available presets. **Import JSON** combina colecciones y **Open collection** reemplaza la lista. Quitá con × o **Remove selected**. **Save collection** pide un nombre y guarda toda la lista restante. Una colección de sonidos sirve tanto en USER como en LIBRARY. Doble clic sobre un nombre permite editarlo directamente; Enter o clic fuera guarda, Escape cancela. Nombres de 1–12 caracteres ASCII imprimibles. Las ediciones del dispositivo se preparan hasta **Write USER changes**, **Write LIBRARY changes** o **Write PROJECT changes**; las offline son inmediatas. Cambiar el nombre del proyecto renombra las secciones A–D ocupadas.

En USER/LIBRARY importá JSON, buscá y filtrá fuentes o elegí sonidos de fábrica/del dispositivo. Arrastrá a una posición para reemplazarla o entre posiciones para mover/intercambiar. Ctrl-clic selecciona elementos, Shift-clic un rango, Ctrl+A todos los resultados del filtro (también otras páginas) o posiciones ocupadas, Escape limpia la selección. Arrastrá un grupo a posiciones consecutivas: si no entra entero, no se prepara ningún cambio. Los elementos desplazados van a las posiciones liberadas. Arrastrá elementos del dispositivo a la colección derecha y usá **Save collection** para guardar miles de sonidos o proyectos en JSON. Revisá la cola y referencias antes de Write; los sonidos activos se recargan al reemplazarlos.

En PROJECTS arrastrá proyectos A–D completos o una sección por su chip. Las referencias siguen siendo numéricas; instalá los sonidos necesarios. En BACKUP, **Save complete performance** guarda USER, LIBRARY, ocho proyectos, estado actual y ajustes, incluso cambios pendientes; se puede abrir y ordenar offline y restaurar después en FM1 con **Write complete backup**. Sólo Write escribe. Una colección de presets o proyectos no es una copia completa. Consultá la [guía](https://mishamashinamm.github.io/mashina-fm1-fw/web-library/guide.html). El puerto USB se llama **MASHINA**; volvé a seleccionarlo en el DAW si es necesario.

### Instalación inicial

Usá el [instalador](https://mishamashinamm.github.io/mashina-fm1-fw/webapp/installer/) y mantené el FM1 encendido hasta que arranque. Instalar v0.15 desde otro firmware o una versión anterior incompatible (incluida v0.14) borra automáticamente presets, proyectos y ajustes. Guardá lo que quieras conservar primero. Las actualizaciones compatibles conservan datos. No hace falta inicializar por separado.

## 8. Ejemplos para tocar en vivo

**Capas:** T1 y T2 con RX 1 y zonas superpuestas, sonidos distintos y panorama separado.

**Split:** T1 y T2 con RX 1 y rangos MIN/MAX de ZONES que no se superponen.

**Cambio por velocidad:** T1 1–80 y T2 81–127 con el mismo RX y el mismo rango de notas.

**Un pedal para varios parámetros:** creá varias macros REC con el mismo CC y canal y distintos destinos.

**Pads externos:** asigná DRUM-X o DRUM-LAB a una pista, definí RX en MIXER, notas en SEQ y respuesta en PLAY.

---

MASHINA v0.15 · M-VAVE FM1

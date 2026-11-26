/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* FELUCCA: one compilation unit (the HAL is header-only). Each file uses what the files above it
 * define, so the order matters. */

/* ---------------------------------------------------- build options --- */
/* build.py: FELUCCA_<NAME>=0 / 1 in the environment overrides these. Elsewhere: FELUCCA_FM4
 * (core.h: the DIGITAL engine), FELUCCA_ICONS (icons.c, the parameter icons; on by default),
 * FELUCCA_OTA_RAMONLY (ota.c). */
#ifndef FELUCCA_FLASH
#define FELUCCA_FLASH 1          /* flash driver + storage.c: settings, projects, user presets */
#endif
#ifndef FELUCCA_OTA
#define FELUCCA_OTA 1            /* M-UPGRADE update entry and the editor SysEx; needs FELUCCA_FLASH */
#endif
#ifndef FELUCCA_OTA_DRYRUN
#define FELUCCA_OTA_DRYRUN 0     /* 1 = stage, ask "success", then undo: no record, no reset */
#endif
/* Experimental 32 USER + 32 library: opt-in build, no physical flash writes
 * only after reclaiming retired project sectors has been verified. */
/* Optional diagnostic build. It retains the established FUN project format,
 * reports the candidate flash banks via read-only WebMIDI and forbids ALL
 * experimental library/project writes at compile time. */
#ifndef FELUCCA_FLASH_DIAG
#define FELUCCA_FLASH_DIAG 0
#endif
#ifndef FELUCCA_LIB32
#define FELUCCA_LIB32 0
#endif
#ifndef FELUCCA_LIB32_WRITE
#define FELUCCA_LIB32_WRITE 0
#endif
#if FELUCCA_LIB32_WRITE && (!FELUCCA_LIB32 || !FELUCCA_FLASH || !FELUCCA_OTA)
#error "Library flash writes require the experimental library, flash and editor"
#endif
/* Clean-break 64-preset project reference format. Unrelated to the existing
 * FUN* project snapshots unless explicitly enabled. Writing to its candidate
 * 128 KiB partition is independently locked by default; never ship these
 * experimental switches in a normal firmware build. */
#ifndef FELUCCA_PROJECT_REFS
#define FELUCCA_PROJECT_REFS 0
#endif
#ifndef FELUCCA_PROJECT_REFS_WRITE
#define FELUCCA_PROJECT_REFS_WRITE 0
#endif
#if FELUCCA_PROJECT_REFS && (!FELUCCA_LIB32 || !FELUCCA_FLASH)
#error "Project references require both the 64-preset library and flash support"
#endif
#if FELUCCA_LIB32_WRITE && !FELUCCA_PROJECT_REFS
#error "Reclaimed LIBRARY sectors overlap legacy FUN projects; enabling LIB1 writes requires MPR1 project routing"
#endif
#if FELUCCA_PROJECT_REFS_WRITE && (!FELUCCA_PROJECT_REFS || !FELUCCA_LIB32_WRITE)
#error "Project reference writes require explicit project and library write gates"
#endif
/* One-time reformatting is destructive to OLD song/project section sectors.
 * It is NEVER enabled merely by allowing normal project writes. */
#ifndef FELUCCA_PROJECT_REFS_FORMAT
#define FELUCCA_PROJECT_REFS_FORMAT 0
#endif
#if FELUCCA_PROJECT_REFS_FORMAT && (!FELUCCA_PROJECT_REFS_WRITE || !FELUCCA_FLASH)
#error "MPR1 first-install formatting requires separate explicit write permissions"
#endif
/* Separate, deliberately unpublished first-install operation. Merely enabling
 * experimental project/preset writes never authorizes erasing old FUN data.
 * This switch only compiles the code; no ordinary image exposes a trigger. */
#ifndef FELUCCA_CLEAN_INSTALL_LAYOUT
#define FELUCCA_CLEAN_INSTALL_LAYOUT 0
#endif
#if FELUCCA_CLEAN_INSTALL_LAYOUT && (!FELUCCA_PROJECT_REFS_FORMAT || !FELUCCA_PROJECT_REFS_WRITE || !FELUCCA_LIB32_WRITE || !FELUCCA_PROJECT_REFS || !FELUCCA_FLASH)
#error "Combined clean-install preflight requires all independent experimental storage gates"
#endif
/* Public v0.14 clean start is explicit, only on an uninitialized device.
 * The release builder enables this together with all storage gates. */
#ifndef FELUCCA_PUBLIC_LAYOUT
#define FELUCCA_PUBLIC_LAYOUT 0
#endif
#if FELUCCA_PUBLIC_LAYOUT && (!FELUCCA_CLEAN_INSTALL_LAYOUT || FELUCCA_FLASH_DIAG)
#error "Public layout requires the guarded first-install implementation"
#endif
/* Update-only images require an existing committed MPS1 boot marker. */
#ifndef FELUCCA_INITIALIZED_UPDATE
#define FELUCCA_INITIALIZED_UPDATE 0
#endif
#if FELUCCA_INITIALIZED_UPDATE && (!FELUCCA_PROJECT_REFS_WRITE || !FELUCCA_LIB32_WRITE || FELUCCA_PROJECT_REFS_FORMAT || FELUCCA_CLEAN_INSTALL_LAYOUT || FELUCCA_FLASH_DIAG)
#error "Initialized update requires writable MPR1/LIB1 and no formatter or diagnostic mode"
#endif
#if FELUCCA_FLASH_DIAG && (!FELUCCA_FLASH || !FELUCCA_LIB32 || FELUCCA_LIB32_WRITE || FELUCCA_PROJECT_REFS || FELUCCA_PROJECT_REFS_WRITE || FELUCCA_PROJECT_REFS_FORMAT || FELUCCA_CLEAN_INSTALL_LAYOUT)
#error "Read-only flash diagnostic requires LIB32 reads and prohibits all experimental project/preset writes"
#endif
#ifndef FELUCCA_LIB96
#define FELUCCA_LIB96 0
#endif
#if FELUCCA_LIB96 && ((!FELUCCA_INITIALIZED_UPDATE && !FELUCCA_PUBLIC_LAYOUT) || !FELUCCA_LIB32 || !FELUCCA_FLASH || !FELUCCA_OTA)
#error "LIB96 requires a guarded initialized-update or clean-install layout"
#endif
#ifndef FELUCCA_CDC
#define FELUCCA_CDC 1            /* USB CDC-ACM serial console */
#endif
#ifndef FELUCCA_UAC
#define FELUCCA_UAC 1            /* USB audio input: the master output, 16-bit stereo 44.1 kHz (usb.c) */
#endif
#ifndef FELUCCA_UAC_TONE
#define FELUCCA_UAC_TONE 0       /* bench: the USB input sends test triangles instead of the music */
#endif
#ifndef FELUCCA_UART
#define FELUCCA_UART 1           /* TRS MIDI IN on UART1 / PH8 */
#endif
#ifndef FELUCCA_DUAL_CORE
#define FELUCCA_DUAL_CORE 1      /* (JIANT 0.6.71) the second core renders some synth tracks (hal/fm1_multicore.h) */
#endif
#ifndef FELUCCA_SEQ_PROBE
#define FELUCCA_SEQ_PROBE 0     /* Stage 18A size-only probe; never flash this experimental build */
#endif
#ifndef FELUCCA_NO_SEQUENCER
#define FELUCCA_NO_SEQUENCER 1  /* JIANT LIVE DRUMS: live-only by default; 0 for legacy compatibility tests */
#endif
#if FELUCCA_NO_SEQUENCER && FELUCCA_SEQ_PROBE
#error "Select either FELUCCA_NO_SEQUENCER or FELUCCA_SEQ_PROBE, not both"
#endif
#ifndef FELUCCA_HOTRAM
#define FELUCCA_HOTRAM 1         /* (JIANT 0.6.78, experimental) core 0's hottest audio code in RAM (core.h) */
#endif
#ifndef FELUCCA_ID
#define FELUCCA_ID "FM-1_913"    /* package identity (build.py: the .fwsc marker string) */
#endif
#ifndef FELUCCA_VERSION
#define FELUCCA_VERSION "v0.14" /* JIANT's (based on Felucca 1.1.5.1): the splash, the menu, the console, the editor */
#endif
#if FELUCCA_OTA && !FELUCCA_FLASH
#error "FELUCCA_OTA needs FELUCCA_FLASH"
#endif

/* ------------------------------------------------------------- HAL --- */
#include <stdint.h>
#include "fm1_time.h"
#include "fm1_sys.h"
#include "fm1_irq.h"
#include "fm1_guard.h"
#if FELUCCA_DUAL_CORE
#include "fm1_multicore.h"
#endif
#include "fm1_input.h"
#include "fm1_timer.h"
#include "fm1_audio.h"
#include "fm1_adc.h"
#include "fm1_lcd_hw.h"
#include "felucca_tables.h"      /* generated by build.py (tools/gen_tables.py) */

/* ----------------------------------------------------- base, display --- */
#include "libc.c"
#include "lcd.c"
#include "gfx.c"

/* ----------------------------------------------------------- sound --- */
#include "core.h"
#include "engines.c"             /* dsp.c, the eng_*.c files, the factory patterns */
#include "params.c"
#include "mod.c"               /* the modulation matrix, used by voice.c and fx.c */
#include "voice.c"
#include "slicer.c"              /* the SLICER, one on a bus (JIANT 0.6.3), used by fx.c */
#include "fx.c"

/* ------------------------------------------------- MIDI, sequencer --- */
#include "usb.c"
#if FELUCCA_UART
#include "midi_uart.c"
#endif
#include "song_chain.c"
#include "seq.c"
#include "audio.c"

/* -------------------------------------------------------------- UI --- */
#include "panel.c"
#include "ui.c"
#include "menu_items.c"          /* MENU's rows and settings (ui_menu.c, editor_menu.c) */
#include "icons.c"               /* parameter icons (FELUCCA_ICONS), used by ui_draw.c */
#include "ui_graph.c"
#include "ui_draw.c"
#include "ui_menu.c"
#include "ui_input.c"
#include "ui_layer.c"              /* the quick layers (FX GLO SCL EDIT REC held; SEQ TOOLS: ui_tools.c) */

/* --------------------------------------------- storage, update, editor --- */
#if FELUCCA_FLASH
#if FELUCCA_DUAL_CORE
#define FL_PARK() fm1_mc_park()  /* (0.9.12) core 1 out of any job before the flash goes off (hal/fm1_multicore.h) */
#endif
#include "fm1_flash.h"
#include "fm1_clk320.h"          /* (0.9.12) MENU > SYSTEM > CPU CLOCK 320 MHZ's switch (curist's, MIT; main.c) */
#include "storage_hw.c"
#include "storage.c"
#endif
#include "upreset.c"             /* user presets (RAM mirror; flash with FELUCCA_FLASH) */
#if FELUCCA_LIB32
#include "preset_library_runtime.c" /* 32 additional, uploadable sound records; opt-in only */
#endif
#include "project.c"
#if FELUCCA_OTA
#include "ota.c"
#include "ota_hw.c"
#include "editor.c"              /* web editor SysEx (needs the OTA SysEx plumbing) */
#endif
#if FELUCCA_CDC
#include "console.c"
#endif
#include "master.c"              /* the MASTER pot (#137: read before the audio starts) */
#include "main.c"

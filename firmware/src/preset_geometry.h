/* SPDX-License-Identifier: GPL-3.0-only */
/* Shared geometry for the isolated 96-slot host integration.
 * Device expansion is compile-only until flash ownership and
 * upgrade/format gates have been audited. */
#ifndef MASHINA_PRESET_GEOMETRY_H
#define MASHINA_PRESET_GEOMETRY_H
#if defined(MASHINA_LIB96_HOST_TEST) && !defined(MASHINA_HOST_TEST)
#error "Expanded preset integration is host-only"
#endif
#if defined(MASHINA_EMU_LIB96) && MASHINA_EMU_LIB96 && !defined(FELUCCA_WEB)
#error "Expanded simulator geometry requires browser build"
#endif
#if defined(MASHINA_LIB96_NOR_TEST) || defined(MASHINA_LIB96_HOST_TEST) || (defined(MASHINA_EMU_LIB96) && MASHINA_EMU_LIB96) || (defined(FELUCCA_LIB96) && FELUCCA_LIB96)
#define MASHINA_LIBRARY_SLOTS 96u
#define MASHINA_LIBRARY_END 0xD8000u
#else
#define MASHINA_LIBRARY_SLOTS 32u
#define MASHINA_LIBRARY_END 0xC8000u
#endif
#define MASHINA_PRESET_SLOTS (32u + MASHINA_LIBRARY_SLOTS)
#endif

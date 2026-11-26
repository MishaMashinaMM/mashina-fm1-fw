/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Kerem Kilic (Ellic Studio) */
#pragma once
#define BLE_API static
#define BLE_DEVICE_NAME "MASHINA"
#define BLE_CENTRAL 1
#define BLE_MIDI_INITIAL_READ 1
#define FELUCCA_BLE_BOND 1
#define BLE_DIAG 0
#define BLE_DMA_MEMORY __attribute__((section(".pool"), aligned(4)))
#define FM1_RF_CAL_MEMORY __attribute__((section(".lowbuf"), aligned(4)))
/* Stock uses 27-octet PDUs. Keep that radio-tested limit until DLE is measured. */
#define BLE_LL_MAX_OCTETS 27u
/* Standard 23-byte ATT MTU, bounded rings for a single input controller. */
#define BLE_ATT_MTU_MAX 23u
#define BLE_LL_TX_RING 128u
#define BLE_SCAN_RING 8u
#define BLE_BARRIER() RING_PUBLISH()
#include "ble/ble.h"
#include "ble/ble_scan.h"
#include "ble/ble_store.h"
#include "ble/ble_vm.h"

static int mel_ble_ready(void);
static int mel_ble_room(void);
static void mel_ble_sync(uint32_t pkt, uint32_t pulse);
static int mel_ble_emit(uint32_t pkt);
static void mel_ble_drain(void);
static void mel_ble_poll(void);
static void mel_ble_boot(void);
static int mel_ble_set(int on);
static void mel_ble_disconnect(void);
static void mel_ble_forget(void);
static void mel_ble_scan(int on);
static void mel_ble_visible(int on);
static int mel_ble_connect(uint32_t row);
static int mel_ble_last(void);
static const char *mel_ble_status(void);
static uint32_t mel_ble_code(void);
static uint32_t mel_ble_signature(void);
static int mel_ble_flash_read(void *, uint32_t, uint8_t *, uint32_t);
static void mel_ble_radio_start(const struct ble_rf_trims *);

/* A tagged trailer after PER5 and its optional template; their offsets stay fixed.
 * Both settings copies and their existing CRC/commit protect keys and calibration. */
typedef struct {
    uint8_t addr[6], valid, flags;          /* flags: MB_VISIBLE, MB_RADIO (0 in older saves: CONNECT OUT, off) */
    uint8_t bond[28], rf[BLE_RF_COPY_SIZE], dev[BLE_DEV_STORE_SIZE];
    uint32_t magic;
} mel_ble_saved_t;
#define MEL_BLE_MAGIC 0x31454C42u /* BLE1 */
#define MB_VISIBLE 1u   /* Mode VISIBLE: advertise for a computer or phone, never scan */
#define MB_RADIO 2u     /* the radio as last set: started again at power-on */
static mel_ble_saved_t mb_saved __attribute__((section(".lowbuf"), aligned(4)));
static volatile uint32_t mb_revision;
static uint32_t mb_committed;

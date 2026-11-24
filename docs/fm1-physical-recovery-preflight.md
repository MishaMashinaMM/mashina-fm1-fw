# FM1 physical recovery preflight — READ ONLY / NOT A FLASHING AUTHORIZATION

This document is for the experimental `presets-library` 32+32 first-device trial. It does not authorize physical firmware installation, destructive formatting, bootloader writes, or a library overwrite. One FM1 must remain completely unchanged.

## Why the new hardware matters

The FM1 JieLi WL82/AC791N has a mask-ROM USB download mode called `WL82 UBOOT1.00`. Unlike an application-level WebMIDI installer, that mode can recover an application which no longer boots. Community-developed FM-1 Transporter uses the **Seeed XIAO RP2040** to force the mask-ROM USB_KEY signal even if the FM1 does not enumerate normally.

- Transporter and supported boards: https://github.com/kurogedelic/FM-1-transporter
- Published web app: https://kurogedelic.github.io/FM-1-transporter/web/
- Independent Windows/Linux USB-only recovery: https://github.com/Quixotic7/MvaveFM1Unbricker
- External recovery overview: https://sbegueria.github.io/fm1-dev-wiki/Recovery.html

**Board identity is critical:** Seeed XIAO RP2040, XIAO RP2040 Plus, XIAO nRF52840 Sense and nRF52840 Sense Plus are distinct products. Use the published XIAO RP2040 firmware and wiring ONLY after checking the exact PCB marking. The nRF52840 variant is not a drop-in replacement for this RP2040-specific Transporter build.

## Primary route: ordinary USB cable, NO XIAO needed

**First choice:** [M-Flasher](https://github.com/jvitkauskas/m-flasher) ([Windows download and guide](https://jvitkauskas.github.io/m-apps/flasher/)). Its documented **Back up the whole flash** operation is read-only and captures the full 1 MiB device image plus JSON metadata. The utility can put a normally running FM-1/Felucca into the WL82 ROM download mode over its ordinary USB-MIDI connection. MASHINA is a modified Felucca-family firmware, so successful identification must be confirmed on the physical unit; never assume the tool can reach every custom build.

For a Windows 10 owner with two FM1 units:

1. Pick ONE unit. Leave the second entirely unplugged and unchanged. Confirm the old 43-object backup and your nine-sound portable archive are independently retained.
2. Download M-Flasher from the developer's official page. Connect the chosen, charged FM1 via a normal USB **data** cable, without a hub. No RP2040, breakout cable, or special wiring is needed for this step.
3. In M-Flasher, identify the device and inspect its displayed flash chip/model. If it offers **DOWNLOAD MODE**, that command reboots the unit into ROM download mode; it is not a flash erase/write. Confirm that it is the intended FM1 before continuing. If the command fails or the device identification is unexpected, STOP.
4. Choose **Back up the whole flash**. This operation must ONLY save a dump; do not choose firmware installation, **Restore**, **Install m-bootloader**, **Flash bootloader**, or any auto-recommended bootloader change. If prompted to install m-bootloader, choose **No**. Do not proceed if software requests a write for the backup.
5. Save the first `1 MiB .bin` plus its metadata privately. Start a SECOND independent physical backup/read session on the SAME unit, saving under a different name. Avoid using a copy of the first file as the second.
6. Extract the non-flashable MASHINA trial review artifact and run `python tools/verify_fm1_recovery_dumps.py FIRST.bin SECOND.bin` on Windows (or `py` instead of `python`). The hashes and byte-for-byte equality must match. Keep both original dumps in at least two separate offline locations; do not attach them to public GitHub.
7. Exit download mode using M-Flasher's **Restart** or power-cycle the FM1. Verify that the original MASHINA firmware still boots and plays before proceeding to any installation planning.

This does not yet prove a full-flash restore will succeed. It does provide the required original-device data that can be used for recovery if a later destructive format fails. Importantly, **the XIAO RP2040 and shielded USB-C breakout remain UNUSED unless ordinary USB recovery itself is unavailable.**

Source: M-Flasher [README](https://github.com/jvitkauskas/m-flasher), [usage and download-mode guidance](https://github.com/jvitkauskas/m-flasher/blob/main/docs/usage.md) and [safety](https://github.com/jvitkauskas/m-flasher/blob/main/docs/safety.md).

## Hardware connection, ONLY after identity and cable pinout are verified

The documented **XIAO RP2040** Transporter mapping is:

| FM1 USB data signal | XIAO RP2040 pin |
| --- | --- |
| D+ | D6 / GP0 |
| D− | D7 / GP1 |
| Ground | GND |

**Never connect USB VBUS/5 V from the breakout to the FM1 or XIAO GPIO.** The FM1 uses its own battery when using Transporter. A five-core shielded USB-C breakout may include CC and/or shield separately from VBUS, D+, D− and ground. Do NOT assume wire function from insulation color or connector order. Identify each conductor with a continuity meter and the actual cable manufacturer's pinout before making connections; insulate and leave all unused conductors disconnected. Do not attach the breakout directly to a live computer USB socket while connected to XIAO GPIO.

A commercially documented USB data cable is the alternative for normal non-recovery FM1 use. Before attempting any electrical recovery, independently validate the hardware connection against the Transporter project's instructions.

## Recovery proof gate before any destructive 32+32 device trial

1. Preserve **two separate original 43-object USER/project archives**, verify all 43 CRCs, and keep the nine-sound portable pack. These files are NOT a byte-for-byte flash dump.
2. Record the exact XIAO board markings and cable conductor continuity; identify the FM1's flash ID and chip identity read-only. Do not assume both FM1 units have the same flash chip.
3. While one FM1 still boots, use **M-Flasher over ordinary USB first** to capture two independent full 1 MiB flash dumps from separate read sessions. Do not connect XIAO for this. Retain both privately; do not upload them to public GitHub. Read-only actions only.
4. Verify the two offline files via `python3 tools/verify_fm1_recovery_dumps.py first.bin second.bin`. It refuses truncation, identical input paths, blank boot/application areas, and any one-byte mismatch — including mismatches in settings/user sectors above 0x93000. Save the displayed hashes next to the private dumps. Do not publish the binaries.
5. Confirm that a known-good stock V15 recovery package and the exact-version tools are accessible, and independently check the package identity/hash against the tool's documentation. A successful dump is **not proof** that recovery writing will work.
6. Verify a safe recovery procedure with the owner, including recovery of the *actual original state* using that FM1's full dump, not just reinstalling stock V15. No physical recovery write is authorized by this checklist; that is a separate high-risk action.
7. Only after the above evidence and separate permission can we consider a flashable one-device clean-install trial. The firmware's existing non-flashable build gate remains mandatory until then.

## Recovery cautions

The mask-ROM recovery tools distinguish the boot head `0x0000–0x3FFF`, application `0x4000–0x92FFF`, and user/device data `0x93000–0xFFFFF`. A stock application restore only writes the application area and does **not** restore the old MASHINA USER data/projects. Never erase the whole chip, modify the boot head, or restore one device's full flash image onto a different unit. An ordinary `.fwsc` is not a raw image to write at address 0x4000.

**Important limitation:** the read-only double-dump verifier proves only that the two supplied files match and are plausible full-size captures. It does not authenticate their source, guarantee recoverability, or prove that any individual flash sector is owned by the experimental formatter.

This preparation does not require the user to dismantle, wire, or connect anything yet.

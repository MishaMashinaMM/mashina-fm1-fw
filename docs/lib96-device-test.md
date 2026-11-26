# MASHINA 32+96 — physical test only

Use ONE FM1 already running initialized stable v0.14 (32+32). Keep the second device untouched. Before flashing, use the official v0.14 Web Library BACKUP to capture and verify a complete JSON; keep it offline. Also export any USER and LIBRARY sound packs you care about. This update changes factory fallback identities, so prior references to unoverridden factory slots may play a different sound. Do not treat a 32-slot backup as a 96-slot full restore. No first-install formatting is enabled in this build.

Extract entire artifact, start `py -m http.server 8000` in the extracted directory, open http://localhost:8000/webapp/installer/ in Chrome/Edge. Flash only the initialized v0.14 test device. Keep USB connected; reboot as prompted.

Open http://localhost:8000/web-library/expand.html. Connect/inspect. If all expanded sectors are blank or already valid LIB1, no erasure is necessary. If verified retired legacy records exist, check the pre-upgrade backup box and explicitly type MASHINA96, confirm reclaim. Only C8000–D7FFF may be reclaimed, never old 32 LIBRARY slots, MPR1 projects, USER or settings. Unknown/corrupt sectors or failed operations: STOP, report exact results; do not retry automatically. Power-cycle, inspect again.

Open http://localhost:8000/web-library/ . Connect, capture and verify complete 96-slot backup. Check all 96 fallback/override names and 32 USER slots. Test PC 0,31,32,63,64,95,121,127; blank USER does not change sound. Create disposable USER and LIBRARY records, verify writes/readbacks at 065, 096, 128, then power-cycle and inspect. Test drums/FM6 with complete companion data; load and save an unused project section referencing high slot, reboot and recall; test resetting an unreferenced override to factory. Capture updated complete JSON and verify restore on the same geometry using disposable test data. Do not downgrade until high-slot project references and overrides have been exported; v0.14 cannot address them.

This package is not published to Pages; no merge/release until actual device verification. The original 107-preset catalog is unchanged.

# MASHINA firmware installation policy

- Different firmware -> MASHINA: clean installation.
- Compatible MASHINA versions, including reinstall: preserve user presets, library, projects and settings.
- Incompatible MASHINA storage change: clean installation.
- Existing data compatibility never gates whether a supported FM1 can receive the firmware. Backup/export is optional; Web Library is the means of retaining compatible data.
- Initialize automatically; no intermediate firmware or separate first-install page.
- Hardware identity, image integrity, flash bounds and verified writes remain required.
- Storage ABI tag and marker must change when storage or preset mapping changes incompatibly. Do not change either for compatible version bumps. First boot commits marker last.

/* SPDX-License-Identifier: GPL-3.0-only
 * Offline interpretation of diagnostic CRC32s. NEVER initiates MIDI or writes.
 * Repeating CRCs are evidence of matching checksums, not proof of a flash alias.
 */
export const ERASED_4K_CRC32 = "F154670A"; // CRC32 of 4096 bytes of 0xFF
const hex = x => "0x" + x.toString(16).toUpperCase();
export function analyzeFlashFingerprints(diag) {
  if (diag?.format !== "mashina-flash-diagnostic" || diag.version !== 2)
    throw Error("Expected a version-2 physical flash diagnostic JSON");
  const fp = diag.fingerprints;
  if (fp?.format !== "mashina-flash-sector-fingerprints" ||
      fp.version !== 1 || fp.sectorBytes !== 4096 ||
      fp.complete !== true || !Array.isArray(fp.sectors) || fp.sectors.length !== 40)
    throw Error("Diagnostic has no complete 40-sector CRC32 capture");
  const sectors = fp.sectors;
  for (let i = 0; i < 40; i++) {
    const s = sectors[i];
    const expected = i < 32 ? 0xA0000 + i * 4096 : 0xEA000 + (i - 32) * 4096;
    if (s?.index !== i || s.address !== expected || s.readable !== true ||
        !/^[0-9A-Fa-f]{8}$/.test(s.crc32))
      throw Error("Invalid or unreadable fingerprint at index " + i);
  }
  const library = sectors.slice(32).map(s => s.crc32.toUpperCase());
  const repeatedHalves = library.slice(0,4).every((value,i)=>value===library[i+4]);
  const erased = library.map((crc,i)=>crc===ERASED_4K_CRC32 ? hex(0xEA000+i*4096) : null).filter(Boolean);
  const duplicateGroups = [];
  const groups = new Map();
  library.forEach((crc,i)=> {
    if (!groups.has(crc)) groups.set(crc,[]);
    groups.get(crc).push(hex(0xEA000+i*4096));
  });
  for (const [crc,addresses] of groups)
    if (addresses.length>1) duplicateGroups.push({crc32:crc,addresses});
  return {
    format:"mashina-flash-fingerprint-analysis",version:1,
    mode:"OFFLINE_READ_ONLY",decision:"PHYSICAL_WRITES_BLOCKED",
    librarySectorCount:8,libraryRepeated16KiBPattern:repeatedHalves,
    libraryErasedSectorAddresses:erased,libraryDuplicateChecksums:duplicateGroups,
    messages:[
      repeatedHalves
        ? "LIBRARY CRC32 sequence repeats exactly after 16 KiB. Possible repeated data, address aliasing, or read-path issue; CRC32 alone cannot distinguish these."
        : "No exact 16 KiB CRC32 repetition detected in the LIBRARY capture.",
      erased.length
        ? erased.length+" LIBRARY sectors have the erased 4 KiB CRC32; this is not proof of ownership or even byte-for-byte erasure."
        : "No LIBRARY sector has the erased 4 KiB CRC32 (F154670A).",
      "Unknown sectors must not be erased or written, even for a clean-slate installation. Legacy project migration is optional only after a separately authorized replacement install.",
      "This report never authorizes flash writes or firmware installation."
    ]
  };
}

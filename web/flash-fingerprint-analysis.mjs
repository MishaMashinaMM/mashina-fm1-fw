/* SPDX-License-Identifier: GPL-3.0-only
 * Offline interpretation of diagnostic CRC32s. NEVER initiates MIDI or writes.
 * Repeating CRCs are evidence of matching checksums, not proof of a flash alias.
 */
export const ERASED_4K_CRC32 = "F154670A"; // CRC32 of 4096 bytes of 0xFF
/* Zero-length, CRC-committed FELU storage record in an otherwise erased
 * 4 KiB sector. Header CRC makes the full-sector CRC invariant to the
 * object ID, slot and sequence. Never infer ownership from CRC alone. */
export const EMPTY_FELU_RECORD_CRC32 = "127B0F88";
const hex = x => "0x" + x.toString(16).toUpperCase();
export function analyzeFlashFingerprints(diag) {
  if (diag?.format !== "mashina-flash-diagnostic" || diag.version !== 2)
    throw Error("Expected a version-2 physical flash diagnostic JSON");
  const fp = diag.fingerprints;
  const count=fp?.version===1?40:fp?.version===2?46:fp?.version===3?53:0;
  if (fp?.format !== "mashina-flash-sector-fingerprints" ||
      !count || fp.sectorBytes !== 4096 ||
      fp.complete !== true || !Array.isArray(fp.sectors) || fp.sectors.length !== count)
    throw Error("Diagnostic has no complete compatible 40/46/53-sector CRC32 capture");
  const sectors = fp.sectors;
  // Both physical diagnostic generations remain valid offline: earlier probes
  // inspected provisional USR (EA000), newer probes inspect reclaimed FUN (C0000).
  const libraryBase = sectors[32]?.address === 0xC0000 ? 0xC0000 :
                      sectors[32]?.address === 0xEA000 ? 0xEA000 : null;
  if (libraryBase === null || (count>=46 && libraryBase!==0xC0000))
    throw Error("Unknown LIBRARY fingerprint address map");
  const extra=[0xDC000,0xDD000,0xDE000,0xDF000,0x9F000,0xFE000,
               0xFC000,0xFD000,0xD8000,0xD9000,0xE5000,0xE6000,0xE7000];
  for (let i = 0; i < count; i++) {
    const s = sectors[i];
    const expected=i<32 ? 0xA0000+i*4096 :
                   i<40 ? libraryBase+(i-32)*4096 : extra[i-40];
    if (s?.index !== i || s.address !== expected || s.readable !== true ||
        !/^[0-9A-Fa-f]{8}$/.test(s.crc32))
      throw Error("Invalid or unreadable fingerprint at index " + i);
  }
  const library = sectors.slice(32,40).map(s => s.crc32.toUpperCase());
  const userAndCompanion = count>=46?sectors.slice(40,46):[];
  const auxiliary = count===53?sectors.slice(46,53):[];
  const repeatedHalves = library.slice(0,4).every((value,i)=>value===library[i+4]);
  const erased = library.map((crc,i)=>crc===ERASED_4K_CRC32 ? hex(libraryBase+i*4096) : null).filter(Boolean);
  const duplicateGroups = [];
  const emptyFeluPattern = sectors.filter(s => s.crc32.toUpperCase()===EMPTY_FELU_RECORD_CRC32)
    .map(s => hex(s.address));
  const groups = new Map();
  library.forEach((crc,i)=> {
    if (!groups.has(crc)) groups.set(crc,[]);
    groups.get(crc).push(hex(libraryBase+i*4096));
  });
  for (const [crc,addresses] of groups)
    if (addresses.length>1) duplicateGroups.push({crc32:crc,addresses});
  return {
    format:"mashina-flash-fingerprint-analysis",version:1,
    mode:"OFFLINE_READ_ONLY",decision:"PHYSICAL_WRITES_BLOCKED",
    librarySectorCount:8,libraryBaseAddress:hex(libraryBase),libraryRepeated16KiBPattern:repeatedHalves,
    libraryErasedSectorAddresses:erased,libraryDuplicateChecksums:duplicateGroups,
    emptyFeluPatternSectorAddresses:emptyFeluPattern,
    userCompanionSectorCount:userAndCompanion.length,
    userCompanionErasedSectorAddresses:userAndCompanion.filter(s=>s.crc32.toUpperCase()===ERASED_4K_CRC32).map(s=>hex(s.address)),
    auxiliarySectorCount:auxiliary.length,
    auxiliaryErasedSectorAddresses:auxiliary.filter(s=>s.crc32.toUpperCase()===ERASED_4K_CRC32).map(s=>hex(s.address)),
    messages:[
      repeatedHalves
        ? "LIBRARY CRC32 sequence repeats exactly after 16 KiB. Possible repeated data, address aliasing, or read-path issue; CRC32 alone cannot distinguish these."
        : "No exact 16 KiB CRC32 repetition detected in the LIBRARY capture.",
      erased.length
        ? erased.length+" LIBRARY sectors have the erased 4 KiB CRC32; this is not proof of ownership or even byte-for-byte erasure."
        : "No LIBRARY sector has the erased 4 KiB CRC32 (F154670A).",
      emptyFeluPattern.length
        ? emptyFeluPattern.length+" sectors match CRC32 127B0F88, the exact full-sector CRC of a valid zero-length FELU record. This CRC is independent of the record type, slot and sequence because the commit header contains its own CRC. Repetition alone therefore does NOT indicate flash aliasing. Validate each header and bytes before any erasure."
        : "No sectors have the known zero-length committed FELU record CRC32.",
      count>=46
        ? "Four USER and two companion sectors were also fingerprinted. CRC32 is evidence for later comparison, not proof of valid USER data, safe erasure or recovery."
        : "This historic 40-sector diagnostic did not inspect USER/companion sectors.",
      count===53
        ? "Seven settings, song index, autosave and voice-bank sectors were fingerprinted. Their contents/ownership and recovery are not established by CRC32."
        : "This historic capture did not cover all seven additional active-state sectors.",
      "Unknown sectors must not be erased or written, even for a clean-slate installation. Legacy project migration is optional only after a separately authorized replacement install.",
      "This report never authorizes flash writes or firmware installation."
    ]
  };
}

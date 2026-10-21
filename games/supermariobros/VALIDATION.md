# Validation - 21 October 2026

Optimized complete-campaign build, submitted as a standalone big game.
The publication changes only this game, its documentation/thumbnail, standalone
build/release metadata and its CI tests. The launcher catalogue is unchanged.

## Reference fidelity and data

The native SMB1 engine is pinned to SMB-Vanilla commit
20f4e02614412f9dbd407dd4e4fab4ade5eb79da. All four engine translation units are
compiled. Native executable game logic, level data, item/enemy behavior,
transitions, final-world messages and second-quest logic remain present.

The complete original program/data layout was reconstructed from doppelganger's
disassembly with a small five-pass 6502 assembler using py65 1.2.0. The data ends
at $10000; WorldAddrOffsets remains at $9CB4. Original graphics are the 8 KiB CHR
pattern table. Program/pattern hashes are checked by tools/assets.py. Combined
with the standard vertical-mirroring header, their SHA-1 matches the NTSC
reference ea343f4e445a9050d4b4fbac2c77d0693b1d0922. This reconstruction checks
source-data fidelity; it is not a claim of cycle-accurate emulation.

The small Mario icon sprite comes directly from the supplied Mario Wiki assets.
The source manifest/provenance and vendor notes retain original credits/rights.
Offline generated table/icon verification passes.

## Native tests

TinyCC compiles and runs tests/test.c with signed 8-bit char semantics and
wrapping signed arithmetic. ARM GCC builds production code with warnings as
errors, excluding unused-parameter/local diagnostics inherited from the core.
Runtime engine invariant checks remain enabled and recover safely.

## Lossless optimization regression

Before editing the engine, the original executable, source, size/performance
reports and save fixtures were preserved locally under ignored
`build/mario-optimization/before/`. The same differential harness was first
compiled against that unoptimized port, then against the optimized port.

- `tests/fidelity.c` hashes 42,656 serialized tick states and 331 complete
  RGB565 images. These include the native scenarios below and the 8,500-frame
  reference input sequence. Both versions produce the frozen fingerprint
  `3f10cd3237a76c28`; the test fails on a mismatch. Serialization excludes
  native pointers, structure padding and host byte order.
- All 34 generated level/start/ending save files remain byte-identical to
  the unoptimized fixtures. `tests/legacy-save.bin` is a frozen old save.
- `tests/rom.c` checks all 65,536 address-map inputs, verifies every one of
  the 10,530 retained bytes against the original tables, checks exact legacy
  save import/continuation, and rejects correctly checksummed snapshots with
  invalid pointers before changing live state or entering core recovery.
- `tools/assets.py --check` validates the complete original table/pattern
  hashes, the disassembly data-range manifest, every fixed constant table
  extent and deterministic generated files. All 131 original `.db`/`.dw`
  spans are retained. Dynamic area/enemy pointers retain conservative
  contiguous 512-byte windows at every root, including sub-areas; metatile
  banks retain 256 bytes, all VRAM streams retain their terminator reads and
  the complete music tail remains present. Final packing uses 115 spans.
- `tests/test_packing.py` verifies that ELF name compaction preserves public
  symbols, imports, section symbols, the entry point and EADK metadata.

The original 64 KiB CPU address-space backing is replaced by its 2 KiB writable
RAM. Constant tables are read directly from flash; no full ROM copy is needed.
The 16 KiB PPU backing is replaced by the two name/attribute tables and palette
(2,080 bytes). The renderer and save layout retain their original data exactly.
No game rules, levels, graphics, input cadence or drawing cadence were reduced.
No runtime decompressor, heap allocation or new audio behavior was introduced.

## Gameplay and save scenarios

- Original title, Start input, gameplay and frame advancement pass.
- Every one of the 32 level starts loads its original world/level. Automatic
  entrance-pipe walks proceed to the actual underground/water areas. The four
  terrain types are reached, with terrain mask 15.
- Exact save restoration is compared for 300 subsequent control ticks on each
  stage: 9,600 paired continuation ticks, checking all game RAM, PPU memory and
  displayed sprite/OAM data. Another initial continuation comparison covers
  the first level. Pointer rebasing comes from the restored original RAM.
- Every one of the 4,404 save bytes is individually corrupted and rejected.
- Every adaptation screen renders inside the screen and 2,560-pixel strip
  capacity. Rendering does not advance simulation or statistics.
- Native power-up scenarios verify mushroom growth, Fire Flower transformation
  and Star invincibility through the original item handler.
- A final-world unit scenario drives the original victory-message state machine,
  waits for the final-world button and verifies the second-quest unlock. This
  is not an automated playthrough of the complete 8-4 maze.
- An intentionally invalid dispatch returns through the adapter's checkpoint
  rather than terminating the process or overwriting a save.

A reference input recording from nathsou/smb drives the engine with real button
inputs and reaches 1-2 at frame 1,975, 1-3 at 4,537 and 1-4 at 6,276. It therefore
clears 1-1, 1-2 and 1-3. It does not validate a full 32-stage completion. The first
three stages originate from HappyLee/Mars608's TASVideos #3728 recording, as
noted in the upstream file. This recording is test data, absent from the NWA.

## Actual ARM application

The compiled app passes on N0110/Epsilon (148,928 bytes RAM allowance),
N0115/custom RAM (133,652), N0120/Epsilon (153,676) and Upsilon (107,674).
Each profile checks held-key launch protection, start/movement/jump input,
pause, cancelled restart, exact restoration, Home and On-Off exits, preservation
of a sentinel calculator file, RAM cleanup and firmware-memory integrity.

The builtin embedded checkpoint is explicitly exercised with a corrupted
engine dispatch on all four profiles: the app displays its error screen, exits
safely and retains the prior valid save. The final-world fixture also runs on
all four profiles and unlocks the second quest through the real B input.

All 32 native-generated level fixtures are then loaded into the N0110 ARM app,
restored byte-exactly, resumed and played with movement, run and jump input.
World/level identity, render performance, clean exit and sentinel preservation
are checked for each. There are 32 corresponding ARM screenshots. These are
short per-stage execution tests, not an uninterrupted completion of all stages.

Two-player title selection via Alpha is verified, along with guide navigation.
Full-storage testing verifies retry, return, explicit exit and preservation of
the original full record. Original game-state snapshots open frozen in pause.

Across the tested profiles/levels, maximum draw time is 13 ms and there are no
frames over the 33 ms drawing interval. The production loop runs 60 simulation
steps per second and draws at up to 30 Hz. Core tick timings round below 1 ms in
the sampled emulator clock. These are emulator observations; display timing
and feel on a physical calculator remain to be checked.

## Build and footprint

ARM GCC 15.2.1, nwlink 1.0.0, C11, -Os, LTO, -fwrapv, -fsigned-char and
-fno-strict-aliasing. Standalone and module builds complete. The lossless packer
checks identical release/debug installed bytes on all four target profiles.

| Resource | Bytes |
| --- | ---: |
| Standalone NWA | 101,868 |
| Installed flash | 75,881 |
| Static RAM | 14,142 |
| Deepest observed ARM stack | 432 |
| Save payload / including record overhead | 4,404 / 4,421 |
| Render strip | 5,120 |
| Compressed launcher icon | 237 |

Compared with the first complete build, NWA size falls by 28,508 bytes (21.9%),
installed flash by 21,584 bytes (22.1%) and static RAM by 77,792 bytes (84.6%).
Maximum observed rendering remains 13 ms, with no frames over 33 ms on the
four profiles and 32 stage scenarios. No CPU interpreter or full-screen pixel
buffer is used. The original sound engine
advances its counters, but no audio output is implemented on the calculator.

Screenshots in docs come from the compiled ARM app; detailed build and ARM
reports are kept under ignored output/. The downloadable local app is copied
to build/apps/SuperMarioBros.nwa.

The isolated publication snapshot was rebuilt with the unmodified common
headers and emulator from upstream main `f0469907ccc0e6bae2a02d75b0c858a06270df61`. Native
fidelity/ROM checks, offline assets, private-symbol packing and all four-profile
ARM/32-stage scenarios pass. Root build/release checks place the app once in
Individual Games; `games/games.json` and all launcher sources remain unchanged.

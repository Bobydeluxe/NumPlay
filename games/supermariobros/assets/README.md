# Original data and artwork

The game and its artwork belong to **Nintendo**. This is an unofficial NumWorks
adaptation by **Bobydeluxe**; these assets retain their original rights and are
not relicensed by the GPL licence for the adapter/engine code.

The user-supplied references are the
[Super Mario Bros. article](https://www.mariowiki.com/Super_Mario_Bros.) and
[asset category](https://www.mariowiki.com/Category:Super_Mario_Bros._assets).

- `small-mario.png` is the original 12x16 sprite from
  [SMB Smallmario.png](https://www.mariowiki.com/File:SMB_Smallmario.png), downloaded
  from `https://www.mariowiki.com/images/0/02/SMB_Smallmario.png`. It creates the
  launcher icon with nearest-neighbour scaling.
- `tables.bin` reconstructs the original 32 KiB program/data address space from
  doppelganger's annotated disassembly, as preserved in
  [MitchellSternke/SuperMarioBros-C](https://github.com/MitchellSternke/SuperMarioBros-C/blob/5bb920506a869cdc1fe2ef888a6526a1cf991630/docs/smbdis.asm).
  Only the C core executes; the reconstructed bytes supply its original level,
  enemy, palette, geometry and lookup tables. `symbols.json` records labels.
  `data-ranges.json` records every `.db`/`.dw` directive in that same disassembly.
  The NWA now embeds a compact 10,530-byte subset, retaining all these data
  directives plus contiguous windows for every dynamic area/enemy/metatile
  root, complete VRAM command streams and the complete music-data tail. The
  removed 6502 instructions were never executed by the native C engine.
- `patterns.chr` is the original 8 KiB pattern table preserved as a separate
  graphics asset in [threecreepio/smb-glitchedworlds](https://github.com/threecreepio/smb-glitchedworlds).
  Its exact source commit is recorded in `provenance.json`. This table contains
  both sprite and background tiles, the original font and title drawing data.
  The adapter does not run that project's modified game logic or title.
- `src/palette.h` uses the Mesen RGB palette supplied by SMB-Vanilla.

`tools/assets.py` regenerates `src/data.c`, `src/rom.h` and `src/icon.png` offline
with Pillow. It verifies the source-data manifest, all fixed table extents and
preserves original bytes and address semantics within every retained span.
`--check` validates the table/pattern SHA-256 hashes and generated outputs without
writing. `tools/assemble_tables.py` can reconstruct `tables.bin` from the pinned
disassembly with py65 1.2.0; it checks the original address layout and SHA-256.
Ordinary ARM builds use the checked-in generated C and do not need these tools.

For verification, combining the reconstructed program and graphics with the
standard vertical-mirroring iNES header produces SHA-1
`ea343f4e445a9050d4b4fbac2c77d0693b1d0922`, the original NTSC reference identified
by SMB-Vanilla. No cartridge dump was downloaded and no `.nes` file is needed
on the calculator.

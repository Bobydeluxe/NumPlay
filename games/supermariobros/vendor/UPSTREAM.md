# Engine provenance

The SMB1 core is from [SMB-Vanilla by nukep](https://github.com/nukep/SMB-Vanilla),
commit `20f4e02614412f9dbd407dd4e4fab4ade5eb79da`, under GNU GPL version 3.
See `LICENSE-SMB-Vanilla.md`. The original game, data and artwork are Nintendo's.
Bobydeluxe's NumWorks adapter is also GPL-3.0-only.

Only the SMB1 engine is compiled: `common.c`, `common_sound.c`, `area.c` and
`smb1only.c`, with their headers. Lost Levels-specific source is not included.
Gameplay runs natively in C. There is no 6502 interpreter, generic NES emulator,
SDL, OpenGL or operating-system dependency in the calculator build.

Local changes to the engine:

- `interface.h`: point at immutable graphics in flash instead of copying them
  into RAM; store the supplied vertical scroll value rather than the horizontal
  value (SMB1 uses horizontal scrolling and a zero vertical scroll).
  Keep only the 2 KiB writable CPU RAM and 2,080 bytes of name/attribute/palette
  memory. Original constant addresses resolve to packed immutable flash data
  through generated `src/rom.h`; dynamic ROM pointers use a checked address map.
  PPU writes are checked before indexing the compact memory.
- `smb1only.c`: bound writable VRAM command buffers by the actual CPU RAM size.
- `base.h`: TinyCC feature-macro compatibility; route invariant failures through
  the adapter's recoverable game-loop checkpoint instead of process termination.
  Keep runtime checks even when libc debug assertions are disabled.
- `common.c`: split the VRAM terminator's post-increment into explicit steps;
  mark one local used only by debug diagnostics; remove a redundant nonnegative
  comparison on an unsigned byte. Game behavior is preserved.

Compile with `-fwrapv -fsigned-char -fno-strict-aliasing`. Signed `char` matters
for the original loop-command parser on ARM, whose default differs from the
desktop build. The adapter supplies keyboard input, original OAM timing,
60 simulation steps per second, strip rendering, pause UI and checked saves.
Sound-engine counters still advance; no audio callback is installed because
this calculator version has no sound output.

The upstream project's [documented differences](https://github.com/nukep/SMB-Vanilla/blob/20f4e02614412f9dbd407dd4e4fab4ade5eb79da/docs/differences.md)
apply: no NES lag-frame/cycle artifacts, and arbitrary out-of-bounds worlds are
not guaranteed. This does not replace or shorten the original 32-level campaign.

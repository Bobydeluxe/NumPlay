> **Doom is a [NumPlay](../../README.md) game that comes on its own.** It is too big to sit next to NumPlay. Get `Doom.nwa` from the [latest release](https://github.com/Mason363/NumPlay/releases/latest).

<p align="center">
  <img src="src/icon.png" width="84" alt="Doom icon">
</p>

<h1 align="center">Doom</h1>

<p align="center">
  <b>Rip and tear through the moon bases of Mars, on the NumWorks calculator.</b><br>
  id Software's Doom, the shareware episode: Knee-Deep in the Dead, all nine levels, run by the real Doom engine.
</p>

<p align="center">
  <a href="https://github.com/Mason363/NumPlay/releases/latest/download/Doom.nwa"><b>Download Doom.nwa</b></a>
  &nbsp;·&nbsp; <a href="#controls">Controls</a> &nbsp;·&nbsp; <a href="#doom-from-your-own-wad">Your own WAD</a>
</p>

<p align="center">
  <img src="../../docs/media/doom.gif" width="640" alt="The marine fighting zombiemen and imps through the Phobos lab">
</p>

## Highlights

* **The real game:** the Doom engine itself (Chocolate Doom, by way of RP2040 Doom), not a lookalike. Monsters think, doors open, lifts rise and fall, barrels explode and the levels play exactly as on a PC in 1993: even the demos it plays on the title screen run as they were recorded.
* **Knee-Deep in the Dead, all of it:** Hangar, Nuclear Plant, Toxin Refinery, Command Control, Phobos Lab, Central Processing, Computer Station, Phobos Anomaly and the secret Military Base, with their secrets, keys, switches and the Barons of Hell at the end.
* **Everything from the shareware WAD:** every wall, floor, sprite, menu and screen, the status bar with the marine's face, the automap, the intermission maps of Phobos with your kills, items, secrets and time, and the episode's ending.
* **The whole 320 x 200 picture:** drawn at Doom's own resolution with its light levels and its palette (far floors and ceilings smoothed, so they don't shimmer as you move), the screen flashing red when you're hurt and gold when you pick something up, and stretched to the calculator's 320 x 240 the way monitors of the day made Doom's pixels a little taller.
* **The five skill levels,** from I'm too young to die to Nightmare!, and the menus as they were: New Game, Options, Load Game, Save Game, Read This! and Quit Game, with their questions and Doom's quit messages.
* **All of vanilla's options:** messages, Graphic Detail (high, or low with its blocky double pixels), Screen Size from the smallest view in its border to the full screen without the status bar, five levels of brightness, and the automap's follow mode, grid, marks and zoom.
* **The calculator's own options:** every key can do any action (Options, Keys), the automap can lie over the game, secret areas can show on it in their own colour, and a Cheats menu has every code: god mode, all weapons and keys, no clipping, warping, the powers, the whole map and the chainsaw. The codes can be typed too, letters and all.
* **Saved like in the game:** six save slots, named after their level, and Quick Save and Quick Load. Home keeps the moment you were at, and the next start puts you back there. Your saves are also copied into `doom_saves.py`, so installing the app again doesn't erase them.
* **Your own WAD:** build `Doom.nwa` from Doom, The Ultimate Doom, Doom II, Final Doom, Freedoom or your own maps, as many as fit in the calculator ([below](#doom-from-your-own-wad)).

<table>
  <tr>
    <td align="center"><img src="docs/title.png" width="390" alt="The title screen"><br><sub>The title screen</sub></td>
    <td align="center"><img src="docs/hangar.png" width="390" alt="E1M1: Hangar, the courtyard"><br><sub>E1M1: Hangar</sub></td>
  </tr>
  <tr>
    <td align="center"><img src="docs/imps.png" width="390" alt="Imps coming down a corridor"><br><sub>Imps down the corridor</sub></td>
    <td align="center"><img src="docs/fight.png" width="390" alt="Shooting zombiemen and a sergeant"><br><sub>Zombiemen around the yellow key</sub></td>
  </tr>
  <tr>
    <td align="center"><img src="docs/lab.png" width="390" alt="An imp's fireball over the slime"><br><sub>E1M5: Phobos Lab</sub></td>
    <td align="center"><img src="docs/automap.png" width="390" alt="The automap"><br><sub>The automap</sub></td>
  </tr>
  <tr>
    <td align="center"><img src="docs/intermission.png" width="390" alt="The intermission"><br><sub>Hangar finished</sub></td>
    <td align="center"><img src="docs/menu.png" width="390" alt="The main menu over the game"><br><sub>The main menu over the game</sub></td>
  </tr>
  <tr>
    <td align="center"><img src="docs/keys.png" width="390" alt="The calculator's keys, in Read This!"><br><sub>Read This!: the calculator's keys</sub></td>
    <td align="center"><img src="docs/ending.png" width="390" alt="The episode's ending"><br><sub>The end of Knee-Deep in the Dead</sub></td>
  </tr>
  <tr>
    <td align="center"><img src="docs/options.png" width="390" alt="The Options menu"><br><sub>Options: detail, screen size, keys, cheats</sub></td>
    <td align="center"><img src="docs/cheats.png" width="390" alt="The Cheats menu"><br><sub>The Cheats menu</sub></td>
  </tr>
  <tr>
    <td align="center"><img src="docs/border.png" width="390" alt="A smaller screen size in its border"><br><sub>A smaller screen, as vanilla Doom shrinks it</sub></td>
    <td align="center"><img src="docs/freedoom.png" width="390" alt="Freedoom: Phase 2, built from its WAD"><br><sub>Freedoom: Phase 2, built from its own WAD</sub></td>
  </tr>
</table>

## How to play

Press a key on the title screen for the menu, then choose New Game, Knee-Deep in the Dead and a skill level. Find the exit of each level, shooting what gets in your way: keycards open the doors of their colour, and walls that look a little different may hide a secret. Back opens doors and presses switches. EXE (or ⌫) opens the menu and pauses the game, where you can save, load, change the options or read the keys again.

## Controls

| Key | What it does |
| --- | --- |
| Up / Down | Walk forward / back |
| Left / Right | Turn |
| OK | Fire |
| Back | Use: open doors, press switches |
| EXE or ⌫ | Menu (and pause) |
| shift (held) | Strafe with Left and Right |
| x,n,t / var | Strafe left / right |
| alpha (held) | Run |
| 1 to 7 | Choose a weapon |
| ( / ) | The previous / next weapon you have |
| Toolbox | Automap: the map, the map over the game, the game |
| + / − | Screen size; on the automap, zoom |
| × / ÷ / 0 | Automap: follow you, grid, the whole map |
| 8 / 9 | Automap: mark the spot, clear the marks |
| √ | Brightness |
| ln / log | Quick Save / Quick Load |
| π | Pause |
| ans | Type a cheat code |
| Home | Keep the moment and quit |

In menus, the arrows choose, OK or EXE confirms and Back goes back. When the game asks a question, OK says yes and Back says no. Read This! in the menu shows the keys as they are, and the very first game starts with a word on them.

**Options, Keys** gives any key any action: choose an action, press the key (it shows each action's keys, on pages that Left and Right turn; THE KEYS AS THEY CAME puts them back). The keys and the options are kept in `doom.cfg`.

**Options, More Options**: Brightness (off and four levels, as vanilla's gamma), Always run (as vanilla's autorun trick: you run without holding alpha), Map over the view (the automap's second mode), Secrets on the map (the walls of secret areas in their own colour), and Screen refresh (which way the calculator's panel is refreshed, see below).

**Options, Cheats**: God mode (IDDQD), all weapons and keys (IDKFA), weapons and ammo (IDFA), no clipping (IDCLIP), warp (IDCLEV, Left and Right choose the level), a power (IDBEHOLD: invulnerability, strength, invisibility, radiation suit, the map, light amplification), the whole map (IDDT) and the chainsaw (IDCHOPPERS). The last line, or the ans key in the game, lets you type any code, IDMYPOS and IDSPISPOPD too: the letter keys type what is written above them in alpha, numbers are typed as they are.

## Good to know

* **No sound:** the calculator has no speaker, so Doom plays in silence.
* **Saves:** a save takes a few KB of the calculator's 42 KB of files, and its copy in `doom_saves.py` as much again. If there is no room, the game says so: delete a slot or an old script.
* **Its own pace:** the game thinks 35 times a second, as Doom always did, and draws up to 35 frames a second. On the N0120, every view of all nine levels is drawn within 1/35 s (the slowest takes 22 ms). On the N0110, 79% of the views of E1M1 and 90 to 96% of the other levels' are; in low detail (Options, Graphic Detail), 98 to 100%. Where a room is too much, it draws fewer frames, but monsters, doors and you keep their speed. (Measured in an emulator from the middle of every part of each map, looking 8 ways: `tools/fps.py`.)
* **No tearing:** the calculator's panel refreshes itself from its memory 40 times a second, a column at a time. Each part of a frame is sent once the panel has gone past it and before it comes back, so the panel never shows two frames at once. If your calculator's panel refreshes the other way (some may), choose Right to left in More Options.
* **Only the shareware episode:** the registered episodes (The Shores of Hell and Inferno) are not part of the free shareware WAD, but you can [build Doom from yours](#doom-from-your-own-wad).

## Doom from your own WAD

The release's `Doom.nwa` has the shareware episode. With the WAD of a Doom you own, or a free one, you can make your own: in `games/doom`, `make WAD=` packs the maps you choose and builds `output/doom.nwa`, to install as `Doom.nwa` in place of the release's.

```sh
cd games/doom
make check WAD=path/to/doom2.wad MAPS="MAP01 MAP02"        # Doom II's first two maps
make check WAD=path/to/doom.wad MAPS=E2M1-E2M3              # the start of The Shores of Hell
make check WAD=path/to/freedoom2.wad MAPS=MAP01-MAP02       # Freedoom: Phase 2
make check WAD=path/to/doom2.wad PWAD=mymap.wad MAPS=MAP01  # your own map, over Doom II's
make test WAD=... MAPS=...                                  # plays it on a computer first
```

`check` builds it and checks it fits (it says so if it does not); `make WAD=...` alone just builds it. It needs what the shareware build needs (`arm-none-eabi-gcc`, nwlink) and Python 3.

**What fits.** An app gets 2 MB of the calculator's flash (2,031,616 bytes) and the engine takes 220 KB of it, so about 1.8 MB is left for the WAD. A whole Doom, Doom II or Freedoom is 10 to 15 MB even packed, so a build has a few maps: only what they can show goes in (their walls and floors, the sprites of their monsters and items and of what those turn into or fire, the weapons, and the screens of their episode), never the sounds or the music, as the calculator has no speaker. id's own maps are smaller than Freedoom's, whose pictures are more detailed. Measured with Freedoom 0.13:

| WAD | Maps | Packed | `Doom.nwa` (`doom.bin`) | |
| --- | --- | --- | --- | --- |
| `freedoom1.wad` | E1M1 | 1,350,344 bytes | 1,569,924 bytes | fits, 450 KB to spare |
| `freedoom1.wad` | E3M8 | 1,608,004 bytes | 1,827,584 bytes | fits (with the end of the third episode) |
| `freedoom1.wad` | E1M1 E1M2 | 1,923,516 bytes | 2,143,096 bytes | 112 KB too big |
| `freedoom2.wad` | MAP01 | 1,167,888 bytes | 1,387,468 bytes | fits |
| `freedoom2.wad` | MAP01-MAP02 | 1,498,260 bytes | 1,717,840 bytes | fits |
| `freedoom2.wad` | MAP01-MAP03 | 1,843,164 bytes | 2,062,744 bytes | 31 KB too big |
| `freedoom2.wad` | MAP07 | 2,051,912 bytes | 2,271,492 bytes | too big: its monsters' pictures |
| `freedoom2.wad` | MAP30 | 2,055,744 bytes | 2,275,324 bytes | too big: the Icon of Sin brings every monster |

**Choosing the maps.** `MAPS` takes map names (`E2M3`, `MAP07`), whole episodes (`E2`) and ranges (`MAP01-MAP06`), separated by spaces; without it, every map is taken (only a small WAD fits). In the game, each map leads to the next one in the build (a secret exit to a map left out goes on as the normal exit); after the last one, the episode ends, or for Doom II the game, with its text and cast of monsters. New Game starts at the first map of the episode chosen, an episode with none says so, IDCLEV and the Cheats menu warp only to the maps in the build, and the title skips the demos of maps left out.

**Which Doom.** The WAD's file name says which game it is, as for Chocolate Doom: keep it (`doom.wad`, `doom2.wad`, `tnt.wad`, `plutonia.wad`, `freedoom1.wad`, `freedoom2.wad`...). Doom II's maps play with all its monsters and the super shotgun, its intermission texts and its cast at the end; the third episode of Doom ends with its scrolling picture. PWADs go over the IWAD in the order given (`PWAD="a.wad b.wad"`): their maps, textures, flats and sprites replace the IWAD's of the same name or are added, as Chocolate Doom's `-merge` does. They must be maps for vanilla Doom (not Boom's, ZDoom's or Hexen's).

**If a map does not go in,** the build says why: a level can have up to 255 different wall textures, 511 sectors, 253 tags and 32,767 vertexes. Its things must also fit in the 130 KB of RAM the calculator gives an app, as id's and Freedoom's do; a huge map made for modern ports may not, and the game then stops with an error. `make test` plays each map of the build on a computer. Only Freedoom has been tried so far, as id's WADs are not free to share; the shareware DOOM1.WAD goes through the same tool (`make check WAD=build/ref/doom1.wad`).

## How it's made

The calculator gives an app about 130 KB of RAM: Doom on a PC asked for 4 MB. This port runs Graham Sanderson's [RP2040 Doom](https://github.com/kilograham/rp2040-doom), which made all of the shareware DOOM1.WAD playable on a 264 KB microcontroller: the WAD is packed into its WHD format (1.4 MB without the sounds and music, read in place from flash), the levels are never copied to RAM, and the game's objects are squeezed with 16-bit pointers.

The calculator has less RAM still, and no room for a frame buffer: the screen is drawn in five bands of 64 columns. Each frame the engine walks the level once and keeps every column of wall and floor it would draw, then draws each band whole, its floors and ceilings, walls, then its sprites, masked walls and weapon in vanilla's order, clipped as vanilla clips them; then the status bar, messages and menus, and sends it to the screen through Doom's palette. The picture is vanilla's to the pixel: the three demos play tic for tic as in Chocolate Doom, and their frames match Chocolate Doom's (but for a few pixels where vanilla reads past the end of a texture), except on floors and ceilings far enough that their texels are more than a pixel apart. There vanilla's point sampling jumps from texel to texel as the view moves, and a flat a half, a quarter or an eighth the size (each pixel the average of the texels under it, in the palette's nearest colour) is drawn instead.

`make` builds `output/doom.nwa` (needs `arm-none-eabi-gcc` and Node.js for nwlink). `make test` plays it on a computer with the sanitizers on, and `make media` takes the pictures on this page from there. `python3 tools/fetch.py build/ref` downloads the shareware DOOM1.WAD and `make data WAD=build/ref/doom1.wad` packs it again into `src/doom1.whx`, with its flats made smaller for far floors and ceilings in `src/doom1.mip` (`tools/mips.py`). `tools/fps.py` measures the frame rate of every level in the emulator, from every spot of each map.

## Credits

Doom is id Software's game (1993). Not affiliated with id Software, ZeniMax or Bethesda.

* **[RP2040 Doom](https://github.com/kilograham/rp2040-doom)** by Graham Sanderson: the engine, the WHD format and its converter (`tools/whd_gen`), and the renderer this port draws in bands.
* **[Chocolate Doom](https://github.com/chocolate-doom/chocolate-doom)** by Simon Howard and its contributors, which RP2040 Doom is made from.
* **The Doom source code** released by id Software in 1997 under the GNU GPL.
* **The shareware DOOM1.WAD** (version 1.9) by id Software: its levels, pictures and texts.
* **[ADPCM-XQ](https://github.com/dbry/adpcm-xq)** by David Bryant and **[LodePNG](https://github.com/lvandeve/lodepng)** by Lode Vandevenne, used by the converter.

Made by Mason Chen as part of NumPlay.

## License

The engine is licensed under the GNU General Public License, version 2 or later (`COPYING.md`); RP2040 Doom's own files under the BSD 3-Clause license, as they say at their top; the converter's libraries under their own licenses (`tools/whd_gen`). NumPlay's code is licensed under the GNU General Public License v3.0: see the `LICENSE` file at the root of the repository. The game's data in `src/doom1.whx` and `src/doom1.mip` is id Software's shareware DOOM1.WAD (version 1.9), the free episode they let players share, repacked for the calculator: it stays id Software's.

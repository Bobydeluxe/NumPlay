/* The shareware DOOM1.WAD, packed by tools/whd_gen into RP2040 Doom's WHX format: read in place. And its flats
 * made smaller by tools/mips.py, for the floors and ceilings far away (pd_render.cpp). */
#ifdef __APPLE__ /* (a Mac's computer build: Mach-O's names and sections) */
    .const_data
    .balign 4
    .globl _doom_whd
_doom_whd:
    .incbin DOOM_WHX
    .balign 4
    .globl _doom_mips
_doom_mips:
    .incbin DOOM_MIP
#else
    .section .rodata.doom_whd, "a"
    .balign 4
    .global doom_whd
doom_whd:
    .incbin DOOM_WHX
    .section .rodata.doom_mips, "a"
    .balign 4
    .global doom_mips
doom_mips:
    .incbin DOOM_MIP
#endif

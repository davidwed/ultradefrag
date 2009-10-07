;------------------------------------------------------------------------------
;
; UltraDefrag for DOS - powerful disk defragmentation tool for FAT12/16/32
;                       filesystems, DOS operating system and 8086 processor.
;
;------------------------------------------------------------------------------

Main goals:

1. Use defragmentation algorithm similar to UltraDefrag for Windows algorithm.
2. Single COM executable.
3. 8086 CPU
4. 100 kb RAM
5. no protected mode, DPMI, EMS, XMS
6. support for as much DOS versions as possible

Development techniques:

1. Write it entirely in assembler.
2. Use BIOS calls when possible (OS independent code).
3. Use direct access to disc through reading/writing sectors.

Project has been started at 7 October 2009.

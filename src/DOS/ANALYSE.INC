;------------------------------------------------------------------------------
; UltraDefrag for DOS - powerful disk defragmentation tool for FAT12/16/32
;                       filesystems, DOS operating system and 8086 processor.
; Copyright (c) 2009 by Dmitri Arkhangelski (dmitriar@gmail.com).
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
;------------------------------------------------------------------------------

; volume analysis code

volume_analysis:
		jmp	volume_analysis_proc
		
		; local data here
		mesg1 db 13,10,"Sectors per cluster: 0x$"
		mesg2 db 13,10,"Bytes per sector:    0x$"
		mesg3 db 13,10,"Clusters per drive:  0x$"
		mesg4 db 13,10,'$'
		
		get_drive_info_failed db "GetDriveInfo (36h int21h) failed!",13,10,'$'
		sec_per_clus_36_21   dw 0
		bytes_per_sec_36_21  dw 0
		clus_per_drive_36_21 dw 0

volume_analysis_proc:
		call	get_drive_info_36_21
		
		; print drive information
		mov	dx, offset mesg1
		call	print
		mov	ax, word ptr sec_per_clus_36_21
		show_ax
		mov	dx, offset mesg2
		call	print
		mov	ax, word ptr bytes_per_sec_36_21
		show_ax
		mov	dx, offset mesg3
		call	print
		mov	ax, word ptr clus_per_drive_36_21
		show_ax
		mov	dx, offset mesg4
		call	print

		retn

;------------------------------------------------------------------------------
; gets drive information through 36h int21h DOS function
; returns wrong information under Win XP
;------------------------------------------------------------------------------
get_drive_info_36_21:
		mov	ah, 36h
		mov	dl, 1
		add	dl, byte ptr drive_letter
		sub	dl, 'A'
		int	21h
		cmp	ax, 0FFFFh
		jne	save_drive_info
		mov	dx, offset get_drive_info_failed
		call	print
		retn
save_drive_info:
		mov	word ptr sec_per_clus_36_21, ax
		mov	word ptr bytes_per_sec_36_21, cx
		mov	word ptr clus_per_drive_36_21, dx
		
		retn

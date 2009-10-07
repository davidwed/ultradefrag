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

; defragmentation related definitions
ANALYZE_ACTION	EQU	0
DEFRAG_ACTION	EQU	1
OPTIMIZE_ACTION	EQU	2

FILTER_LEN		EQU 120

;------------------------------------------------------------------------------
; for debugging
;------------------------------------------------------------------------------
hex_out	MACRO
		cmp     al, 10
		sbb     al, 69h
		das
		int	29h
ENDM

show_r	MACRO	register:REQ
		push	register

		IFDIFI	<register>,<ax>
			mov	ax, register
		ENDIF
		mov	al, ah
		shr	al, 1		
		shr	al, 1		
		shr	al, 1		
		shr	al, 1		
		hex_out

		pop	ax
		push	ax

		mov	al, ah
		and	al, 01111b
		hex_out

		pop	ax
		push	ax

		shr	al, 1		
		shr	al, 1		
		shr	al, 1		
		shr	al, 1		
		hex_out

		pop	ax
		push	ax

		and	al, 01111b
		hex_out

		pop	register
ENDM

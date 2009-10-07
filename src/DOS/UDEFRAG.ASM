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

		.8086		; must work on 8086+
		.model	tiny

		.xlist
		include UDEFRAG.INC
		.list

		.code
		
		org	80h
cmd_len  db ?
cmd_line db ?		

		org	100h
START:
		; display copyright message
		mov	dx, offset copyright_msg
		mov	ah, 9h
		int	21h

		; free unused memory
		stack_top equ program_length+100h+500h
		mov	sp, stack_top
		mov	ah, 4Ah
		mov	bx, (stack_top shr 4) + 1
		int	21h
		jnc	analyze_cmd_line
		jmp	no_mem
		
		; init variables
		xor	ax, ax
		mov	byte ptr in_filter, al
		mov	byte ptr ex_filter, al

analyze_cmd_line:		
		; analyze command line
		cld
		xor	cx, cx
		mov	cl, cmd_len
		cmp	cl, 1
		jg	L
		jmp	show_usage
L:
		mov	di, offset cmd_line

find_param:
		mov	al, 20h
		repz	scasb
		test	cx, cx
		jnz	L0
		jmp	cmd_scanned
L0:
		dec	di ; now DI points to the current parameter
		inc	cx
		cmp	byte ptr [di], 0Dh
		jne	L01
		jmp	cmd_scanned
L01:
		cmp	byte ptr [di+1], ':'
		jne	not_a_drive_letter
		mov	al, byte ptr [di]
		mov	byte ptr drive_letter, al
not_a_drive_letter:
		cmp	byte ptr [di], '-'
		je	option_found
		; skip this paramter
skip_param:
		mov	si, di
		lodsb
		inc	di
		dec	cx
		test	cx, cx
		jz	cmd_scanned
		cmp	al, 0Dh
		je	cmd_scanned
		cmp	al, 20h
		je	find_param
		jmp	short skip_param
option_found:
		inc	di
		dec	cx

		; test for literal options
		mov	al, byte ptr [di]
		cmp	al, 'Z'
		jle	test_letter
		sub	al, 'z' - 'Z' ; convert to lowercase

test_letter:
		; test for command
		cmp	al, 'A'
		jne	L1
		mov	byte ptr action, ANALYZE_ACTION
		jmp	short skip_param
L1:
		cmp	al, 'D'
		jne	L3
		mov	byte ptr action, DEFRAG_ACTION
		jmp	short skip_param
L3:
		cmp	al, 'O'
		jne	L5
		mov	byte ptr action, OPTIMIZE_ACTION
		jmp	short skip_param
L5:
		; test for options
		cmp	al, 'I'
		jne	L7
		mov	bx, offset in_filter
		call	set_filter
		jmp	short skip_param
L7:
		cmp	al, 'E'
		jne	L9
		mov	bx, offset ex_filter
		call	set_filter
		jmp	short skip_param
L9:
		cmp	al, 'S'
		jne	L11
		call	set_size_threshold
		jmp	short skip_param
L11:
		cmp	al, 'F'
		jne	L14
		mov	bx, offset frag_threshold
		call	set_word
		jmp	short skip_param
L14:
		cmp	al, 'R'
		jne	L15
		mov	bx, offset refresh_int
		call	set_word
		jmp	short skip_param
L15:

cmd_scanned:
		; print parameters
		mov	al, byte ptr action
		add	al, '0'
		mov	byte ptr msg[9], al
		mov	al, byte ptr drive_letter
		mov	byte ptr msg[21], al
		mov	dx, offset msg
		mov	ah, 9h
		int	21h
		mov	dx, offset msg2
		call	print
		mov	di, offset in_filter
		mov	cx, FILTER_LEN
		xor	ax, ax
		repne	scasb
		dec	di
		mov	byte ptr [di], '$'
		mov	dx, offset in_filter
		mov	ah, 9h
		int	21h
		mov	dx, offset crlf
		call	print
		mov	dx, offset msg3
		call	print
		mov	di, offset ex_filter
		mov	cx, FILTER_LEN
		xor	ax, ax
		repne	scasb
		dec	di
		mov	byte ptr [di], '$'
		mov	dx, offset ex_filter
		mov	ah, 9h
		int	21h
		mov	dx, offset crlf
		call	print
		mov	dx, offset msg4
		call	print
		mov	ax, word ptr size_threshold+2
		show_r	ax
		mov	ax, word ptr size_threshold
		show_r	ax
		mov	dx, offset crlf
		call	print
		mov	dx, offset msg5
		call	print
		mov	ax, word ptr frag_threshold
		show_r	ax
		mov	dx, offset crlf
		call	print
		mov	dx, offset msg6
		call	print
		mov	ax, word ptr refresh_int
		show_r	ax
		mov	dx, offset crlf
		call	print
		

		; exit program
		mov	ax, 4C00h
		int	21h

;------------------------------------------------------------------------------
; set_filter - sets filter, di+1 points to value, bx points to buffer
;------------------------------------------------------------------------------
set_filter:
		push	ax
		push	cx
		push	si
		push	di
		
		mov	cx, FILTER_LEN
		mov	si, di
		inc	si
		mov	di, bx
repeat1:
		lodsb
		cmp	al, 0Dh
		je	break1
		cmp	al, 20h
		je	break1
		stosb
		dec	cx
		jnz	repeat1
		
break1:
		; fill unused space with zeros
		xor	ax, ax
		inc	cx
		;;show_r cx
		rep	stosb

		pop	di
		pop	si
		pop	cx
		pop	ax
		retn

;------------------------------------------------------------------------------
; set_size_threshold - sets size threshold, di+1 points to it
;------------------------------------------------------------------------------
set_size_threshold:
		push	ax
		push	cx
		push	dx
		push	si
		
		xor	cx, cx
		xor	ax, ax
		mov	si, di
		inc	si
repeat3:
		mov	al, byte ptr [si]
		cmp	al, '0'
		jl	break3
		cmp	al, '9'
		jg	break3
		sub	al, '0'
		jc	break3
		;lea	cx, [cx + cx*4]
		;lea	cx, [ax + cx*2]
		push	ax
		mov	ax, 10
		mul	cx
		mov	cx, ax
		pop	ax
		add	cx, ax
		
		inc	si
		jmp	short repeat3
break3:
		mov	word ptr size_threshold, cx
		
		; check for the K suffix
		mov	al, byte ptr [si]
		cmp	al, 'k'
		je	k_suffix_found
		cmp	al, 'K'
		jne	exit3
k_suffix_found:
		mov	ax, 1024
		mul	cx
		mov	si, offset size_threshold
		mov	word ptr [si], ax
		mov	word ptr [si+2], dx
exit3:		
		pop	si
		pop	dx
		pop	cx
		pop	ax
		retn

;------------------------------------------------------------------------------
; set_word - sets 16-bit parameter, di+1 points to value, bx - to buffer
;------------------------------------------------------------------------------
set_word:
		push	ax
		push	cx
		push	dx
		push	si
		
		xor	cx, cx
		xor	ax, ax
		mov	si, di
		inc	si
repeat2:
		mov	al, byte ptr [si]
		cmp	al, '0'
		jl	break2
		cmp	al, '9'
		jg	break2
		sub	al, '0'
		jc	break2
		;lea	cx, [cx + cx*4]
		;lea	cx, [ax + cx*2]
		push	ax
		mov	ax, 10
		mul	cx
		mov	cx, ax
		pop	ax
		add	cx, ax
		
		inc	si
		jmp	short repeat2
break2:
		mov	si, bx
		mov	word ptr [si], cx
		
		pop	si
		pop	dx
		pop	cx
		pop	ax
		retn

;------------------------------------------------------------------------------
; print - prints string pointed by dx, modifies ax
;------------------------------------------------------------------------------
print:
		mov	ah, 9h
		int	21h
		retn
		
;------------------------------------------------------------------------------
; no_mem - displays 'No enough memory!' message and terminates the program
;------------------------------------------------------------------------------
no_mem:
		mov	dx, offset no_mem_msg
		mov	ah, 9h
		int	21h
		mov	ax, 4C01h
		int	21h
		
;------------------------------------------------------------------------------
; show_usage - shows program usage and terminates the program
;------------------------------------------------------------------------------
show_usage:
		mov	dx, offset usage
		mov	ah, 9h
		int	21h
		mov	ax, 4C00h
		int	21h

;------------------------------------------------------------------------------
; data block
;------------------------------------------------------------------------------

copyright_msg db "UltraDefrag for DOS v0.0.1 - powerful disk defragmenter."
              db 13,10
              db "Copyright (c) Dmitri Arkhangelski, 2009.",13,10,13,10
              db "UltraDefrag comes with ABSOLUTELY NO WARRANTY",13,10,13,10,'$'
usage         db "Usage: udefrag [command] [options] [volumeletter:]",13,10,13,10
              db "  The default action is to display this help screen.",13,10,13,10
              db "Commands:",13,10
              db "  -a         analyze specified volume",13,10
              db "  -d         defragment volume",13,10
              db "  -o         optimize volume space",13,10,13,10
              db "Options:",13,10
              db "  -iMask    include filter,",13,10
              db "            no more than 120 characters long",13,10
              db "  -eMask    exclude filter",13,10
              db "            no more than 120 characters long",13,10
              db "  -sSize    size threshold,",13,10
              db "            Size = Number[Suffix],",13,10
              db "            Suffix is optional, use K for kilobytes",13,10
              db "  -fNumber  fragments threshold",13,10
              db "  -rNumber  progress refresh interval",13,10
              db 13,10
              db "  All Numbers must be less than 65536.",13,10
              db '$'

no_mem_msg    db "No enough memory!",13,10,'$'

drive_letter   db 0			  
action         db ANALYZE_ACTION
size_threshold dd 0
frag_threshold dw 0
refresh_int    dw 0

msg           db "Action = 0, Letter = 0",13,10,'$'
msg2          db "Include: $"
msg3          db "Exclude: $"
msg4          db "Size threshold: 0x$"
msg5          db "Fragments threshold: 0x$"
msg6          db "Progress refresh interval: 0x$"
crlf          db 13,10,'$'

in_filter     db FILTER_LEN+1 dup (?)
ex_filter     db FILTER_LEN+1 dup (?)

program_length equ $-START

;------------------------------------------------------------------------------

END START

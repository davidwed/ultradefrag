/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2009 by Dmitri Arkhangelski (dmitriar@gmail.com).
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* io.c - low level I/O functions */

#include "driver.h"

NTSTATUS OpenTheFile(PFILENAME pfn, HANDLE *phFile)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK ioStatus;
	ULONG flags = FILE_SYNCHRONOUS_IO_NONALERT;

	if(!pfn || !phFile) return STATUS_INVALID_PARAMETER;
	
	if(pfn->is_dir) flags |= FILE_OPEN_FOR_BACKUP_INTENT;
	else flags |= FILE_NO_INTERMEDIATE_BUFFERING;
	
	if(nt4_system){
		InitializeObjectAttributes(&ObjectAttributes,&pfn->name,0,NULL,NULL);
	} else {
		InitializeObjectAttributes(&ObjectAttributes,&pfn->name,OBJ_KERNEL_HANDLE,NULL,NULL);
	}
	return ZwCreateFile(phFile,FILE_GENERIC_READ,&ObjectAttributes,&ioStatus,
			  NULL,0,FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_OPEN,flags,
			  NULL,0);
}

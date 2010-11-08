/*
 *  ZenWINX - WIndows Native eXtended library.
 *  Copyright (c) 2007-2010 by Dmitri Arkhangelski (dmitriar@gmail.com).
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

/**
 * @file dbg.c
 * @brief Debugging.
 * @addtogroup Debug
 * @{
 */

#include "ntndk.h"
#include "zenwinx.h"

typedef struct _DBG_OUTPUT_DEBUG_STRING_BUFFER {
    ULONG ProcessId;
    UCHAR Msg[4096-sizeof(ULONG)];
} DBG_OUTPUT_DEBUG_STRING_BUFFER, *PDBG_OUTPUT_DEBUG_STRING_BUFFER;

#define DBG_BUFFER_SIZE (4096-sizeof(ULONG))

HANDLE hDbgSynchEvent = NULL;

int  __stdcall winx_debug_print(char *string);

/**
 * @brief Initializes the synchronization 
 * objects used in the debugging routines.
 * @note Internal use only.
 */
void winx_init_synch_objects(void)
{
	short event_name[64];
	
	/*
	* Attach process ID to the event name
	* to make it safe for execution
	* by many parallel processes.
	*/
	_snwprintf(event_name,64,L"\\winx_dbgprint_synch_event_%u",
		(unsigned int)(DWORD_PTR)(NtCurrentTeb()->ClientId.UniqueProcess));
	event_name[63] = 0;
	
	if(hDbgSynchEvent == NULL){
		(void)winx_create_event(event_name,SynchronizationEvent,&hDbgSynchEvent);
		if(hDbgSynchEvent) (void)NtSetEvent(hDbgSynchEvent,NULL);
	}
}

/**
 * @brief Destroys the synchronization
 * objects used in the debugging routines.
 * @note Internal use only.
 */
void winx_destroy_synch_objects(void)
{
	winx_destroy_event(hDbgSynchEvent);
}

/**
 * @brief DbgPrint() native equivalent.
 * @note DbgPrint() exported by ntdll library cannot
 * be captured by DbgPrint loggers, therefore we are
 * using our own routine for debugging purposes.
 */
void __cdecl winx_dbg_print(char *format, ...)
{
	char *buffer = NULL;
	va_list arg;
	int length;
	
	/* never call winx_dbg_print_ex() here! */

	if(!format) return;
	buffer = winx_virtual_alloc(DBG_BUFFER_SIZE);
	if(!buffer){
		winx_debug_print("Cannot allocate memory for winx_dbg_print()!");
		return;
	}

	/* 1a. store resulting ANSI string into buffer */
	va_start(arg,format);
	memset(buffer,0,DBG_BUFFER_SIZE);
	length = _vsnprintf(buffer,DBG_BUFFER_SIZE - 1,format,arg);
	(void)length;
	buffer[DBG_BUFFER_SIZE - 1] = 0;
	va_end(arg);

	/* 1b. send resulting ANSI string to the debugger */
	winx_debug_print(buffer);
	
	winx_virtual_free(buffer,DBG_BUFFER_SIZE);
}

/**
 * @brief Delivers a message to the DbgView program.
 * @param[in] string the string to be delivered.
 * @return Zero for success, negative value otherwise.
 * @note Internal use only.
 */
int __stdcall winx_debug_print(char *string)
{
	HANDLE hEvtBufferReady = NULL;
	HANDLE hEvtDataReady = NULL;
	HANDLE hSection = NULL;
	LPVOID BaseAddress = NULL;
	LARGE_INTEGER SectionOffset;
	ULONG ViewSize = 0;

	UNICODE_STRING us;
	NTSTATUS Status;
	OBJECT_ATTRIBUTES oa;
	LARGE_INTEGER interval;
	
	DBG_OUTPUT_DEBUG_STRING_BUFFER *dbuffer;
	int length;
	
	if(string == NULL)
		return (-1);

	/* never call winx_dbg_print_ex() here! */

	/* 0. synchronize with other threads */
	if(hDbgSynchEvent){
		interval.QuadPart = -(11000 * 10000); /* 11 sec */
		Status = NtWaitForSingleObject(hDbgSynchEvent,FALSE,&interval);
		if(Status != WAIT_OBJECT_0) return (-1);
	}
	
	/* 1. Open debugger's objects. */
	RtlInitUnicodeString(&us,L"\\BaseNamedObjects\\DBWIN_BUFFER_READY");
	InitializeObjectAttributes(&oa,&us,0,NULL,NULL);
	Status = NtOpenEvent(&hEvtBufferReady,SYNCHRONIZE,&oa);
	if(!NT_SUCCESS(Status)) goto failure;
	
	RtlInitUnicodeString(&us,L"\\BaseNamedObjects\\DBWIN_DATA_READY");
	InitializeObjectAttributes(&oa,&us,0,NULL,NULL);
	Status = NtOpenEvent(&hEvtDataReady,EVENT_MODIFY_STATE,&oa);
	if(!NT_SUCCESS(Status)) goto failure;

	RtlInitUnicodeString(&us,L"\\BaseNamedObjects\\DBWIN_BUFFER");
	InitializeObjectAttributes(&oa,&us,0,NULL,NULL);
	Status = NtOpenSection(&hSection,SECTION_ALL_ACCESS,&oa);
	if(!NT_SUCCESS(Status)) goto failure;
	SectionOffset.QuadPart = 0;
	Status = NtMapViewOfSection(hSection,NtCurrentProcess(),
		&BaseAddress,0,0,&SectionOffset,(SIZE_T *)&ViewSize,ViewShare,
		0,PAGE_READWRITE);
	if(!NT_SUCCESS(Status)) goto failure;
	
	/* 2. Send a message. */
	/*
	* wait a maximum of 10 seconds for the debug monitor 
	* to finish processing the shared buffer
	*/
	interval.QuadPart = -(10000 * 10000);
	if(NtWaitForSingleObject(hEvtBufferReady,FALSE,&interval) != WAIT_OBJECT_0)
		goto failure;
	
	dbuffer = (DBG_OUTPUT_DEBUG_STRING_BUFFER *)BaseAddress;

	/* write the process id into the buffer */
	dbuffer->ProcessId = (DWORD)(DWORD_PTR)(NtCurrentTeb()->ClientId.UniqueProcess);

	(void)strncpy(dbuffer->Msg,string,4096-sizeof(ULONG));
	dbuffer->Msg[4096-sizeof(ULONG)-1] = 0;
	/* ensure that the buffer has new line character */
	length = strlen(dbuffer->Msg);
	if(dbuffer->Msg[length-1] != '\n'){
		if(length == (4096-sizeof(ULONG)-1)){
			dbuffer->Msg[length-1] = '\n';
		} else {
			dbuffer->Msg[length] = '\n';
			dbuffer->Msg[length+1] = 0;
		}
	}

	/* signal that the buffer contains meaningful data and can be read */
	(void)NtSetEvent(hEvtDataReady,NULL);
	
	NtCloseSafe(hEvtBufferReady);
	NtCloseSafe(hEvtDataReady);
	if(BaseAddress)
		(void)NtUnmapViewOfSection(NtCurrentProcess(),BaseAddress);
	NtCloseSafe(hSection);
	if(hDbgSynchEvent) (void)NtSetEvent(hDbgSynchEvent,NULL);
	return 0;

failure:
	NtCloseSafe(hEvtBufferReady);
	NtCloseSafe(hEvtDataReady);
	if(BaseAddress)
		(void)NtUnmapViewOfSection(NtCurrentProcess(),BaseAddress);
	NtCloseSafe(hSection);
	if(hDbgSynchEvent) (void)NtSetEvent(hDbgSynchEvent,NULL);
	return (-1);
}

/*
* winx_dbg_print_ex() code.
*/

typedef struct _NT_STATUS_DESCRIPTION {
	unsigned long status;
	char *description;
} NT_STATUS_DESCRIPTION, *PNT_STATUS_DESCRIPTION;

NT_STATUS_DESCRIPTION status_descriptions[] = {
	{ STATUS_SUCCESS,                "Operation successful"           },
	{ STATUS_OBJECT_NAME_INVALID,    "Object name invalid"            },
	{ STATUS_OBJECT_NAME_NOT_FOUND,  "Object name not found"          },
	{ STATUS_OBJECT_NAME_COLLISION,  "Object name already exists"     },
	{ STATUS_OBJECT_PATH_INVALID,    "Path is invalid"                },
	{ STATUS_OBJECT_PATH_NOT_FOUND,  "Path not found"                 },
	{ STATUS_OBJECT_PATH_SYNTAX_BAD, "Bad syntax in path"             },
	{ STATUS_BUFFER_TOO_SMALL,       "Buffer is too small"            },
	{ STATUS_ACCESS_DENIED,          "Access denied"                  },
	{ STATUS_NO_MEMORY,              "Not enough memory"              },
	{ STATUS_UNSUCCESSFUL,           "Operation failed"               },
	{ STATUS_NOT_IMPLEMENTED,        "Not implemented"                },
	{ STATUS_INVALID_INFO_CLASS,     "Invalid info class"             },
	{ STATUS_INFO_LENGTH_MISMATCH,   "Info length mismatch"           },
	{ STATUS_ACCESS_VIOLATION,       "Access violation"               },
	{ STATUS_INVALID_HANDLE,         "Invalid handle"                 },
	{ STATUS_INVALID_PARAMETER,      "Invalid parameter"              },
	{ STATUS_NO_SUCH_DEVICE,         "Device not found"               },
	{ STATUS_NO_SUCH_FILE,           "File not found"                 },
	{ STATUS_INVALID_DEVICE_REQUEST, "Invalid device request"         },
	{ STATUS_END_OF_FILE,            "End of file reached"            },
	{ STATUS_WRONG_VOLUME,           "Wrong volume"                   },
	{ STATUS_NO_MEDIA_IN_DEVICE,     "No media in device"             },
	{ STATUS_UNRECOGNIZED_VOLUME,    "Cannot recognize file system"   },
	{ STATUS_VARIABLE_NOT_FOUND,     "Environment variable not found" },
	
	/* A file cannot be opened because the share access flags are incompatible. */
	{ STATUS_SHARING_VIOLATION,      "File is locked by another process"},
	
	{ 0xffffffff,                    NULL                             }
};

/**
 * @brief Retrieves a human readable
 * explanation of the NT status code.
 * @param[in] status the NT status code.
 * @return A pointer to string containing
 * the status explanation.
 * @note This function returns explanations
 * only for well known codes. Otherwise 
 * it returns an empty string.
 * @par Example:
 * @code
 * printf("%s\n",winx_get_error_description(STATUS_ACCESS_VIOLATION));
 * // prints "Access violation".
 * @endcode
 */
char * __stdcall winx_get_error_description(unsigned long status)
{
	int i;
	
	for(i = 0;; i++){
		if(status_descriptions[i].description == NULL) break;
		if(status_descriptions[i].status == status)
			return status_descriptions[i].description;
	}
	return "";
}

/**
 * @brief Delivers an error message
 * to the DbgView program.
 * @details The error message includes 
 * NT status and its explanation.
 * @param[in] status the NT status code.
 * @param[in] format the format string.
 * @param[in] ... the parameters.
 * @par Example:
 * @code
 * NTSTATUS Status = NtCreateFile(...);
 * if(!NT_SUCCESS(Status)){
 *     winx_dbg_print_ex(Status,"Cannot create %s file",filename);
 * }
 * @endcode
 */
void __cdecl winx_dbg_print_ex(unsigned long status,char *format, ...)
{
	#define ERR_MSG_LENGTH 4096 /* because impossible to define exactly */
	char *buffer;
	va_list arg;
	int length;

	/*
	* Localized strings sometimes appears on the screen 
	* in abracadabra form, therefore we should avoid  
	* FormatMessage() calls in debugging.
	* When I tried to get debugging information on NT4/w2k
	* systems running under MS Virtual PC emulator, DbgView
	* displayed localized text in completely unreadable form
	* (as sequence of question characters).
	*/
	
	if(format == NULL)
		return;

	buffer = winx_heap_alloc(ERR_MSG_LENGTH);
	if(!buffer){
		DebugPrint("Cannot allocate memory for winx_dbg_print_ex()!\n");
		return;
	}

	va_start(arg,format);
	memset(buffer,0,ERR_MSG_LENGTH);
	length = _vsnprintf(buffer,ERR_MSG_LENGTH - 1,format,arg);
	(void)length;
	buffer[ERR_MSG_LENGTH - 1] = 0;
	DebugPrint("%s: 0x%x! %s.\n",buffer,(unsigned int)status,
			winx_get_error_description(status));
	va_end(arg);
	
	winx_heap_free(buffer);
}

/**
 * @brief Delivers formatted message, decorated by specified
 * character at both sides, to DbgView program.
 * @param[in] ch character to be used for decoration.
 * If zero value is passed, DEFAULT_DBG_PRINT_DECORATION_CHAR is used.
 * @param[in] width the width of the resulting message, in characters.
 * If zero value is passed, DEFAULT_DBG_PRINT_HEADER_WIDTH is used.
 * @param[in] format the format string.
 * @param[in] ... the parameters.
 */
void __cdecl winx_dbg_print_header(char ch, int width, char *format, ...)
{
	char *buffer = NULL;
	va_list arg;
	int length, left, right;
	char *s_left = NULL, *s_right = NULL, *s_result = NULL;
	
	/* never call winx_dbg_print_ex() here! */

	if(!format) return;
	buffer = winx_virtual_alloc(DBG_BUFFER_SIZE);
	if(!buffer){
		winx_debug_print("Cannot allocate memory for winx_dbg_print_header()!");
		return;
	}

	/* store formatted string into buffer */
	va_start(arg,format);
	memset(buffer,0,DBG_BUFFER_SIZE);
	length = _vsnprintf(buffer,DBG_BUFFER_SIZE - 1,format,arg);
	buffer[DBG_BUFFER_SIZE - 1] = 0;
	va_end(arg);
	
	if(width <= 0)
		width = DEFAULT_DBG_PRINT_HEADER_WIDTH;

	if(length + 4 > width){
		/* print string not decorated */
		winx_debug_print(buffer);
	} else {
		/* decorate string */
		if(ch == 0)
			ch = DEFAULT_DBG_PRINT_DECORATION_CHAR;
		right = left = (width - length - 2) / 2;
		if(width - length - 2 - left * 2)
			right ++;
		
		s_left = winx_heap_alloc(left + 1);
		if(s_left == NULL){
			winx_debug_print("Cannot allocate memory for winx_dbg_print_header()!");
			winx_debug_print(buffer); /* print the string not decorated */
			goto done;
		}
		s_right = winx_heap_alloc(right + 1);
		if(s_right == NULL){
			winx_debug_print("Cannot allocate memory for winx_dbg_print_header()!");
			winx_debug_print(buffer); /* print the string not decorated */
			goto done;
		}
		s_result = winx_heap_alloc(left + right + 2 + length + 1);
		if(s_result == NULL){
			winx_debug_print("Cannot allocate memory for winx_dbg_print_header()!");
			winx_debug_print(buffer); /* print the string not decorated */
			goto done;
		}
		
		memset(s_left,ch,left);
		s_left[left] = 0;
		memset(s_right,ch,right);
		s_right[right] = 0;
		
		_snprintf(s_result,left + right + 2 + length + 1,"%s %s %s",
			s_left, buffer, s_right);
		s_result[left + right + 2 + length] = 0;
		winx_debug_print(s_result);
	}

done:
	winx_virtual_free(buffer,DBG_BUFFER_SIZE);
	if(s_left) winx_heap_free(s_left);
	if(s_right) winx_heap_free(s_right);
	if(s_result) winx_heap_free(s_result);
}

/** @} */

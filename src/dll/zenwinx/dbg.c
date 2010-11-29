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

typedef struct _winx_dbg_log_entry {
	struct _winx_dbg_log_entry *next;
	struct _winx_dbg_log_entry *prev;
	char *buffer;
} winx_dbg_log_entry;

HANDLE hDbgSynchEvent = NULL;
HANDLE hFlushLogEvent = NULL;
int debug_print_enabled = 1;

/**
 * @brief Indicates whether debug logging
 * to the file is enabled or not.
 * @details The logging can be enabled
 * either by setting UD_ENABLE_DBG_LOG
 * environment variable before the program
 * launch or by calling winx_enable_dbg_log().
 * Size of the log is limited by available
 * memory, which is rational since logging
 * is rarely needed and RAM is usually
 * smaller than free space available on disk.
 */
int logging_enabled = 0;
winx_dbg_log_entry *dbg_log = NULL;

int  __stdcall winx_debug_print(char *string);
void winx_flush_dbg_log(void);
void winx_remove_dbg_log(void);

/**
 * @brief Initializes the synchronization 
 * objects used in the debugging routines.
 * @details This routine checks also for
 * UD_ENABLE_DBG_LOG environment variable controlling
 * debug output to the log file. This is done,
 * because winx_init_synch_objects is called
 * on early stage of the library initialization.
 * @note Internal use only.
 */
void winx_init_synch_objects(void)
{
	short event_name[64];
	wchar_t buffer[16];
	
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
	
	_snwprintf(event_name,64,L"\\winx_flush_log_synch_event_%u",
		(unsigned int)(DWORD_PTR)(NtCurrentTeb()->ClientId.UniqueProcess));
	event_name[63] = 0;
	
	if(hFlushLogEvent == NULL){
		(void)winx_create_event(event_name,SynchronizationEvent,&hFlushLogEvent);
		if(hFlushLogEvent) (void)NtSetEvent(hFlushLogEvent,NULL);
	}
	
	/* check for UD_ENABLE_DBG_LOG */
	if(winx_query_env_variable(L"UD_ENABLE_DBG_LOG",buffer,sizeof(buffer)/sizeof(wchar_t)) >= 0){
		if(!wcscmp(buffer,L"1"))
			logging_enabled = 1;
	}

	winx_remove_dbg_log();
}

/**
 * @brief Destroys the synchronization
 * objects used in the debugging routines.
 * @note Internal use only.
 */
void winx_destroy_synch_objects(void)
{
	winx_flush_dbg_log();
	winx_destroy_event(hDbgSynchEvent);
	winx_destroy_event(hFlushLogEvent);
	hDbgSynchEvent = NULL;
}

/**
 * @brief Removes the log file.
 * @note Internal use only.
 */
void winx_remove_dbg_log(void)
{
	char windir[MAX_PATH + 1];
	char path[MAX_PATH + 1];
	
	if(winx_get_windows_directory(windir,MAX_PATH + 1) < 0){
		DebugPrint("winx_remove_dbg_log: cannot get windows directory path\n");
		return;
	}
	_snprintf(path,MAX_PATH + 1,"%s\\UltraDefrag\\udefrag.log",windir);
	path[MAX_PATH] = 0;
	(void)winx_delete_file(path);
}

/**
 * @brief Appends all collected debugging
 * information to the log file.
 * @note Internal use only.
 */
void winx_flush_dbg_log(void)
{
	char windir[MAX_PATH + 1];
	char path[MAX_PATH + 1];
	WINX_FILE *f;
	winx_dbg_log_entry *log_entry;
	int length;
	char crlf[] = "\r\n";
	LARGE_INTEGER interval;
	NTSTATUS Status;
	
	if(dbg_log == NULL)
		return;
	
	/* synchronize with other threads */
	if(hFlushLogEvent){
		interval.QuadPart = -(11000 * 10000); /* 11 sec */
		Status = NtWaitForSingleObject(hFlushLogEvent,FALSE,&interval);
		if(Status != WAIT_OBJECT_0)	return;
	}
	
	/* disable parallel access to dbg_log list */
	if(hDbgSynchEvent){
		interval.QuadPart = -(11000 * 10000); /* 11 sec */
		Status = NtWaitForSingleObject(hDbgSynchEvent,FALSE,&interval);
		if(Status != WAIT_OBJECT_0){
			if(hFlushLogEvent)
				(void)NtSetEvent(hFlushLogEvent,NULL);
			return;
		}
		debug_print_enabled = 0;
		(void)NtSetEvent(hDbgSynchEvent,NULL);
	}

	if(winx_get_windows_directory(windir,MAX_PATH + 1) < 0){
		DebugPrint("winx_remove_dbg_log: cannot get windows directory path\n");
		goto done;
	}
	_snprintf(path,MAX_PATH + 1,"%s\\UltraDefrag\\udefrag.log",windir);
	path[MAX_PATH] = 0;
	f = winx_fbopen(path,"a",DBG_BUFFER_SIZE);
	if(f != NULL){
		for(log_entry = dbg_log; log_entry; log_entry = log_entry->next){
			if(log_entry->buffer){
				length = strlen(log_entry->buffer);
				if(length > 0){
					if(log_entry->buffer[length - 1] == '\n')
						log_entry->buffer[length - 1] = 0;
					(void)winx_fwrite(log_entry->buffer,sizeof(char),length,f);
					/* add a proper newline characters */
					(void)winx_fwrite(crlf,sizeof(char),2,f);
				}
			}
			if(log_entry->next == dbg_log) break;
		}
		winx_fclose(f);
		winx_list_destroy((list_entry **)(void *)&dbg_log);
	}

done:
	/* synchronize with other threads */
	if(hDbgSynchEvent){
		interval.QuadPart = -(11000 * 10000); /* 11 sec */
		Status = NtWaitForSingleObject(hDbgSynchEvent,FALSE,&interval);
		if(Status != WAIT_OBJECT_0){
			if(hFlushLogEvent)
				(void)NtSetEvent(hFlushLogEvent,NULL);
			return;
		}
		debug_print_enabled = 1;
		(void)NtSetEvent(hDbgSynchEvent,NULL);
	}

	if(hFlushLogEvent)
		(void)NtSetEvent(hFlushLogEvent,NULL);
}

/**
 * @brief Enables debug logging to the file.
 */
void __stdcall winx_enable_dbg_log(void)
{
	logging_enabled = 1;
}

/**
 * @brief Disables debug logging to the file.
 */
void __stdcall winx_disable_dbg_log(void)
{
	logging_enabled = 0;
	winx_flush_dbg_log();
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
		winx_debug_print("winx_dbg_print: cannot allocate required amount of memory");
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
	winx_dbg_log_entry *new_log_entry = NULL, *last_log_entry = NULL;
	
	if(string == NULL)
		return (-1);

	/* never call winx_dbg_print_ex() here! */

	/* 0. synchronize with other threads */
	if(hDbgSynchEvent){
		interval.QuadPart = -(11000 * 10000); /* 11 sec */
		Status = NtWaitForSingleObject(hDbgSynchEvent,FALSE,&interval);
		if(Status != WAIT_OBJECT_0) return (-1);
	}
	
	if(debug_print_enabled == 0){
		if(hDbgSynchEvent)
			(void)NtSetEvent(hDbgSynchEvent,NULL);
		return 0;
	}
	
	/* save message if logging to file is enabled */
	if(logging_enabled){
		if(dbg_log)
			last_log_entry = dbg_log->prev;
		new_log_entry = (winx_dbg_log_entry *)winx_list_insert_item((list_entry **)(void *)&dbg_log,
			(list_entry *)last_log_entry,sizeof(winx_dbg_log_entry));
		if(new_log_entry == NULL){
			/* not enough memory */
		} else {
			new_log_entry->buffer = winx_strdup(string);
			if(new_log_entry->buffer == NULL){
				/* not enough memory */
				winx_list_remove_item((list_entry **)(void *)&dbg_log,(list_entry *)new_log_entry);
			}
		}
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
	{ STATUS_SUCCESS,                "operation successful"           },
	{ STATUS_OBJECT_NAME_INVALID,    "object name invalid"            },
	{ STATUS_OBJECT_NAME_NOT_FOUND,  "object name not found"          },
	{ STATUS_OBJECT_NAME_COLLISION,  "object name already exists"     },
	{ STATUS_OBJECT_PATH_INVALID,    "path is invalid"                },
	{ STATUS_OBJECT_PATH_NOT_FOUND,  "path not found"                 },
	{ STATUS_OBJECT_PATH_SYNTAX_BAD, "bad syntax in path"             },
	{ STATUS_BUFFER_TOO_SMALL,       "buffer is too small"            },
	{ STATUS_ACCESS_DENIED,          "access denied"                  },
	{ STATUS_NO_MEMORY,              "not enough memory"              },
	{ STATUS_UNSUCCESSFUL,           "operation failed"               },
	{ STATUS_NOT_IMPLEMENTED,        "not implemented"                },
	{ STATUS_INVALID_INFO_CLASS,     "invalid info class"             },
	{ STATUS_INFO_LENGTH_MISMATCH,   "info length mismatch"           },
	{ STATUS_ACCESS_VIOLATION,       "access violation"               },
	{ STATUS_INVALID_HANDLE,         "invalid handle"                 },
	{ STATUS_INVALID_PARAMETER,      "invalid parameter"              },
	{ STATUS_NO_SUCH_DEVICE,         "device not found"               },
	{ STATUS_NO_SUCH_FILE,           "file not found"                 },
	{ STATUS_INVALID_DEVICE_REQUEST, "invalid device request"         },
	{ STATUS_END_OF_FILE,            "end of file reached"            },
	{ STATUS_WRONG_VOLUME,           "wrong volume"                   },
	{ STATUS_NO_MEDIA_IN_DEVICE,     "no media in device"             },
	{ STATUS_UNRECOGNIZED_VOLUME,    "cannot recognize file system"   },
	{ STATUS_VARIABLE_NOT_FOUND,     "environment variable not found" },
	
	/* A file cannot be opened because the share access flags are incompatible. */
	{ STATUS_SHARING_VIOLATION,      "file is locked by another process"},
	
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
		DebugPrint("winx_dbg_print_ex: cannot allocate %u bytes of memory",ERR_MSG_LENGTH);
		return;
	}

	va_start(arg,format);
	memset(buffer,0,ERR_MSG_LENGTH);
	length = _vsnprintf(buffer,ERR_MSG_LENGTH - 1,format,arg);
	(void)length;
	buffer[ERR_MSG_LENGTH - 1] = 0;
	DebugPrint("%s: 0x%x: %s\n",buffer,(unsigned int)status,
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
		winx_debug_print("winx_dbg_print_header: cannot allocate required amount of memory");
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
			winx_debug_print("winx_dbg_print_header: cannot allocate required amount of memory");
			winx_debug_print(buffer); /* print the string not decorated */
			goto done;
		}
		s_right = winx_heap_alloc(right + 1);
		if(s_right == NULL){
			winx_debug_print("winx_dbg_print_header: cannot allocate required amount of memory");
			winx_debug_print(buffer); /* print the string not decorated */
			goto done;
		}
		s_result = winx_heap_alloc(left + right + 2 + length + 1);
		if(s_result == NULL){
			winx_debug_print("winx_dbg_print_header: cannot allocate required amount of memory");
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

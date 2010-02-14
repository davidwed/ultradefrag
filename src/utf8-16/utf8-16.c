/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
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

/*
* UTF-8 to UTF-16 converter.
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>

int convert_file(char *path);

int __cdecl main(int argc, char **argv)
{
	int i;

	/* Display copyright. */
	printf("UTF-8 to UTF-16 converter v0.0.1, Copyright (c) Dmitri Arkhangelski, 2010.\n");
	printf("UTF-8 to UTF-16 converter comes with ABSOLUTELY NO WARRANTY. This is free\n");
	printf("software and you are welcome to redistribute it under certain conditions.\n\n");

	/*
	* Convert all specified files
	* from UTF-8 to UTF-16 encoding.
	*/
	if(argc < 2){
		printf("Required argument missing!\n");
		printf("Usage example: utf8-16 c:\\texts\\*.txt\n");
		return 1;
	}
	
	for(i = 1; i < argc; i++){
		if(convert_file(argv[i]) < 0){
			printf("%s convertion failed!\n",argv[i]);
			return 2;
		}
	}
	printf("%u files converted.\n",i - 1);
	return 0;
}

int convert_file(char *path)
{
	struct _stat stat;
	int size;
	char *src;
	short *dest;
	FILE *f;
	
	/* skip trash */
	if(path == NULL) return (-1);
	if(path[0] == 0) return (-1);
	
	/* get file size */
	f = fopen(path,"rb");
	if(f == NULL){
		printf("Cannot open the file: %s!\n",strerror(errno));
		return (-1);
	}
	if(_fstat(_fileno(f),&stat) != 0){
		printf("Cannot retrieve the file statistics: %s!\n",strerror(errno));
		(void)fclose(f);
		return (-1);
	}
	(void)fclose(f);
	size = stat.st_size;
	
	/* skip directories */
	if(size == 0) return 0;
	
	/* allocate memory */
	src = malloc(size + 1);
	if(src == NULL){
		printf("Cannot allocate memory for src buffer!\n");
		return (-1);
	}
	dest = malloc((size + 1) << 1);
	if(dest == NULL){
		printf("Cannot allocate memory for dest buffer!\n");
		free(src);
		return (-1);
	}
	
	/* open the file */
	f = fopen(path,"rb");
	if(f == NULL){
		printf("Cannot open the file: %s!\n",strerror(errno));
		goto cleanup;
	}
	
	/* read the file contents */
	if(!fread(src,1,size,f)){
		printf("Cannot read the file: %s!\n",strerror(errno));
		(void)fclose(f);
		goto cleanup;
	}
	
	/* close the file */
	(void)fclose(f);
	
	/* convert the file contents */
	src[size] = 0;
	if(!MultiByteToWideChar(CP_UTF8,0,(char *)src,-1,dest,size + 1)){
		printf("Conversion failed!\n");
		goto cleanup;
	}
	dest[size] = 0;

	/* open the file for writing */
	f = fopen(path,"wb");
	if(f == NULL){
		printf("Cannot open the file: %s!\n",strerror(errno));
		goto cleanup;
	}
	
	/* write new file contents */
	if(!fwrite(dest,2,wcslen(dest),f)){
		printf("Cannot write to the file: %s!\n",strerror(errno));
		(void)fclose(f);
		goto cleanup;
	}
	
	/* close the file */
	(void)fclose(f);

	free(src);
	free(dest);
	return 0;

cleanup:
	free(src);
	free(dest);
	return (-1);
}

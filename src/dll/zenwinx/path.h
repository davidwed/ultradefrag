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

/*
 * path.c definitions.
 */

#ifndef _PATH_H_
#define _PATH_H_

void __stdcall winx_path_remove_extension(char *path);
void __stdcall winx_path_remove_filename(char *path);
void __stdcall winx_path_extract_filename(char *path);
void __stdcall winx_get_module_filename(char *path);

#endif /* _PATH_H_ */

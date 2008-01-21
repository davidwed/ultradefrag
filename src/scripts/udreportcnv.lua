#!/usr/local/bin/lua
--[[
  udreportcnv.lua - UltraDefrag report converter.
  Converts lua reports to HTML and other formats.
  Copyright (c) 2008 by Dmitri Arkhangelski (dmitriar@gmail.com).

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
--]]

-- USAGE: lua udreportcnv.lua <luar file with full path> <path to system32 directory> [-v]

-- parse command line
assert(arg[1],"Lua Report file name must be specified!")
assert(arg[2],"Path to the system32 directory\nmust be specified as second parameter!")

-- read options
dofile(arg[2] .. "\\udreportopts.lua")

-- source file reading
dofile(arg[1])


-- common functions

function write_ansi(f, ...)
	for i, v in ipairs(arg) do f:write(v) end
end

function write_unicode(f, ...)
	local b, j
	for i, v in ipairs(arg) do
		j = 1
		while true do
			b = string.byte(v, j)
			if b == nil then break end
			f:write(string.char(b), "\0")
			j = j + 1
		end
	end
end

-- converters

function produce_html_output()
	local filename
	local pos = 0

	repeat
		pos = string.find(arg[1],"\\",pos + 1,true)
		if pos == nil then filename = "FRAGLIST.HTM" ; break end
	until string.find(arg[1],"\\",pos + 1,true) == nil
	filename = string.sub(arg[1],0,pos - 1) .. "FRAGLIST.HTM"

	-- note that 'b' flag is needed for utf-16 files
	local f = assert(io.open(filename,"wb"))

	local write_data
	if use_utf16 == 0 then
		write_data = write_ansi
	else
		write_data = write_unicode
	end

	write_data(f,
		"<html><head><title>Fragmented files on ", volume_letter,
		":</title>\n", style, "</head>\n",
		"<body>\n<center>\n", title_tags.open,
		"Fragmented files on ", volume_letter,
		":", title_tags.close,
		"<table ", table_style, ">\n",
		"<tr><td class=\"c\"># fragments</td>\n",
		"<td class=\"c\">filename</td>\n",
		"<td class=\"c\">comment</td></tr>\n"
		)
	for i, v in ipairs(files) do
		local class
		if v.filtered == 1 then class = "f" else class = "u" end
		write_data(f,
			"<tr class=\"", class, "\"><td class=\"c\">", v.fragments,
			"</td><td>"
			)
		if use_utf16 == 0 then
			-- each <> brackets must be replaced with square brackets
			local tmp = string.gsub(v.name,"<","[")
			tmp = string.gsub(tmp,">","]")
			f:write(tmp)
		else
			for j, b in ipairs(v.uname) do
				f:write(string.char(b))
			end
		end
		write_data(f,
			"</td><td class=\"c\">", v.comment,
			"</td></tr>\n"
			)
	end
	write_data(f,"</table>\n</center>\n</body></html>")
	f:close()
	
	-- if option -v is specified, open report in default web browser
	if arg[3] == "-v" then
		if os.shellexec ~= nil then
			os.shellexec(filename,"open")
		else
			os.execute("cmd.exe /C " .. filename)
		end
	end
end


-- main source code
-- convertion to other (readable) formats
if produce_html == 1 then
	produce_html_output()
end

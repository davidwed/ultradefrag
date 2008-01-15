#!/usr/local/bin/lua
--[[
  udreportcnv.lua - UltraDefrag report converter.
  Converts lua reports to HTML and other formats.
  Copyright (c) 2007,2008 by Dmitri Arkhangelski (dmitriar@gmail.com).

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

-- report options

-- I. Output Formats
produce_html = 1

-- II. HTML specific options
use_utf16 = 0
style = [[
<style>
td {font-family: monospace; font-size: 10pt}
.c {text-align: center}
.f {background-color: #000000; color: #FFFFFF}
</style>
]]
title_tags = {open = "<pre><h3>", close = "</h3></pre>"}
table_style = [[border="1" color="#FFAA55" cellspacing="0" width="100%"]]

-- source file reading
dofile(arg[1])


-- common functions

function write_ansi(f, ...)
	for i, v in ipairs(arg) do f:write(v) end
end

function write_unicode(f, ...)
	local s, j
	for i, v in ipairs(arg) do
		j = 1
		while true do
			s = string.byte(v, j)
			if s == nil then break end
			s = string.char(s)
			-- replace "\n" characters with spaces because write call adds 0xd byte before "\n"
			if s == "\n" then
				f:write(" ")
			else
				f:write(s)
			end
			f:write("\0")
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

	local f = assert(io.open(filename,"w"))

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
			f:write(v.name)
		else
			for j, w in ipairs(v.uname) do
				f:write(string.char(w))
			end
		end
		write_data(f,
			"</td><td class=\"c\">", v.comment,
			"</td></tr>\n"
			)
	end
	write_data(f,"</table>\n</center>\n</body></html>")
	f:close()
	
	-- if option -v was specified, open report in default web browser
	os.execute("cmd.exe /C " .. filename)
end


-- main source code
-- convertion to other (readable) formats
if produce_html == 1 then
	produce_html_output()
end

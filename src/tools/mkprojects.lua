#!/usr/local/bin/lua
--[[
  mkprojects.lua - produce project files for various IDE's from one *.build file.
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

  SYNOPSIS:    lua mkprojects.lua <filename>
--]]

-- NOTES for C programmers: 
--   1. the first element of each array has index 1.
--   2. only nil and false values are false, all other including 0 are true

name, deffile, baseaddr, nativedll, umentry = "", "", "", 0, ""
src, rc, libs, adlibs = {}, {}, {}, {}

input_filename = ""
target_type, target_ext, target_name, nt4target_name = "", "", "", ""

-- MS Visual Studio backend
dsw_part1 = [[
Microsoft Developer Studio Workspace File, Format Version 6.00
# WARNING: DO NOT EDIT OR DELETE THIS WORKSPACE FILE!

###############################################################################

]]
dsw_part2 = [[

Package=<5>
{{{
}}}

Package=<4>
{{{
}}}

###############################################################################

Global:

Package=<5>
{{{
}}}

Package=<3>
{{{
}}}

###############################################################################

]]

function produce_msvc_project()
	local tt
	
	local f = assert(io.open(".\\" .. name .. ".dsw","w"))
	f:write(dsw_part1)
	f:write("Project: \"", name, "\"=.\\", name, ".dsp - Package Owner=<4>\n")
	f:write(dsw_part2)
	f:close()

	if(target_type ~= "gui") then
		tt = "\"Win32 (x86) Console Application\" 0x0103"
	else
		tt = "\"Win32 (x86) Application\" 0x0101"
	end

	f = assert(io.open(".\\" .. name .. ".dsp","w"))
	f:write("# Microsoft Developer Studio Project File - Name=\"", name,
		"\" - Package Owner=<4>\n",
		"# Microsoft Developer Studio Generated Build File, Format Version 6.00\n",
		"# ** DO NOT EDIT **\n\n",
		"# TARGTYPE ", tt, "\n\n" 
		)
	f:write("CFG=", name, " - Win32 Debug\n",
		"!MESSAGE This is not a valid makefile. To build this project using NMAKE,\n",
		"!MESSAGE use the Export Makefile command and run\n!MESSAGE \n",
		"!MESSAGE NMAKE /f \"", name, ".mak\".\n",
		"!MESSAGE \n",
		"!MESSAGE You can specify a configuration when running NMAKE\n",
		"!MESSAGE by defining the macro CFG on the command line. For example:\n",
		"!MESSAGE \n",
		"!MESSAGE NMAKE /f \"", name, ".mak\" CFG=\"", name, " - Win32 Debug\"\n",
		"!MESSAGE \n",
		"!MESSAGE Possible choices for configuration are:\n",
		"!MESSAGE \n",
		"!MESSAGE \"", name, " - Win32 Release\" (based on \"Win32 (x86) Application\")\n",
		"!MESSAGE \"", name, " - Win32 Debug\" (based on \"Win32 (x86) Application\")\n",
		"!MESSAGE \n\n",
		"# Begin Project\n",
		"# PROP AllowPerConfigDependencies 0\n",
		"# PROP Scc_ProjName \"\"\n",
		"# PROP Scc_LocalPath \"\"\n",
		"CPP=cl.exe\n",
		"MTL=midl.exe\n",
		"RSC=rc.exe\n\n"
		)
	f:write("!IF  \"$(CFG)\" == \"", name, " - Win32 Release\"\n\n"
		)

	f:close()
end

-- MinGW backend
function produce_mingw_project()
end

-- frontend
input_filename = arg[1]
if input_filename == nil then
	error("Filename must be specified!")
end
dofile(input_filename)
produce_msvc_project()
produce_mingw_project()
print("Project files from " .. input_filename .. " were produced successfully.\n")

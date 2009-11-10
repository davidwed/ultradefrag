--[[
This is the Ultra Defragmenter build configurator.
Copyright (c) 2007,2008,2009 by Dmitri Arkhangelski (dmitriar@gmail.com).

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

-- This program was originally written in Perl.

-- NOTES for C programmers: 
--   1. the first element of each array has index 1.
--   2. only nil and false values are false, all other including 0 are true

-- This program requires IUP library.
-- You may download it from http://sourceforge.net/projects/iup/files/
-- If you are running 32-bit system, download iup3_0_rc2_Win32_dll6_lib.zip archive,
-- unpack it and copy all DLL's to your system32 directory. Enjoy it!
require "iuplua51"
require "iupluaimglib51"

-- The following module requires also CD library:
-- http://sourceforge.net/projects/canvasdraw/files/
-- If you are running 32-bit system, download cd5_2_Win32_dll6_lib.zip archive,
-- unpack it and copy all DLL's to your system32 directory. Enjoy it!
require "iupluacontrols51"

iup.SetLanguage("ENGLISH")

-- initialize parameters
apply_patch = 0
udver, winxver, mingwbase, ddkbase, netpath, nsisroot, ziproot, vsbinpath, rosinc = "","","","","","","","",""
mingw64base = ""

-- make a backup copy of the file
f = assert(io.open("./SETVARS.BK","w"))

-- get parameters from SETVARS.CMD file
for line in io.lines("./SETVARS.CMD") do
	f:write(line,"\n")
	-- split line to a name-value pair
	for k, v in string.gmatch(line,"(.+)=(.+)") do
		-- print(k, v)
		-- f:write(k,"=",v,"\n")
		if string.find(k, "ULTRADFGVER") then udver = v
		elseif string.find(k, "ZENWINXVER") then winxver = v
		elseif string.find(k, "WINDDKBASE") then ddkbase = v
		elseif string.find(k, "MINGWBASE") then mingwbase = v
		elseif string.find(k, "NETRUNTIMEPATH") then netpath = v
		elseif string.find(k, "NSISDIR") then nsisroot = v
		elseif string.find(k, "SEVENZIP_PATH") then ziproot = v
		elseif string.find(k, "MSVSBIN") then vsbinpath = v
		elseif string.find(k, "DDKINCDIR") then rosinc = v
		elseif string.find(k, "MINGWx64BASE") then mingw64base = v
		end
	end
end

f:close()

function param_action(dialog, param_index)
	if param_index == -2 then
		-- dialog initialization
		icon = iup.image {
		    { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
		    { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
		    { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
		    { 1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,1 },
		    { 1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1 },
		    { 1,1,1,2,1,1,1,2,2,1,1,1,1,1,1,1 },
		    { 1,1,1,2,2,1,1,1,2,2,1,1,1,1,1,1 },
		    { 1,1,1,1,2,2,1,1,2,2,1,1,1,1,1,1 },
		    { 1,1,1,1,1,2,2,2,2,2,1,1,1,1,1,1 },
		    { 1,1,1,1,1,1,2,2,2,2,2,1,1,1,1,1 },
		    { 1,1,1,1,1,1,1,1,1,2,2,2,1,1,1,1 },
		    { 1,1,1,1,1,1,1,1,1,1,2,2,2,1,1,1 },
		    { 1,1,1,1,1,1,1,1,1,1,1,2,2,1,1,1 },
		    { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
		    { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
		    { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 }
			; colors = { "255 255 0", "255 0 0" }
		}
		dialog.icon = icon
		dialog.size = "400x218"
	end
	return 1
end

ret, udver, winxver, mingwbase, ddkbase, netpath, nsisroot, ziproot, vsbinpath, rosinc, mingw64base, apply_patch = 
	iup.GetParam("UltraDefrag build configurator",param_action,
		"UltraDefrag version: %s\n"..
		"ZenWINX version: %s\n"..
		"MinGW base path: %s\n"..
		"Windows DDK base path: %s\n"..
		".NET runtime path: %s\n"..
		"NSIS root path: %s\n"..
		"7-Zip root path: %s\n"..
		"Visual Studio bin path: %s\n"..
		"ReactOS include path: %s\n"..
		"MinGW x64 base path: %s\n"..
		"Apply patch to MinGW: %b[No,Yes]\n",
		udver, winxver, mingwbase, ddkbase, netpath, nsisroot, ziproot, vsbinpath, rosinc, mingw64base, apply_patch
		)
if ret == 1 then
	-- save options
	f = assert(io.open("./SETVARS.CMD","w"))
	f:write("@echo off\necho Set common environment variables...\n")
	for i, j, k in string.gmatch(udver,"(%d+).(%d+).(%d+)") do
		f:write("set VERSION=", i, ",", j, ",", k, ",0\n")
		f:write("set VERSION2=\"", i, ", ", j, ", ", k, ", 0\\0\"\n")
	end
	for i, j, k in string.gmatch(winxver,"(%d+).(%d+).(%d+)") do
		f:write("set ZENWINX_VERSION=", i, ",", j, ",", k, ",0\n")
		f:write("set ZENWINX_VERSION2=\"", i, ", ", j, ", ", k, ", 0\\0\"\n")
	end
	f:write("set ULTRADFGVER=", udver, "\n")
	f:write("set ZENWINXVER=", winxver, "\n")
	f:write("set WINDDKBASE=", ddkbase, "\n")
	f:write("set MINGWBASE=", mingwbase, "\n")
	f:write("set MINGWx64BASE=", mingw64base, "\n")
	f:write("set NETRUNTIMEPATH=", netpath, "\n")
	f:write("set NSISDIR=", nsisroot, "\n")
	f:write("set SEVENZIP_PATH=", ziproot, "\n")
	f:write("set MSVSBIN=", vsbinpath, "\n")
	f:write("set DDKINCDIR=", rosinc, "\n")
	f:close()
	print("SETVARS.CMD script was successfully updated.")
	if apply_patch == 1 then
		print("Apply MinGW patch option was selected.")
		if os.execute("cmd.exe /C mingw_patch.cmd " .. mingwbase .. " " .. mingw64base) ~= 0 then
			error("Cannot apply patch to MinGW!")
		end
	end
end

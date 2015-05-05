#!/usr/local/bin/lua
--[[
  mkmod.lua - produces makefiles for various compilers from a single *.build file.
  Copyright (c) 2007-2013 Dmitri Arkhangelski (dmitriar@gmail.com).

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

--[[
  Synopsis: lua mkmod.lua <filename>
    If processing completes successfully, binary modules are placed
    in ../../bin directory; *.lib or *.a files - in  ../../lib directory.
  Note: BUILD_ENV environment variable must be set before the script run.

  Notes for C programmers: 
    1. the first element of each array has index 1.
    2. only nil and false values are false, all other including 0 are true

  If some library exports functions of __stdcall calling convention
  a *mingw.def file must be presented; it must contain names 
  decorated by at signs and number of bytes needed to pass all 
  the parameters on a 32-bit machine.
--]]

nativedll = 0
src, rc, includes, libs, adlibs = {}, {}, {}, {}, {}
files, resources, headers, inc = {}, {}, {}, {}
cpp_files = 0

-- files which names contain these patterns will be
-- included as dependencies to MinGW makefiles
rsrc_patterns = { "%.ico$", "%.bmp$", "%.manifest$" }

ddk_cmd = "build.exe"
mingw_cmd = "mingw32-make --no-builtin-rules -f Makefile.mingw"
sdk_cmd = "nmake.exe /NOLOGO /f Makefile.winsdk"

-- common subroutines
function copy(src, dst)
    if os.execute("cmd.exe /C copy /Y " .. src .. " " .. dst) ~= 0 then
        error("Cannot copy from " .. src .. " to " .. dst .. "!");
    end
end

function build_list_of_headers()
    local f, i, j, dir, n, h
    if not includes[1] then return end
    -- include headers listed in 'headers' file,
    -- but only these locating in directories
    -- listed in 'includes' table
    n = 0 -- total number of headers
    f = assert(io.open("headers","rt"))
    for line in f:lines() do
        i, j, dir = string.find(line,"^.*\\(.-)\\.-$")
        headers[line] = dir
        n = n + 1
    end
    f:close()
    if n == 0 then
        error("The \"headers\" file seems to be empty!")
    end
    for i, dir in ipairs(includes) do
        n = 0 -- number of headers found for current directory
        for h in pairs(headers) do
            if headers[h] == dir then
                table.insert(inc,h)
                n = n + 1
            end
        end
        if n == 0 then
            error("No headers found in " .. dir .. "!")
            return
        end
    end
    assert(inc[1],"No acceptable headers found in \"headers\" file!")
end

-- WinDDK backend
makefile_contents = [[
#
# DO NOT EDIT THIS FILE!!!  Edit .\sources if you want to add a new source
# file to this component.  This file merely indirects to the real make file
# that is shared by all the driver components of the Windows NT DDK
#

!INCLUDE $(NTMAKEENV)\makefile.def

]]
function produce_ddk_makefile()
    local t, umt

    local f = assert(io.open(".\\makefile","w"))
    f:write(makefile_contents)
    f:close()

    f = assert(io.open(".\\sources","w"))

    f:write("TARGETNAME=", name, "\n")
    f:write("TARGETPATH=obj\n")

    -- f:write("AMD64_OPTIMIZATION=/Od\n")
    -- f:write("IA64_OPTIMIZATION=/Od\n\n")
    -- f:write("386_OPTIMIZATION=/Ot /Og\n") -- never tested!!!

    if     target_type == "console" then t = "PROGRAM"; umt = "console"
    elseif target_type == "gui"     then t = "PROGRAM"; umt = "windows"
    elseif target_type == "native"  then t = "PROGRAM"; umt = "nt"
    elseif target_type == "driver"  then t = "DRIVER"
    elseif target_type == "dll"     then t = "DYNLINK"; umt = "console"
    else   error("Unknown target type: " .. target_type .. "!")
    end
    
    f:write("TARGETTYPE=", t, "\n\n")
    if target_type == "dll" then
        f:write("DLLDEF=", deffile, "\n\n")
    end

    -- use /FA switch to generate assembly listings
    -- f:write("USER_C_FLAGS=/DUSE_WINDDK /FA\n\n")
    f:write("USER_C_FLAGS=/DUSE_WINDDK\n\n")
    
    if target_type == "console" or target_type == "gui" then
        f:write("CFLAGS=\$(CFLAGS) /MT\n\n")
    end

    f:write("SOURCES=")
    for i, v in ipairs(src) do f:write(v, " ") end
    for i, v in ipairs(rc) do f:write(v, " ") end
    f:write("\n")
    
    if target_type == "console" or target_type == "gui" then 
        f:write("USE_MSVCRT=1\n")
    end
    if target_type == "native" then
        f:write("USE_NTDLL=1\n")
        f:write("MINWIN_SDK_LIB_PATH=\$(SDK_LIB_PATH)\n\n")
    end
    if target_type == "dll" then
        if nativedll == 1 then
            f:write("USE_NTDLL=1\n")
        else
            f:write("USE_MSVCRT=1\n")
        end
    end
    f:write("\n")
    
    if target_type == "native" or target_type == "dll" then
        f:write("# very important for nt 4.0 ")
        f:write("(without RtlUnhandledExceptionFilter function)\n")
        f:write("BUFFER_OVERFLOW_CHECKS=0\n\n")
    end

    f:write("LINKLIBS=")
    for i, v in ipairs(libs) do
        if v ~= "msvcrt" then
            f:write("\$(DDK_LIB_PATH)\\", v, ".lib ")
        end
    end
    for i, v in ipairs(adlibs) do f:write(v, ".lib ") end
    f:write("\n\n")
    
    if target_type ~= "driver" then f:write("UMTYPE=", umt, "\n") end
    if target_type ==  "console" or target_type == "gui" then
        f:write("UMENTRY=", umentry, "\n")
    end
    if target_type == "dll" then
        f:write("DLLBASE=", baseaddr, "\nDLLENTRY=DllMain\n")
    end
    f:close()
end

-- WinSDK backend
function produce_sdk_makefile()
    local s, upname
    local arch = "i386"
    local outpath = "..\\..\\bin\\"
    local libpath = "..\\..\\lib\\"

    local f = assert(io.open(".\\Makefile.winsdk","w"))
    
    if os.getenv("AMD64") then
        arch = "amd64"
        outpath = outpath .. "amd64\\"
        libpath = libpath .. "amd64\\"
    elseif os.getenv("IA64") then
        arch = "ia64"
        outpath = outpath .. "ia64\\"
        libpath = libpath .. "ia64\\"
    end

    f:write("ALL : \"", target_name, "\"\n\n")
    f:write("CPP_PROJ=/nologo /W3 /D \"WIN32\" /D \"NDEBUG\" /D \"_MBCS\" ")
    f:write("/D \"USE_WINSDK\" /D \"_CRT_SECURE_NO_WARNINGS\" /GS- /arch:SSE2 ")
    if arch == "ia64" then
        -- optimization for ia64 is not available:
        -- the compiler stucks with the message:
        -- error loading dll 'sched.dll': dll not found
        f:write("/Od ")
    else
        f:write("/O2 ")
    end
    
    -- the following check eliminates need of msvcr90.dll library
    if nativedll == 0 then
        f:write("/MT ")
    end
    
    if target_type == "console" then
        f:write("/D \"_CONSOLE\" ")
        s = "console"
    elseif target_type == "gui" then
        f:write("/D \"_WINDOWS\" ")
        s = "windows"
    elseif target_type == "dll" then
        upname = string.upper(name) .. "_EXPORTS"
        f:write("/D \"_CONSOLE\" /D \"_USRDLL\" /D \"", upname, "\" ")
        s = "console"
    elseif target_type == "native" then
        s = "native"
    else error("Unknown target type: " .. target_type .. "!")
    end
    
    f:write(" /c \n")
    f:write("RSC_PROJ=/l 0x409 /d \"NDEBUG\" \n")
    
    f:write("LINK32_FLAGS=")
    for i, v in ipairs(libs) do
        if nativedll ~= 0 or v ~= "msvcrt" then
            f:write(v, ".lib ")
        end
    end
    for i, v in ipairs(adlibs) do
        f:write(v, ".lib ")
    end
    if nativedll == 0 and target_type ~= "native" then
        -- DLL for console/gui environment
        f:write("/nologo /incremental:no ")
    else
        f:write("/nologo /incremental:no /nodefaultlib ")
    end
    if arch == "i386" then
        f:write("/machine:I386 ")
    elseif arch == "amd64" then
        f:write("/machine:AMD64 ")
    elseif arch == "ia64" then
        f:write("/machine:IA64 ")
    end
    f:write("/subsystem:", s, " ")
    if target_type == "dll" then
        if nativedll == 0 then
            f:write("/dll ")
        else
            f:write("/entry:\"DllMain\" /dll ")
        end
        f:write("/def:", deffile, " ")
        f:write("/implib:", libpath, name, ".lib ")
    elseif target_type == "native" then
        f:write("/entry:\"NtProcessStartup\" ")
    end
    if target_type == "gui" and umentry == "main" then
        -- let GUI apps use main() whenever they need
        f:write(" /entry:\"mainCRTStartup\" ")
    end
    f:write(" /out:\"", outpath, target_name, "\" \n\n")
    
    f:write("CPP=cl.exe\nRSC=rc.exe\nLINK32=link.exe\n\n")

    build_list_of_headers()
    f:write("header_files: ")
    for i, v in ipairs(inc) do
        f:write("\\\n", v, " ")
    end
    f:write("\n\n")

    f:write("resource_files: ")
    for i, v in ipairs(resources) do
        f:write("\\\n", v, " ")
    end
    f:write("\n\n")
    
    for i, v in ipairs(src) do
        outfile = string.gsub(v,"%.c(.-)$","%-" .. arch .. "%.obj")
        f:write(outfile, ": ", v, " header_files\n")
        f:write("    \$(CPP) \$(CPP_PROJ) /Fo", outfile, " ", v, "\n\n")
    end

    f:write("LINK32_OBJS=")
    for i, v in ipairs(src) do
        f:write(string.gsub(v,"%.c(.-)$","%-" .. arch .. "%.obj"), " ")
    end
    for i, v in ipairs(rc) do
        f:write(string.gsub(v,"%.rc","%-" .. arch .. "%.res"), " ")
    end
    f:write("\n\n")
    
    if target_type == "dll" then
        f:write("DEF_FILE=", deffile, "\n\n")
        f:write("\"", target_name, "\" : \$(DEF_FILE) \$(LINK32_OBJS)\n")
        f:write("    \$(LINK32) \$(LINK32_FLAGS) \$(LINK32_OBJS)\n\n")
    else
        f:write("\"", target_name, "\" : \$(LINK32_OBJS)\n")
        f:write("    \$(LINK32) \$(LINK32_FLAGS) \$(LINK32_OBJS)\n\n")
    end

    for i, v in ipairs(rc) do
        outfile = string.gsub(v,"%.rc","%-" .. arch .. "%.res")
        f:write(outfile, ": ", v, " header_files resource_files\n")
        f:write("    \$(RSC) \$(RSC_PROJ) /Fo", outfile, " ", v, "\n\n")
    end

    f:close()
end

-- MinGW backend
mingw_rules = [[
define build_target
@echo Linking...
@$(CC) -o $(TARGET) $(SRC_OBJS) $(RSRC_OBJS) $(LIB_DIRS) $(LIBS) $(LDFLAGS)
endef

define compile_resource
@echo Compiling $<
@$(WINDRES) $(RCFLAGS) $(RC_PREPROC) $(RC_INCLUDE_DIRS) -O COFF -i "$<" -o "$@"
endef

define compile_source
@echo Compiling $<
@$(CC) $(CFLAGS) $(C_PREPROC) $(C_INCLUDE_DIRS) -c "$<" -o "$@"
endef

]]

function produce_mingw_makefile()
    local adlibs_libs, adlibs_paths, path, lib
    local outpath = "..\\..\\bin\\"
    local libpath = "..\\..\\lib\\"

    local f = assert(io.open(".\\Makefile.mingw","w"))

    if os.getenv("BUILD_ENV") == "mingw_x64" then
        outpath = outpath .. "amd64\\"
        libpath = libpath .. "amd64\\"
    end

    if cpp_files ~= 0 then
        f:write("PROJECT = ", name, "\nCC = g++.exe\n\n")
    else
        f:write("PROJECT = ", name, "\nCC = gcc.exe\n\n")
    end
    f:write("WINDRES = \"\$(COMPILER_BIN)windres.exe\"\n\n")

    f:write("TARGET = ", outpath, target_name, "\n")
    if os.getenv("BUILD_ENV") == "mingw_x64" then
        f:write("CFLAGS = -pipe  -Wall -g0 -O2 -m64\n")
    else
        f:write("CFLAGS = -pipe  -Wall -g0 -O2\n")
    end
    
    f:write("RCFLAGS = \n")
    f:write("C_INCLUDE_DIRS = \n")
    f:write("C_PREPROC = \n")
    f:write("RC_INCLUDE_DIRS = \n")
    f:write("RC_PREPROC = \n")
    
    if target_type == "console" then
        f:write("LDFLAGS = -pipe -Wl,--strip-all\n")
    elseif target_type == "gui" then
        f:write("LDFLAGS = -pipe -mwindows -Wl,--strip-all\n")
    elseif target_type == "native" then
        f:write("LDFLAGS = -pipe -nostartfiles -nodefaultlibs ")
        f:write("-Wl,--entry,_NtProcessStartup\@4,--subsystem,native,--strip-all\n")
    elseif target_type == "driver" then
        f:write("LDFLAGS = -pipe -nostartfiles -nodefaultlibs ")
        f:write(mingw_deffile .. " -Wl,--entry,_DriverEntry\@8,")
        f:write("--subsystem,native,--image-base,0x10000,-shared,--strip-all\n")
    elseif target_type == "dll" then
        f:write("LDFLAGS = -pipe -shared -nostartfiles ")
        f:write("-nodefaultlibs ", mingw_deffile, " -Wl,--kill-at,")
        f:write("--entry,_DllMain\@12,--strip-all\n")
    else error("Unknown target type: " .. target_type .. "!")
    end

    f:write("LIBS = ")
    for i, v in ipairs(libs) do
        f:write("-l", v, " ")
    end

    adlibs_libs = {}
    rev_adlibs_libs = {}
    adlibs_paths = {}
    for i, v in ipairs(adlibs) do
        i, j, path, lib = string.find(v,"^(.*)\\(.-)$")
        if path == nil or lib == nil then
            table.insert(adlibs_libs,v)
        else
            table.insert(adlibs_libs,lib)
            table.insert(adlibs_paths,path)
        end
    end
    for i, v in ipairs(adlibs_libs) do
        f:write("-l", v, " ")
        table.insert(rev_adlibs_libs,1,v)
    end
    -- include libraries in reverse order
    -- needed to link with static additional libraries
    for i, v in ipairs(rev_adlibs_libs) do
        f:write("-l", v, " ")
    end
    -- include standard libraries again
    -- needed to link with static additional libraries
    for i, v in ipairs(libs) do
        f:write("-l", v, " ")
    end
    f:write("\nLIB_DIRS = ")
    for i, v in ipairs(adlibs_paths) do
        f:write("-L\"", v, "\" ")
    end
    f:write("\n\n")
    
    f:write("SRC_OBJS = ")
    for i, v in ipairs(src) do
        f:write(string.gsub(v,"%.c(.-)$","%.o"), " ")
    end

    f:write("\n\nRSRC_OBJS = ")
    for i, v in ipairs(rc) do
        f:write(string.gsub(v,"%.rc","%.res"), " ")
    end
    f:write("\n\n")

    f:write(mingw_rules)
    if target_type == "dll" then
        f:write("define build_library\n")
        f:write("\@echo ---------- build the lib\$(PROJECT).dll.a library ----------\n")
        f:write("\@dlltool -k --output-lib ", libpath, "lib\$(PROJECT).dll.a --def ")
        f:write(mingw_deffile, "\n")
        f:write("endef\n\n")
    end
    f:write("define print_header\n")
    f:write("\@echo ---------- Configuration: ", name, " - Release ----------\n")
    f:write("endef\n\n")
    
    f:write("\$(TARGET): \$(RSRC_OBJS) \$(SRC_OBJS)\n")
    f:write("\t\$(print_header)\n")
    f:write("\t\$(build_target)\n")
    if target_type == "dll" then
        -- produce interface library from .def file
        f:write("\t\$(build_library)\n")
    end
    f:write("\n")
    
    build_list_of_headers()
    
    for i, v in ipairs(src) do
        f:write(string.gsub(v,"%.c(.-)$","%.o"), ": ", v, " ")
        for i, v in ipairs(inc) do
            f:write("\\\n", v, " ")
        end
        f:write("\n\t\$(compile_source)\n\n")
    end

    for i, v in ipairs(rc) do
        f:write(string.gsub(v,"%.rc","%.res"), ": ", v, " ")
        for i, v in ipairs(inc) do
            f:write("\\\n", v, " ")
        end
        for i, v in ipairs(resources) do
            f:write("\\\n", v, " ")
        end
        f:write("\n\t\$(compile_resource)\n\n")
    end

    f:close()
end

-- frontend
input_filename = arg[1]
assert(input_filename,"File name must be specified!")
print(input_filename .. " Preparing the makefile generation...\n")

dofile(input_filename)

if os.execute("cmd.exe /C dir /S /B >project_files") ~= 0 then
    error("Cannot get directory listing!")
end
f = assert(io.open("project_files","rt"))
files = {}
for line in f:lines() do
    table.insert(files,line)
end
f:close()
os.execute("cmd.exe /C del /Q project_files")
assert(files[1],"No project files found!")

-- search for .def files
deffile, mingw_deffile = "", ""
for i, v in ipairs(files) do
    local i, j, def
    i, j, def = string.find(v,"^.*\\(.-)$")
    if not def then def = v end
    if string.find(def,"%.def$") then
        if string.find(def,"mingw%.def$") then
            mingw_deffile = def
        else
            deffile = def
        end
    end
end
if deffile == "" then deffile = mingw_deffile end
if mingw_deffile == "" then mingw_deffile = deffile end

-- setup src, rc and resources tables
for i, v in ipairs(files) do
    local i, j, name, p
    i, j, name = string.find(v,"^.*\\(.-)$")
    if not name then name = v end
    if string.find(name,"%.c$") then
        table.insert(src,name)
    elseif string.find(name,"%.cpp$") then
        cpp_files = cpp_files + 1
        table.insert(src,name)
    elseif string.find(name,"%.rc$") then
        table.insert(rc,name)
    end
    for i, p in ipairs(rsrc_patterns) do
        if string.find(v,p) then
            table.insert(resources,v)
            break
        end
    end
end

if target_type == "console" or target_type == "gui" or target_type == "native" then
    target_ext = "exe"
elseif target_type == "dll" then
    target_ext = "dll"
elseif target_type == "driver" then
    target_ext = "sys"
else
    error("Unknown target type: " .. target_type .. "!")
end
target_name = name .. "." .. target_ext

if os.getenv("BUILD_ENV") == "winddk" then
    print(input_filename .. " winddk build performing...\n")
    produce_ddk_makefile()
    objpath = "objfre_wxp_x86\\i386\\"
    outpath = "..\\..\\bin\\"
    libpath = "..\\..\\lib\\"
    if os.getenv("AMD64") then
        objpath = "objfre_wnet_amd64\\amd64\\"
        outpath = "..\\..\\bin\\amd64"
        libpath = "..\\..\\lib\\amd64"
    elseif os.getenv("IA64") then
        objpath = "objfre_wnet_ia64\\ia64\\"
        outpath = "..\\..\\bin\\ia64"
        libpath = "..\\..\\lib\\ia64"
    end
    if os.execute(ddk_cmd) ~= 0 then
        error("Cannot build the target!")
    end
    copy(objpath .. target_name,outpath)
    if target_type == "dll" then
        copy(objpath .. name .. ".lib",libpath)
    end
elseif os.getenv("BUILD_ENV") == "winsdk" then
    -- SDK is not supported because GUI cannot safely
    -- free memory allocated by WGX library when the /MT
    -- compiler switch is used for their compilation
    error("Windows SDK is not supported!")
    if target_type == "driver" then
        error("Driver compilation is not supported by Windows SDK!")
    else
        print(input_filename .. " windows sdk build performing...\n")
        produce_sdk_makefile()
        if os.execute(sdk_cmd) ~= 0 then
            error("Cannot build the target!")
        end
    end
elseif os.getenv("BUILD_ENV") == "mingw" then
    print(input_filename .. " mingw build performing...\n")
    produce_mingw_makefile()
    if os.execute(mingw_cmd) ~= 0 then
        error("Cannot build the target!")
    end
elseif os.getenv("BUILD_ENV") == "mingw_x64" then
    -- NOTE: MinGW x64 compiler currently generates wrong
    -- code, therefore we cannot use it for real purposes.
    print(input_filename .. " mingw x64 build performing...\n")
    produce_mingw_makefile()
    if os.execute(mingw_cmd) ~= 0 then
        error("Cannot build the target!")
    end
else
    error("\%BUILD_ENV\% has wrong value: " .. os.getenv("BUILD_ENV") .. "!")
end

print(input_filename .. " " .. os.getenv("BUILD_ENV") .. " build was successful.\n")

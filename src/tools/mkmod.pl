#!/usr/bin/perl
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# mkmod.pl - produce makefiles for various compilers from one *.build file.
# Copyright (c) 2007 by Dmitri Arkhangelski (dmitriar@gmail.com).
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# SYNOPSIS:    perl mkmod.pl <filename>
# If processing was sucessful, binary modules will be placed in ../../bin directory;
# *.lib or *.a files - in  ../../lib directory.

use strict;
use Switch;
#use Cwd;
use File::Copy;

my %opts = (
	'NAME' => ' ',
	'TYPE' => ' ',
	'SRC' => ' ',
	'RC' => ' ',
	'LIBS' => ' ',
	'ADLIBS' => ' ',
	'DEFFILE' => ' ',
	'BASEADDR' => ' '
	);

my @src_files;
my @rc_files;
my @libs;
my @adlibs;

my $input_filename;
my ($target_type,$target_ext,$target_name,$nt4target_name);
my $arch;
my @ddk_cmd = ("build.exe"," ");
my @msvc_cmd = ("nmake.exe","/NOLOGO","/A","/f"," ");
my @mingw_cmd = ("mingw32-make","--always-make","-f","Makefile.mingw");

# frontend
$input_filename = $ARGV[0];
if($input_filename eq undef){
	die("Filename must be specified!");
}
print "$input_filename Preparing to makefile generation...\n";
get_opts($input_filename);
$target_type = $opts{'TYPE'};
switch($target_type){
	case 'console' {$target_ext = 'exe';}
	case 'gui'     {$target_ext = 'exe';}
	case 'dll'     {$target_ext = 'dll';}
	case 'driver'  {$target_ext = 'sys';}
	case 'native'  {$target_ext = 'exe';}
	else           {die("Unknown target type: $target_type!");}
}
$target_name = "$opts{'NAME'}.$target_ext";
$nt4target_name = "$opts{'NAME'}_nt4.$target_ext";
switch($ENV{'BUILD_ENV'}){
	case 'winddk' {
		produce_ddk_makefile() if obsolete($input_filename,"sources");
		print "$input_filename winddk build performing...\n";
		$arch = 'i386';
		if($ENV{'AMD64'} ne undef){
			$arch = 'amd64';
		}
		if($ENV{'IA64'} ne undef){
			$arch = 'ia64';
		}
		switch($target_type){
			case 'dll' {
				system(@ddk_cmd) == 0 or die("Can't build the target!");
				if($arch eq 'i386'){
					copy("objfre_wnet_x86\\i386\\$target_name","..\\..\\bin\\$target_name")
						or die("Can't copy file $target_name: $!");
					copy("objfre_wnet_x86\\i386\\$opts{'NAME'}.lib","..\\..\\lib\\$opts{'NAME'}.lib")
						or die("Can't copy file $opts{'NAME'}.lib: $!");
				} else {
					copy("objfre_wnet_$arch\\$arch\\$target_name","..\\..\\bin\\$arch\\$target_name")
						or die("Can't copy file $target_name: $!");
					copy("objfre_wnet_$arch\\$arch\\$opts{'NAME'}.lib",
						 "..\\..\\lib\\$arch\\$opts{'NAME'}.lib")
						or die("Can't copy file $opts{'NAME'}.lib: $!");
				}
			}
			case 'driver' {
				$ddk_cmd[@ddk_cmd - 1] = "-c";
				if($arch eq 'i386'){
					$ENV{'NT4_TARGET'} = "true";
					system(@ddk_cmd) == 0 or die("Can't build the target!");
					copy("objfre_wnet_x86\\i386\\$nt4target_name","..\\..\\bin\\$nt4target_name")
						or die("Can't copy file $target_name: $!");
				}
				$ENV{'NT4_TARGET'} = "false";
				system(@ddk_cmd) == 0 or die("Can't build the target!");
				if($arch eq 'i386'){
					copy("objfre_wnet_x86\\i386\\$target_name","..\\..\\bin\\$target_name")
						or die("Can't copy file $target_name: $!");
				} else {
					copy("objfre_wnet_$arch\\$arch\\$target_name","..\\..\\bin\\$arch\\$target_name")
						or die("Can't copy file $target_name: $!");
				}
			}
			else {
				system(@ddk_cmd) == 0 or die("Can't build the target!");
				if($arch eq 'i386'){
					copy("objfre_wnet_x86\\i386\\$target_name","..\\..\\bin\\$target_name")
						or die("Can't copy file $target_name: $!");
				} else {
					copy("objfre_wnet_$arch\\$arch\\$target_name","..\\..\\bin\\$arch\\$target_name")
						or die("Can't copy file $target_name: $!");
				}
			}
		}
	}
	case 'msvc' {
		produce_msvc_makefile() if obsolete($input_filename,"$opts{'NAME'}.mak");
		print "$input_filename msvc build performing...\n";
		$msvc_cmd[@msvc_cmd - 1] = "$opts{'NAME'}.mak";
		switch($target_type){
			case 'dll' {
				system(@msvc_cmd) == 0 or die("Can't build the target!");
				copy("$target_name","..\\..\\bin\\$target_name")
					or die("Can't copy file $target_name: $!");
				copy("$opts{'NAME'}.lib","..\\..\\lib\\$opts{'NAME'}.lib")
					or die("Can't copy file $opts{'NAME'}.lib: $!");
			}
			case 'driver' {
				$ENV{'NT4_TARGET'} = "true";
				system(@msvc_cmd) == 0 or die("Can't build the target!");
				copy("$nt4target_name","..\\..\\bin\\$nt4target_name")
					or die("Can't copy file $target_name: $!");
				$ENV{'NT4_TARGET'} = "false";
				system(@msvc_cmd) == 0 or die("Can't build the target!");
				copy("$target_name","..\\..\\bin\\$target_name")
					or die("Can't copy file $target_name: $!");
			}
			else {
				system(@msvc_cmd) == 0 or die("Can't build the target!");
				copy("$target_name","..\\..\\bin\\$target_name")
					or die("Can't copy file $target_name: $!");
			}
		}
	}
	case 'mingw' {
		produce_mingw_makefile() if obsolete($input_filename,"Makefile.mingw");
		print "$input_filename mingw build performing...\n";
		switch($target_type){
			case 'dll' {
				system(@mingw_cmd) == 0 or die("Can't build the target!");
				copy("$target_name","..\\..\\bin\\$target_name")
					or die("Can't copy file $target_name: $!");
				copy("lib$target_name.a","..\\..\\lib\\lib$target_name.a")
					or die("Can't copy file lib$target_name.a: $!");
			}
			case 'driver' {
				$ENV{'NT4_TARGET'} = "true";
				system(@mingw_cmd) == 0 or die("Can't build the target!");
				copy("$nt4target_name","..\\..\\bin\\$nt4target_name")
					or die("Can't copy file $target_name: $!");
				$ENV{'NT4_TARGET'} = "false";
				system(@mingw_cmd) == 0 or die("Can't build the target!");
				copy("$target_name","..\\..\\bin\\$target_name")
					or die("Can't copy file $target_name: $!");
			}
			else {
				system(@mingw_cmd) == 0 or die("Can't build the target!");
				copy("$target_name","..\\..\\bin\\$target_name")
					or die("Can't copy file $target_name: $!");
			}
		}
	}
	else {die("\%BUILD_ENV\% has wrong value: $ENV{'BUILD_ENV'}!");}
}
print "$input_filename $ENV{'BUILD_ENV'} build was successful.\n";

# frontend subroutines
sub get_opts {
	my $path = $_[0];
	my @fields;
	
	open(IN,$path) or die("Can\'t open $path: $!");
	foreach (<IN>){
		chomp($_);
		@fields = split(/=/);
		if(@fields == 2){
			if($opts{$fields[0]} ne undef){
				$opts{$fields[0]} = $fields[1];
			}
		}
	}
	close IN;
	if($opts{'SRC'} ne ' '){@src_files = split(/;/,$opts{'SRC'});}
	if($opts{'RC'} ne ' '){@rc_files = split(/;/,$opts{'RC'});}
	if($opts{'LIBS'} ne ' '){@libs = split(/;/,$opts{'LIBS'});}
	if($opts{'ADLIBS'} ne ' '){@adlibs = split(/;/,$opts{'ADLIBS'});}
}

sub obsolete {
	my ($src,$dst) = @_;
	my $src_mtime;

	my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
       $atime,$mtime,$ctime,$blksize,$blocks) = stat($src);
	$src_mtime = $mtime;
	($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
       $atime,$mtime,$ctime,$blksize,$blocks) = stat($dst);
 	return ($mtime > $src_mtime) ? 1 : 0;
}

# WinDDK backend
sub produce_ddk_makefile {
	my ($type,$t,$umt,$e);

	open(OUT,"> makefile") 
		or die("Can\'t open makefile: $!");
	print OUT "#\n";
	print OUT "# DO NOT EDIT THIS FILE!!!  Edit .\\sources. ";
	print OUT "if you want to add a new source\n";
	print OUT "# file to this component.  This file merely indirects ";
	print OUT "to the real make file\n";
	print OUT "# that is shared by all the driver components ";
	print OUT "of the Windows NT DDK\n";
	print OUT "#\n";
	print OUT "\n";
	print OUT "!INCLUDE \$(NTMAKEENV)\\makefile.def\n";

	close OUT;

	open(OUT,"> sources") 
		or die("Can\'t open sources: $!");
	$type = $opts{'TYPE'};
	if($type ne 'driver'){
		print OUT "TARGETNAME=$opts{'NAME'}\n";
	} else {
		print OUT "!IF \"\$(NT4_TARGET)\" == \"true\"\n";
		print OUT "TARGETNAME=$opts{'NAME'}_nt4\n";
		print OUT "!ELSE\n";
		print OUT "TARGETNAME=$opts{'NAME'}\n";
		print OUT "!ENDIF\n";
	}
	print OUT "TARGETPATH=obj\n";
	switch($type) {
		case 'console' {$t = 'PROGRAM';$umt = 'console';$e = 'wmain';}
		case 'gui'     {$t = 'PROGRAM';$umt = 'windows';$e = 'winmain';}
		case 'native'  {$t = 'PROGRAM';$umt = 'nt';}
		case 'driver'  {$t = 'DRIVER';}
		case 'dll'     {$t = 'DYNLINK';$umt = 'console';}
		else           {die("Unknown target type: $type!");}
	}
	print OUT "TARGETTYPE=$t\n\n";
	if($type eq 'dll'){
		print OUT "DLLDEF=$opts{'DEFFILE'}\n\n";
	}
	if($type ne 'driver'){
		print OUT "USER_C_FLAGS=/DUSE_WINDDK\n\n";
	} else {
		print OUT "!IF \"\$(NT4_TARGET)\" == \"true\"\n";
		print OUT "USER_C_FLAGS=/DUSE_WINDDK /DNT4_TARGET\n";
		print OUT "RCOPTIONS=/d NT4_TARGET\n";
		print OUT "!ELSE\n";
		print OUT "USER_C_FLAGS=/DUSE_WINDDK\n";
		print OUT "!ENDIF\n\n";
	}
	if($type eq 'console' || $type eq 'gui'){
		print OUT "CFLAGS=\$(CFLAGS) /MT\n\n";
	}
	print OUT "SOURCES=";
	foreach (@src_files){
		print OUT "$_ ";
	}
	foreach (@rc_files){
		print OUT "$_ ";
	}
	print OUT "\n";
	switch($type) {
		case 'console' {print OUT "USE_MSVCRT=1\n";}
		case 'gui'     {print OUT "USE_MSVCRT=1\n";}
		case 'native'  {print OUT "USE_NTDLL=1\n";}
		case 'dll'     {print OUT "USE_NTDLL=1\n";}
	}
	print OUT "\n";
	if($type eq 'native' || $type eq 'dll'){
		print OUT "# very important for nt 4.0 ";
		print OUT "(without RtlUnhandledExceptionFilter function)\n";
		print OUT "BUFFER_OVERFLOW_CHECKS=0\n\n";
	}
	print OUT "LINKLIBS=";
	foreach (@libs){
		if($_ ne 'msvcrt'){
			print OUT "\$(DDK_LIB_PATH)\\$_.lib ";
		}
	}
	foreach (@adlibs){
		print OUT "$_.lib ";
	}
	print OUT "\n\n";
	if($type ne 'driver'){
		print OUT "UMTYPE=$umt\n";
	}
	if($type eq 'console' || $type eq 'gui'){
		print OUT "UMENTRY=$e\n";
	}
	if($type eq 'dll'){
		print OUT "DLLBASE=$opts{'BASEADDR'}\nDLLENTRY=DllMain\n";
	}
	close OUT;
}

# MS Visual Studio backend
sub produce_msvc_makefile {
	my ($type,$s,$ext,$upname);
	my ($cl_flags,$rsc_flags,$link_flags);
	my $x;

	open(OUT,"> $opts{'NAME'}.mak") 
		or die("Can\'t open $opts{'NAME'}.mak: $!");
	#print OUT "!IF \"\$(OS)\" == \"Windows_NT\"\n";
	#print OUT "NULL=\n";
	#print OUT "!ELSE\n";
	#print OUT "NULL=nul\n";
	#print OUT "!ENDIF\n\n";
	# OUTDIR and INTDIR parameters will be replaced with current directory
	$cl_flags = "CPP_PROJ=/nologo /W3 /O2 /D \"WIN32\" /D \"NDEBUG\" /D \"_MBCS\" ";
	$type = $opts{'TYPE'};
	$upname = $opts{'NAME'};
	$upname =~ tr/a-z/A-Z/;
	$upname = $upname."_EXPORTS";
	switch($type){
		case 'console' {
			$cl_flags = $cl_flags."/D \"_CONSOLE\" ";
			$s = 'console'; $ext = 'exe';
		}
		case 'gui'     {
			$cl_flags = $cl_flags."/D \"_WINDOWS\" ";
			$s = 'windows'; $ext = 'exe';
		}
		case 'dll'     {
			$cl_flags = $cl_flags."/D \"_CONSOLE\" /D \"_USRDLL\" /D \"$upname\" "; 
			$s = 'console'; $ext = 'dll';
		}
		case 'driver'  {
			$cl_flags = $cl_flags."/I \"\$(DDKINCDIR)\" /I \"\$(DDKINCDIR)\\ddk\" ";
			$s = 'native'; $ext = 'sys';
		}
		case 'native'  {
			$s = 'native'; $ext = 'exe';
		}
		else           {die("Unknown target type: $type!");}
	}
	if($type ne 'driver'){
		print OUT "ALL : \"$opts{'NAME'}.$ext\"\n\n";
	} else {
		print OUT "!IF \"\$(NT4_TARGET)\" == \"true\"\n";
		print OUT "ALL : \"$opts{'NAME'}_nt4.$ext\"\n";
		print OUT "!ELSE\n";
		print OUT "ALL : \"$opts{'NAME'}.$ext\"\n";
		print OUT "!ENDIF\n\n";
	}
	if($type ne 'driver'){
		print OUT "$cl_flags /c \n";
	} else {
		print OUT "!IF \"\$(NT4_TARGET)\" == \"true\"\n";
		print OUT "$cl_flags /D \"NT4_TARGET\" /c \n";
		print OUT "!ELSE\n";
		print OUT "$cl_flags /c \n";
		print OUT "!ENDIF\n";
	}
	$rsc_flags = "RSC_PROJ=/l 0x409 /d \"NDEBUG\" ";
	if($type ne 'driver'){
		print OUT "$rsc_flags \n";
	} else {
		print OUT "!IF \"\$(NT4_TARGET)\" == \"true\"\n";
		print OUT "$rsc_flags /d \"NT4_TARGET\" \n";
		print OUT "!ELSE\n";
		print OUT "$rsc_flags \n";
		print OUT "!ENDIF\n";
	}
	$link_flags = "LINK32_FLAGS=";
	foreach (@libs){
		$link_flags = $link_flags."$_.lib ";
	}
	foreach (@adlibs){
		$link_flags = $link_flags."$_.lib ";
	}
	$link_flags = $link_flags."/nologo /incremental:no /machine:I386 /nodefaultlib ";
	$link_flags = $link_flags."/subsystem:$s ";
	switch($type) {
		case 'dll' {
			$link_flags = $link_flags."/entry:\"DllMain\" /dll ";
			$link_flags = $link_flags."/def:$opts{'DEFFILE'} ";
			$link_flags = $link_flags."/implib:$opts{'NAME'}.lib ";
		}
		case 'native' {
			$link_flags = $link_flags."/entry:\"NtProcessStartup\" ";
		}
		case 'driver' {
			$link_flags = $link_flags."/base:\"0x10000\" /entry:\"DriverEntry\" ";
			$link_flags = $link_flags."/driver /align:32 ";
		}
	}
	if($type ne 'driver'){
		print OUT "$link_flags /out:\"$opts{'NAME'}.$ext\" \n\n";
	} else {
		print OUT "!IF \"\$(NT4_TARGET)\" == \"true\"\n";
		print OUT "$link_flags /out:\"$opts{'NAME'}_nt4.$ext\" \n";
		print OUT "!ELSE\n";
		print OUT "$link_flags /out:\"$opts{'NAME'}.$ext\" \n";
		print OUT "!ENDIF\n\n";
	}
	print OUT "CPP=cl.exe\nRSC=rc.exe\nLINK32=link.exe\n\n";
	print OUT ".c.obj::\n";
	print OUT "    \$(CPP) \@<<\n";
	print OUT "    \$(CPP_PROJ) \$<\n"; 
	print OUT "<<\n\n";

	print OUT "LINK32_OBJS=";
	foreach (@src_files){
		$x = $_;
		$x =~ s/\.c/\.obj/;
		print OUT "$x ";
	}
	foreach (@rc_files){
		$x = $_;
		$x =~ s/\.rc/\.res/;
		print OUT "$x ";
	}
	print OUT "\n\n";
	if($type ne 'driver'){
		if($type eq 'dll'){
			print OUT "DEF_FILE=$opts{'DEFFILE'}\n\n";
			print OUT "\"$opts{'NAME'}.$ext\" : \$(DEF_FILE) \$(LINK32_OBJS)\n";
			print OUT "    \$(LINK32) \@<<\n";
			print OUT "  \$(LINK32_FLAGS) \$(LINK32_OBJS)\n";
			print OUT "<<\n\n";
		} else {
			print OUT "\"$opts{'NAME'}.$ext\" : \$(LINK32_OBJS)\n";
			print OUT "    \$(LINK32) \@<<\n";
			print OUT "  \$(LINK32_FLAGS) \$(LINK32_OBJS)\n";
			print OUT "<<\n\n";
		}
	} else {
		print OUT "!IF \"\$(NT4_TARGET)\" == \"true\"\n";
		print OUT "\"$opts{'NAME'}_nt4.$ext\" : \$(LINK32_OBJS)\n";
		print OUT "    \$(LINK32) \@<<\n";
		print OUT "  \$(LINK32_FLAGS) \$(LINK32_OBJS)\n";
		print OUT "<<\n\n";		
		print OUT "!ELSE\n";
		print OUT "\"$opts{'NAME'}.$ext\" : \$(LINK32_OBJS)\n";
		print OUT "    \$(LINK32) \@<<\n";
		print OUT "  \$(LINK32_FLAGS) \$(LINK32_OBJS)\n";
		print OUT "<<\n\n";		
		print OUT "!ENDIF\n\n";
	}
	foreach (@rc_files){
		$x = $_;
		print OUT "SOURCE=$x\n\n";
		$x =~ s/\.rc/\.res/;
		print OUT "$x : \$(SOURCE)\n";
		print OUT "    \$(RSC) \$(RSC_PROJ) \$(SOURCE)\n\n";
	}
	close OUT;
}

# MinGW backend
sub produce_mingw_makefile {
	my ($type,$ext,$target,$targetnt);
	my (@adlibs_libs,@adlibs_paths);
	my $pos;
	my $x;

	open(OUT,"> Makefile.mingw") 
		or die("Can\'t open Makefile.mingw: $!");
	print OUT "PROJECT = $opts{'NAME'}\nCC = gcc.exe\n\n";
	print OUT "WINDRES = \"\$(COMPILER_BIN)windres.exe\"\n\n";
	$type = $opts{'TYPE'};
	switch($type){
		case 'console' {$ext = 'exe';}
		case 'gui'     {$ext = 'exe';}
		case 'dll'     {$ext = 'dll';}
		case 'driver'  {$ext = 'sys';}
		case 'native'  {$ext = 'exe';}
		else           {die("Unknown target type: $type!");}
	}
	$target = "$opts{'NAME'}.$ext";
	$targetnt = "$opts{'NAME'}_nt4.$ext";
	if($type eq 'driver'){
		print OUT "ifeq (\$(NT4_TARGET),true)\n";
		print OUT "TARGET = $targetnt\n";
		print OUT "else\n";
		print OUT "TARGET = $target\n";
		print OUT "endif\n";
	} else {
		print OUT "TARGET = $target\n";
	}
	if($type eq 'driver'){
		print OUT "ifeq (\$(NT4_TARGET),true)\n";
		print OUT "CFLAGS = -pipe  -Wall -g0 -O2 -DNT4_TARGET \n";
		print OUT "else\n";
		print OUT "CFLAGS = -pipe  -Wall -g0 -O2 \n";
		print OUT "endif\n";
	} else {
		print OUT "CFLAGS = -pipe  -Wall -g0 -O2\n";
	}
	if($type eq 'driver'){
		print OUT "ifeq (\$(NT4_TARGET),true)\n";
		print OUT "RCFLAGS = -DNT4_TARGET \n";
		print OUT "else\n";
		print OUT "RCFLAGS = \n";
		print OUT "endif\n";
	} else {
		print OUT "RCFLAGS = \n";
	}
	print OUT "C_INCLUDE_DIRS = \n";
	print OUT "C_PREPROC = \n";
	print OUT "RC_INCLUDE_DIRS = \n";
	print OUT "RC_PREPROC = \n";
	switch($type){
		case 'console' {print OUT "LDFLAGS = -pipe -Wl,--strip-all\n";}
		case 'gui'     {print OUT "LDFLAGS = -pipe -mwindows -Wl,--strip-all\n";}
		case 'native'  {
			print OUT "LDFLAGS = -pipe -nostartfiles -nodefaultlibs ";
			print OUT "-Wl,--entry,_NtProcessStartup\@4,--subsystem,native,--strip-all\n";
		}
		case 'driver'  {
			print OUT "LDFLAGS = -pipe -nostartfiles -nodefaultlibs ";
			print OUT "$opts{'NAME'}-mingw.def -Wl,--entry,_DriverEntry\@8,";
			print OUT "--subsystem,native,--image-base,0x10000,-shared,--strip-all\n";
		}
		case 'dll'     {
			print OUT "LDFLAGS = -pipe -shared -Wl,";
			print OUT "--out-implib,lib$opts{'NAME'}.dll.a -nostartfiles ";
			print OUT "-nodefaultlibs $opts{'NAME'}-mingw.def -Wl,--kill-at,";
			print OUT "--entry,_DllMain\@12,--strip-all\n";
		}
	}
	print OUT "LIBS = ";
	foreach (@libs){
		print OUT "-l$_ ";
	}
	foreach (@adlibs){
		$pos = rindex($_,"\\");
		push(@adlibs_libs,substr($_,$pos + 1));
		push(@adlibs_paths,substr($_,0,$pos));
	}
	foreach (@adlibs_libs){
		print OUT "-l$_ ";
	}
	print OUT "\nLIB_DIRS = ";
	foreach (@adlibs_paths){
		print OUT "-L\"$_\" ";
	}
	print OUT "\n\n";
	print OUT "SRC_OBJS = ";
	foreach (@src_files){
		$x = $_;
		$x =~ s/\.c/\.o/;
		print OUT "$x ";
	}
	print OUT "\n\nRSRC_OBJS = ";
	foreach (@rc_files){
		$x = $_;
		$x =~ s/\.rc/\.res/;
		print OUT "$x ";
	}
	print OUT "\n\n";
	print OUT "define build_target\n";
	print OUT "\@echo Linking...\n";
	print OUT "\@\$(CC) -o \$(TARGET) ";
	print OUT "\$(SRC_OBJS) \$(RSRC_OBJS) \$(LIB_DIRS) \$(LIBS) \$(LDFLAGS)\n";
	print OUT "endef\n\n";
	print OUT "define compile_resource\n";
	print OUT "\@echo Compiling \$<\n";
	print OUT "\@\$(WINDRES) \$(RCFLAGS) \$(RC_PREPROC) \$(RC_INCLUDE_DIRS) ";
	print OUT "-O COFF -i \"\$<\" -o \"\$\@\"\n";
	print OUT "endef\n\n";
	print OUT "define compile_source\n";
	print OUT "\@echo Compiling \$<\n";
	print OUT "\@\$(CC) \$(CFLAGS) \$(C_PREPROC) \$(C_INCLUDE_DIRS) ";
	print OUT "-c \"\$<\" -o \"\$\@\"\n";
	print OUT "endef\n\n";
	
	print OUT ".PHONY: print_header\n\n";
	print OUT "\$(TARGET): print_header \$(RSRC_OBJS) \$(SRC_OBJS)\n";
	print OUT "\t\$(build_target)\n";
	if($type eq 'dll'){
		print OUT "\t\$(correct_lib)\n";
	}
	print OUT "\nprint_header:\n";
	print OUT "\t\@echo ----------Configuration: $opts{'NAME'} - Release----------\n\n";
	if($type eq 'dll'){
		print OUT "define correct_lib\n";
		print OUT "\t\@echo ------ correct the lib\$(PROJECT).dll.a library ------\n";
		print OUT "\t\@dlltool -k --output-lib lib\$(PROJECT).dll.a --def $opts{'NAME'}-mingw.def\n";
		print OUT "endef\n\n";
	}
	foreach (@src_files){
		$x = $_;
		$x =~ s/\.c/\.o/;
		print OUT "$x: $_\n\t\$(compile_source)\n\n";
	}
	foreach (@rc_files){
		$x = $_;
		$x =~ s/\.rc/\.res/;
		print OUT "$x: $_\n\t\$(compile_resource)\n\n";
	}
	close OUT;
}

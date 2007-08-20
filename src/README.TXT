=========================
UltraDefrag Version 1.1.0
Updated Aug 16th, 2007
=========================

1. What is UltraDefrag?

   UltraDefrag is a powerful Open Source defragmentation utility for Windows
2000 and later operating systems.  It is very fast, because the defragmenting
is done by kernel-mode driver. They are three interfaces to them: graphical,
console and native.  The GUI is very useful, because visually represents your
filesystem as a color coded cluster  map. The console is another option for 
those that prefer the command line.  It is also benificial in that it can be 
run in the task scheduler. 
Finally, the native executable will run at boot time in a manner similar to 
chkdsk.

   The design of UltraDefrag is very simple. There are no skins, localization, 
or animations. It is a small and powerful program!

   It can be used on Windows 2000, Windows XP, Windows Server 2003,
Windows Vista and all of the 64-bit editions of Windows.
Windows NT 4.0 is currently unsupported.

   You must have administrative rights to install and use UltraDefrag.

   More information is available at http://ultradefrag.sourceforge.net


2. Features and Limitations

2.1 Features
+ optimal algorithm
+ ability to include/exclude files based on simple filters
+ exclude zero-length files and files that are placed in MFT
+ full support of compressed files processing
+ 'Compact' command to optimize free space disposition (faster than a full 
  defrag)
+ ability to generate a html fragmentation report
+ nice graphical interface
+ full x64 support

2.2 Limitations
- does not support volumes with more than 4.294.967.295 clusters
- does not work on NT 4.0
- gui does not work if screen color depth is less than 16 bits per pixel

3. Reporting Bugs

   Please report all problems and bugs in our bug tracker:
                 http://sourceforge.net/tracker/?group_id=199532

   NOTE: include please in your report following data:
   1. If you see 'Blue Screen Of Death' as result of UltraDefrag working 
   then include important parts of messages on them.
   2. In other cases run Sysinternals DebugView before you start UltraDefrag 
   and include in your report last 200 significant messages (or less). 
   You can free download DebugView here:
                 http://www.sysinternals.com

   For help requests visit our forum:
                 http://sourceforge.net/forum/?group_id=199532

   Other messages you can send to authors email:
                dmitriar@gmail.com


4. Frequently Asked Questions

See the FAQ.TXT file.


5. Building UltraDefrag

See the INSTALL.TXT file.


6. History

See the HISTORY.TXT file.


7. Authors.

See the CREDITS file.


8. Copyright

   Copyright (C) 2007 by Dmitri Arkhangelski.

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
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


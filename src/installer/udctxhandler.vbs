Set objShell = CreateObject("Shell.Application")
Set WshShell = WScript.CreateObject("WScript.Shell")
Set objArgs = WScript.Arguments
Rem WScript.Echo objArgs(0)
objShell.ShellExecute WshShell.ExpandEnvironmentStrings("%ComSpec%"), _
	"/C udctxhandler.cmd " & Chr(34) & objArgs(0) & Chr(34), "", "runas", 1
Rem WScript.StdOut.Write "Hit ENTER to EXIT..."
Rem WScript.StdIn.ReadLine

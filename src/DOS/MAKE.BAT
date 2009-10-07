@echo off
rem add /m to tasm if there are errors occured
TASM /ml /zn /l /m UDEFRAG.ASM
TLINK /t /3 UDEFRAG.OBJ
DIR UDEFRAG.???

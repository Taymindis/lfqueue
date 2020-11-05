@echo off
rem cleaning
@cls

if "%1"=="f" goto FULL
if "%1"=="F" goto FULL
if "%1"=="FULL" goto FULL

:PARTIAL
@del *.ilk
@del *.pdb
@del *.obj
goto EXIT
:FULL
@del *.exe
goto PARTIAL
:EXIT
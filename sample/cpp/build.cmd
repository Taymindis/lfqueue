
@echo off
@rem of course Windows VCode has to start from msvc developer cmd
@rem debug build on/off -- include/exclude argument -- /Zi
@rem for /d2FH4 see https://devblogs.microsoft.com/cppblog/making-cpp-exception-handling-smaller-x64/
@rem

if "%~1"=="R" goto RELEASE
if "%~1"=="r" goto RELEASE

if "%~1"=="d" goto DEBUG
if "%~1"=="D" goto DEBUG

goto HELP

:DEBUG
@echo.
@echo Building for DEBUG -- no exceptions + runtime DLL
@echo.
cl  /std:c++17 /MDd -D_HAS_EXCEPTIONS=0 /Zi /Fe: cppsample_dbg.exe cppsample.cpp ../../lfqueue.c
goto EXIT
:RELEASE
@echo.
@echo Building for RELEASE -- no exceptions and DLL runtime
@echo.
cl  /std:c++17 /MD -D_HAS_EXCEPTIONS=0 /Fe: cppsample.exe cppsample.cpp ../../lfqueue.c
goto EXIT
:HELP
@cls
@echo.
@echo Arguments are required for %~nx0
@echo.
@echo D or d for Debug build
@echo R or r for Release build
@echo.
@echo Default is DEBUG build
@echo.
@goto DEBUG
goto EXIT
:EXIT
@cls
@dir /D

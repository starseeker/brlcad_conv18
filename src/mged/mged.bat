@ECHO OFF

REM	Author:		Bob Parker
REM	Company:	Survice Engineering

SETLOCAL
SET SAVE_CD=%CD%
SET PATH=%~dp0
CD %PATH%
CD ..
SET BRLCAD_DATA=%CD%
CD %SAVE_CD%
SET WEB_BROWSER=C:\Program Files\Internet Explorer\IEXPLORE.EXE

IF "%1"=="-g" (
    START /B mged.exe %2
) ELSE (
    START /B mged.exe 2>&1 nul
)

CLS
EXIT

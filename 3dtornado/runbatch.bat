@echo off
cd /d "%~dp0"

IF EXIST tube.com (
del tube.com
)
nasmw.exe TUBE.ASM -fbin -o tube.com
if %ERRORLEVEL% EQU 0 goto runit
pause
exit
:runit
"C:\Program Files\DOSBox-0.74\DOSBox.exe" tube.com -exit

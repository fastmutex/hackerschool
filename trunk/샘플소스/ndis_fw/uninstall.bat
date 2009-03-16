@echo off

echo drivers
del %SystemRoot%\system32\drivers\ndis_hk.sys
del %SystemRoot%\system32\drivers\ndis_flt.sys

rem echo config and log files
rem del /Q %SystemRoot%\system32\LogFiles\ndis_fw > nul
rem rd %SystemRoot%\system32\LogFiles\ndis_fw > nul
rem del %SystemRoot%\system32\drivers\etc\ndis_fw.conf > nul

echo.
echo Restart Windows to unload drivers from memory.
pause

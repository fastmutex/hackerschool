@echo off

md %SystemRoot%\system32\LogFiles\ndis_fw > nul

echo default config
copy ndis_fw.conf %SystemRoot%\system32\drivers\etc > nul

echo drivers
copy bin\ndis_hk.sys %SystemRoot%\system32\drivers > nul
copy bin\ndis_flt.sys %SystemRoot%\system32\drivers > nul

echo registry
regedit /s install_nt4.reg

echo.
echo Change %SystemRoot%\system32\drivers\etc\ndis_fw.conf file for your taste and restart Windows.
pause

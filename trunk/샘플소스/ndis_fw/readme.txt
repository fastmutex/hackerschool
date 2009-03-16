
  -=- Welcome to Simple NDIS Hooking Based Firewall for NT4/2000 -=-


INSTALLATION

1. Run install_2k.bat for Windows 2000 or install_nt4.bat for NT4

2. Edit %SystemRoot%\system32\drivers\etc\ndis_fw.conf for your taste

3. Restart Windows


NOTE: The code it very raw so get ready for any unexpected behavior on
your system. (tdi_fw project is much more stable)


RULES

Edit file %SystemRoot%\system32\drivers\etc\ndis_fw.conf
Description of file format is in it.

Rules can be automatically reload in 5-10 sec after you change the file.

Errors are written in log (see below).


LOGS

Logs are text files. They're in
%SystemRoot%\system32\LogFiles\ndis_fw directory.

File name consists of year, month and date: YYYYMMDD.log

Numeric value after ALLOW or DENY is number of line in rules file with
applied rule.


BUGS

Check the latest version at http://ntdev.h1.ru/ndis_fw.html

Mail me to nt-dev@unshadow.net


DEBUGGING

You can dynamically load and unload ndis_flt driver using instdrv utility. 
But ndis_hk driver must be loaded before TCP/IP NDIS protocol driver
(tcpip.sys).

Comments for ndis_hk driver are in doxygen format.
See: http://www.doxygen.org

Run: doxygen dofygen.cfg
in .\ndis_hk directory and you will get HTML documentation.

See: .\ndis_hk\doc\html\index.html

---
vlad-ntdev

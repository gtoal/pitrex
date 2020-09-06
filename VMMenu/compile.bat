::
:: DOS VMMenu Compile
:: Chad Gray, 2011
::
:: Used when DOS compilation is carried out of a virtual XP machine
:: running on a Linux host PC.
::
:: Copies source files from Linux host machine and compiles them
::
:: Drive E: is mapped from the host machine and is the "VMMenu"
:: parent folder containing the VMMenu source code
::
:: Directory structure is
::
:: VMMenu
::   |
::   |-DOS        (DOS specific src)
::   |   '-zvg    (DOS ZVG SDK)
::   |
::   |-iniparser  (iniparser src)
::   |
::   |-Linux      (Linux specific src)
::   |   '-zvg    (Linux ZVG SDK)
::   |
::   '-VMMSrc     (VMMenu src)
::

:: Copy files to XP/DOS machine for local compilation
copy e:\DOS\DOSVmm.c .\DOS\
copy e:\DOS\DOSVmm.h .\DOS\

copy e:\DOS\zvg\*.c .\DOS\zvg\
copy e:\DOS\zvg\*.h .\DOS\zvg\

copy e:\iniparser\dictionary.h .\iniparser\
copy e:\iniparser\dictionary.c .\iniparser\
copy e:\iniparser\iniparser.h .\iniparser\
copy e:\iniparser\iniparser.c .\iniparser\

copy e:\VMMSrc\vmmstddef.h .\VMMSrc\
copy e:\VMMSrc\vchars.h .\VMMSrc\
copy e:\VMMSrc\vchars.c .\VMMSrc\
copy e:\VMMSrc\gamelist.h .\VMMSrc\
copy e:\VMMSrc\gamelist.c .\VMMSrc\
copy e:\VMMSrc\vmmenu.c .\VMMSrc\

make target=DOS
:: pause so you can examine the output for errors
pause

:: compress the exe file
upx --brute vmmenu.exe
copy vmmenu.exe e:\


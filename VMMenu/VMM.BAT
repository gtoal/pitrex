@echo off
::
::   Select your soundcard by changing "-soundcard -1" to the option used by
::   your soundcard.
::
::   Changing the resolution to "-r 320x200" can speed up framerates on
::   some PC's.
::
::   Changing "-zvg 1" to "-zvg 2" or "-zvg 3" can significantly speed up
::   framerates on some PC's.
::

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Process the selected game and run the appropriate exe::
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
if %1 == barrier  goto flipxy
if %1 == sundance goto flipxy
if %1 == tacscan  goto flipxy
::if %1 == bwidow goto fskip2
                      
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::             Default is to use Mame 0.104             ::
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
cd mame104
dvm104.exe %1 -zvg 3
cd ..
goto end


::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::  Flip X and Y axes for barrier,sundance and tacscan  ::
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:flipxy
cd mame104
dvm104.exe %1 -zvg 3 -flipx -flipy
cd ..
goto end


::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::           Use frameskip 2 for slower games           ::
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:fskip2
cd mame104
dvm104.exe %1 -zvg 3 -frameskip 2
cd ..
goto end


:end


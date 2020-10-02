This is the Bare-Metal menu program. Put pitrex.img in /boot along
with loader.pit, PiTrexsample.raw and menu.ym.

Currently menu options are hard-coded in loaderMain.c. They should
be read from a file, and Graham suggests that "quoted CSV" format
should be used.

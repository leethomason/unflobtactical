mkdir res
mkdir .\android\ufoattack_1\res\raw

copy .\ufobuilder\win32\Debug\ufobuilder.exe .
ufobuilder.exe .\resin\default.xml .\res\uforesource.db
copy .\res\uforesource.db .\android\ufoattack_1\res\raw\uforesource.png

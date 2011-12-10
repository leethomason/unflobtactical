echo ufobuilder.exe .\resin\ default.xml > buildres.bat
zip ufobuilder.zip -j .\ufobuilder\win32\Release\ufobuilder.exe c:\bin\SDL.dll c:\bin\SDL_image.dll buildres.bat

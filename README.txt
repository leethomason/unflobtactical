Xenowar is a turned based strategy game for Win32 and Android OS (version 2.1 or higher.)

Xenowar is influenced by X-COM, the Android development, and casual games. The project started
out as an experiment to understand mobile GPUs on a mobile phone, and just became too 
interesting to not keep playing with. 

Enjoy!


-- Xenowar community --

http://xenowar.net/forum/index.php


-- Free & Open Source --

The game is free and open source; it is available at:
http://code.google.com/p/unflobtactical/

A signed version is available from the Android app store. (And your support is appreciated!)
https://market.android.com/details?id=com.grinninglizard.UFOAttack


-- Installation, Win32 --

No installer - unzip the files someplace handy on your drive, or on your desktop. Double 
click the icon to run it. If you get a "dll not found error" you probably need the:

Microsoft Visual C++ 2010 Redistributable Package 

http://www.microsoft.com/downloads/en/details.aspx?FamilyID=a7b7a05e-6de6-4d3a-a423-37bf0912db84
(although the link tends to move)


-- Installation, Android, Unsigned version --

Download the APK from:
http://code.google.com/p/unflobtactical/

And install it. You'll need to have your phone set to accept unsigned apps. (The 2 dollar one
is signed.)


-- Using Mods --

You can add mod files to change the graphics of the game. (Or write your own - below.) 
Mod files allow people to share alternate graphics for the game. All mod files use the
extension .xwdb

You can select the mod you wish to use in the game settings.

Please be aware than an ill-constructed mod file can cause you game to crash or render incorrectly. 
If this happens, just delete the mod file.


-- Using Mods, Win32 --

On Win32, place the .xwdb files in the 'mods' subdirectory where you installed you game. When
you next start the game, the mod will be available in the settings screen. You can change the 
mod, or set it back to the default, at any time from the settings screen.


-- Using Mods, Android

If your browser supports unknown file type downloads, you can download the .xwdb file. It will
be placed in the 'dowloads' folder of your SD card, and Xenowar will scan it the next time 
the game starts.

If your browser does NOT support unknown file type downloads, you will get an error message 
from the browser, or a "download could not be completed" in the notification drop down. You 
can then place .xwbd files in the "downloads" subdirectory of your media card when it is 
mounted to your PC.

Some download managers can also download unknown file types.


-- Modding the Game --

If you wish to mod the game yourself, the tool is provided on Win32. 

The command line to create the mod is:
ufobuilder.exe input-xml-file output-database
Where: 
	input-xml-file: The all the XML files and assets need to be in the same directory. The input-xml-file is 
	the path to the XML file that describes how to build the assets, in that directory.
	
	output-database: the output file path to write. The extension should be .xwdb
	
You can build test mods by running 'makemod.bat'.
Examples of how to call the builder are in the 'makemod.bat' file. (It is a text file.)
Examples of the XML files used to build mods are in the 'modtest' subdirectory of the game.
More information can be found at:
	http://code.google.com/p/unflobtactical/wiki/Modding
and in the Xenowar forum.



default texture: 256x128
28 high (but this can be whatever fits)
2 padding
File export: png, xml
file name must contain "font"
doesn't support outline

wrap up tag in <resourc>
font tag fix: <font assetName="font" filename="font_0.png">



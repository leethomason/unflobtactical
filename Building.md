# Introduction #

Build targets for xenowar (ufoattack, unflobtactical):
  1. ufobuilder: a win32 c++ application that converts the source assets into the database file used at runtime. Optional if you don't change the source assets, of course.
  1. ufoattack / win32: the game itself, and also (with command line options) the tool that makes the maps for the game.
  1. ufoattack / android: the game, which is a JNI application on android. Almost all the code is C++. A small Java wrapper is used to drive it.

## Win32 Builds ##

Win32 requires:
  * Visual Studio 2010. The express edition (free  from Microsoft) is fine.
  * SDL and SDL image. http://www.libsdl.org/ Only the development libraries are required (the .lib, .dll, and .h).
  * You need to configure VS 2010 to work with SDL. TODO: add links to setting up VS 2010 with SDL

Once configured, open the solution file and simply hit "build all".

## Android Builds ##

Xenowar is built with eclipse. You need to download and install:
  * Android SDK
  * Eclipse, configured to work with the Android SDK
  * Android NDK

Follow the instructions on the android development site. Once that is configured, building for android is staightforward:
  * From the cygwin command line, run the ndk-build in the android/ufoattack\_1/jni directory
  * Build and run the java project from eclipse.

Xenowar will run in the emulator.

## Changing Assets ##

The "default.xml" file lists all the assets the game uses. It supports:
  1. image files for textures
  1. .ac files for models
  1. text data
  1. a simple font format
  1. simple hierarchical data

Changing the assets is straightforward; change the asset and run the builder. (Below.) Adding new assets is tricky - the code is a bit hacky. Experiment or drop me an email.

I'd like the game to be "moddable". If you are serious about creating a mod, drop me a line and I'll add some new hooks.

If you change or add assets, you need to create a new database file. Do a debug build of the "builder" (or "ufobuilder") from visual studio. Then the batch file:

makeres.bat

Will build a new database from the assets on disk.
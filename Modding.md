# Introduction #

This outlines how to create mod files for Xenowar, and is current as of version 2.4 of the game.

# Using a Mod #

A list of mods files can be found here:
http://code.google.com/p/unflobtactical/issues/list?can=2&q=label%3AMod

PC:
  1. Download a .xwdb data file.
  1. Open your xenowar directory.
  1. Go into the 'mods' sub-directory.
  1. Put the .xwdb file in this directory.
  1. Run the game, and select the mod in the settings screen.

Android:
  1. Run the game, b688 or higher. (2.4). This registers .xwdb files.
  1. With the Android browser, download a mod file.
  1. Run the game, and select the mod in the settings screen.

# Creating a Mod #

## How Xenowar Mods work ##

A .xwdb file extends the default database. Any resource in the .xwdb file will be used instead of the default. This allows mods to be created without the source code. (Although the source code can be helpful.)

Most importantly, if future versions of the game add new resources, the mod will still work. The mod file modifies the database - it doesn't replace it.

Note: currently only image types (bitmaps, textures, palettes, and images) can be modded. Sounds, models, and data are not supported. If you are interested in modding something not supported, post a message to the forum.

## Creating a Mod File ##

The builder for creating mod resources is included with the Win32 version of the game. The instructions for running it are in the README.txt file.

The 'modstest' subdirectory contains example files.
  * XML files that describe the assets in the mod files
  * Image files to put in the database

## Defining the Resources ##

### texture ###

Texture assets used in the game. They must be power of 2 (except for the battleship and base background textures, which are 384x384.). Many textures are hand-packed into texture atlases for performance. It would be nice if future versions of the builder would auto-gen the texture atlases.

  * assetName: the name of the asset.
  * filename: the filename in the directory.
  * dither: all images are converted to 16-bit. If dither is true, a dithering/smoothing algorithm is used. If false, a nearest color match is used.
  * width/height: the builder can down sample large source images. The original and destination size must both be powers of 2. (Except ground textures for battleships and bases, which are 384x384)

### palette ###

A palette is an input image that provides colors for the game.
  * dx: the number of columns
  * dy: the number of rows
  * sx/y: offset for the sample. 0.5 (typical) is the middle of the column / row

### font ###

The font asset is the basis of text rendering. The asset is created with Bitmap Font Generator: http://www.angelcode.com/products/bmfont/

The default asset size for Xenowar is 256x128, although you can use a larger one. (And more memory at runtime.) 2 padding is recommended. The font is interpreted as an alpha texture, so no colors or outlines are supported.

When you export:
  * 2 padding in all directions
  * png image file and xml data file
  * WARNING: the name of the asset and filename must contain the word "font". This is used to throw the texture into a special rendering mode.

BitmapFont will generate a .fnt file, that should be XML. You can cut and past the font tag into your mod file xml. You need to change the font tag to have these attributes:

```
<font assetName="font" filename="font_0.png">
```

Where the "font" and "font\_0.png" are the names you are using. Please remember that "font" must be in the names.

### Not yet supported: 

&lt;model&gt;

 Single form ###

If the model does NOT have a modelName attribute, then it is the "single form". One model is in the model file.

Currently only the ac3d file is supported.

This has the same bug as textures: the model filename (ufopod.ac) is also the model asset name (ufopod) and therefore the name can't be changed.

  * shading: 'flat' shading is very angular. Good for walls and crates and such. 'smooth' shading is smooth everywhere - good for characters and organic shapes. 'crease' attempts to intelligently choose, but the algorithm isn't very intelligent.
  * polyRemoval: 'pre' removes down facing bottom polys in the first stage. This should be set for anything that stands on the ground plane.

### Not yet supported: 

&lt;model&gt;

 multi-form ###

This allows many models to be in one .ac file. Very handy.

  * filename: the name of the file, and correctly NOT the asset name
  * modelName: the name of the asset as needed by the game. This must be the name of the 'group' in the .ac file, which is how a particular model is identified.
  * origin: the coordinates of the models origin point - where it stands on the ground.
  * trigger: the location where a character holds its gun
  * eye: the origin of a character's line of site
  * target: where others target the character. The center of its chest.

### Not yet supported: 

&lt;data&gt;

 sounds ###

Generally sound files. The asset name and file name are the same. (Same bug as above.) For sounds, the attribute 'compress' must be set to 'false', since the sounds are played directly from the resource.

## Not yet supported: Creating New Levels ##

It's possible to start xenowar in 'editor' mode, but it needs work to function without compiler support.
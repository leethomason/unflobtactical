Useful code and links from the web. And sometimes a sort of development blog.

# PreMultiplied Alpha #

http://www.opengl.org/wiki/Blending
http://home.comcast.net/~tom_forsyth/blog.wiki.html#%5B%5BPremultiplied%20alpha%5D%5D

what we want:
FB.rgb = texel.rgb + FB.rgb **(1-texel.a)**

done with:
glBlendFunc( glGL\_ONE, GL\_ONE\_MINUS\_SRC\_ALPHA )


# Xeno Engine #

  * TinyXML2
    * (done) Memory use of TinyXML.
    * (done) Clean up XML output utility

  * XenoEngine 2.0
    * 32 bit (opaque) textures
    * (deferred) ETC-1 support
    * (done) Basic shader support
    * (done) instancing
    * atlasing
    * effect pipeline

  * (done) Separate game logic out of engine layer. Done Dec 2011. The game is still pretty turn-based centered, but at least the assets and game specific classes are properly separated.
  * (done) Layout system - that isn't terrible - for gamui.
  * (done) Fix rectangle class. Done Dec 2011. Many bugs lurk, I'm sure. But initial cleanup, review, and testing is complete. Interesting study in refactoring: while working on the Rectangle class, I was uncertain about the change. The Union behavior is particularly troubling. But in use, it is much cleaner. The x=X0(); x<X1() is a very clean metaphor. (Perhaps always question <= as possibly being a bad path.)
  * (done) Remove deprecated utility.

# New Aliens #

Plan for the next couple of versions:
- stop making UI changes. Painful to get right. Incremental value.
- try to stabilize the assets for modding
- new aliens

Not do:
  * UI changes. Really hard to get right. Player re-education.
  * Crouching. New mechanic, new UI.
  * Highlights & such
  * Emmissive & such
  * installer
  * button improvements / unique buttons

Do:
  * Save file support. #1 request from comments. Does have UI.
  * back / escape key. Maybe: seems natural, but no other integration with OS
  * maybe: info console. Clutters game screen.
  * last names. (done)
  * unique backgrounds. doesn't necessarily work as expected.
  * dragging optional (done)

Ideas:
  * Squid. Mind read team positions.
  * Spitter / Crab. Civ takeover.
  * Rocket types - how to select??
  * Grenades - how to select?? (Choose the first in inventory?)
    * (Explosive)
    * (Incendiary)
    * Flare
    * Smoke



# Ordering Rendering #

What order?
Tom Forsyth suggestion:
http://home.comcast.net/~tom_forsyth/blog.wiki.html

Clever:
http://realtimecollisiondetection.net/blog/?p=86

# Duplicate classes #

This being a hobby project, it seems like I couldn't actually write 2 classes that did the same thing. But exhibit #1: Target and Visibility. Code in both classes get called, yet they both actually had a UnitCanSee( x, y ) method for example.

I deleted Target, and went with the more sophisticated and smarter-caching Visibility.

I'm somewhat mystified by the process to get here. I **think** they both grew up from different functionality that changed over time. But I'm not sure. They had one important difference: Target could be copied and compared to a previous Target, which is used to generate TargetEvents. For example: if you move and see an alien, the game will cancel the move to respond to that new information. The only way to reliably detect that is to compare states, because the number of reasons (fire, explosion, alien moved, player moved, tree burned down, etc. etc.) is too hard to track.

Adding a simple utility method to Visibilty fixed the compare problem, and made it a better superset of Target. But strange. And an interesting reminder that it is easy to duplicate even your own code.

# Preprocessor macros! #

Sweet: http://predef.sourceforge.net/prearch.html

# Resources #

I have a goal to keep UFOattack at about a 1MB resources file. It's by no means a hard limit, but small files are good for both deployment and debugging. The largest consumer of file space so far is the help screens. Fully 1/3 of the file. I pulled them and am replacing them with text and images. The only downside...I am now writing Yet Another Line Breaker. It's a little break from the more grueling job of platform code.

# Planar Shadows #
(Aug01,2010)

UFOAttack uses planar shadows. Nice summary: http://www.devmaster.net/articles/shadow_techniques/

In the first attempt, 3 passes were used:
  1. Render the ground plane lighted, and write to the z buffer.
  1. Using the matrix from the above link, the shadows are then rendered at a slight z-offset.
  1. The ground plane is rendered again, testing the z value, so that only areas in shadow are draw.

Basically the z-buffer is "abused" to act as a stencil, which isn't available in ES1.1. It works pretty well, but has some annoying artifacts since the z is being tested where it shouldn't.

But wouldn't it be nice to do this without the whole z-thing? You can! It's tricky to code, but it can be done in 2 passes. The basic requirement is that the plane is one big texture.

So now:
  1. The ground plane is rendered lighted, no z tested or written
  1. The models are rendered as shadows.

In the 2nd step, the model GEOMETRY is transformed as:
```
  [M] = [Matrix shadow plane][Matrix xform] (as above)
```

The TEXTURE transform is set up as:
```
  [M] = [M swizzle][Matrix shadow plane][Matrix xform]
```
And the texture coordinate array is pointed to the vertex positions. The vertex coordinates are fed to the texture coordinates. The "swizzle" is used to scale from geometry space to texture space. It's a scaling matrix, and flips (xz) to (xy).

Tricky to code without shaders. (With shaders this would be trivial. You wouldn't even specify texture coordinates.)

# Back to Databases #
(Apr21, 2010)

I have great respect for sqlite3. Solid, well thought out, well tested API.

But I'm just attached to hierarchical data. Curse that XML and it's hierarchical ways...I just can't give it up. I wrote a very simple database that takes advantage that the writer can be slow and use a lot of memory, while the reader needs to be lean and mean. Great exercise, and a nice bit of code on the other side. (gamedb, gamedbwriter, and gamedbreader.) gamedb loads only the part of the file need to index the data and reads the data itself on demand.

# On to Android #

With the license change in the iPhone 4 SDK specifying the source language, Apple has crossed the line from being annoying to draconian. I'm switching development to Android. Hopefully it will go well.

Android supports both GL:ES1.1 and ES2.0, which I'd like to switch to at some point. (After it's working.) I have a Nexus One for development, which is a great little phone. I'm sure there will be hiccups on the way - the code thus far is very iPhone-centric.

# DB vs. XML vs. Binary #

Initially, a bunch of serialization (saving/loading) was done with binary files. Binary files are small. That's all I have good to say about them. They are difficult to debug, tool, and version.

Along came sqlite3. sqlite3 is grand - good API, simple, and great tools. You can open up a command line to see what is in the "save files". I probably went a little too gung-ho with sqlite3, just because it was fun to learn and a huge improvement over binary files. It's grand for saving and fetching resources. The resource system will stay in the DB.

But it's hard to represent complex data relationships. I'm sure the DB folks out there know all the tricks, but it's just inconvenient to represent something like a Map, which contains Items, Storage, some Flags, and tagged data. XML is great at that.

XML is super flexible, good tools (you can browse it with IE or Firefox), and represents rich data easily. But it creates huge files relative to the original binary data. However, the game's files are so tiny (<50k) and compress very well (often less that 10k) that I'm switching to it as the primary format for save files.

That's the road so far for serialization. A difficult system to "get right" in the code.

# Code Static analysys #

Run cppcheck and see what it turns up.

# Application data #

Mac Code: wacky NS stuff (already done)

Win Code: SHGetSpecialFolderPath

# Considering SQLite DB #

Considering moving to SQLite for file access, and removing the roll-my-own. The only downside is learning a new API; upside is a way to view and edit the save files, and a solid mechanism for file access.

Nice example of simple (but efficient) SQLite that avoids lots of prints to strings.

```
sqlite3* db = NULL;
sqlite3_open("myfile.db", &db);

sqlite3_stmt* stmt = NULL;
sqlite3_prepare_v2(db, "select * from foo where a=? and b=?;", &stmt,
NULL);

sqlite3_bind_int(stmt, 1, 42);
sqlite3_bind_double(stmt, 2, 4.2);

while (sqlite3_step(stmt) == SQLITE_ROW) {
  foo row;
  row.a = sqlite3_column_int(stmt, 0);
  row.b = sqlite3_column_double(stmt, 1);
  row.d = sqlite3_column_bytes(stmt, 2);
  assert(row.d <= sizeof(row.c));
  memcpy(row.c, sqlite3_column_blob(stmt, 2), row.d);

  // do something with row
}

sqlite3_finalize(stmt);
sqlite3_close(db); 
```
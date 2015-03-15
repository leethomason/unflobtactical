# Xenowar News #

## Logging (Nov 27, 2010) ##

Debugging is always a challenge. It's a big block of code, with lots of different states, and there is (so far) only me developing. But 2 good debugging tricks to the rescue!
  1. The AI can play either Alien or Terran. I can turn the game loose to play itself and wait for issues.
  1. When the game crashes, the next time you play it will try to send the currentgame.xml back to grinninglizard.com.

When a log comes in, I set the AI to play itself using the current game at the time of the crash. About 1 log in 4 turns up a debuggable issues - which is quite good! I'm finding some good bugs this way.

I'm also cleaning up, reviewing, and minimizing code. But there is no substitute for testing.

## Name, Art, Links (Nov 23, 2010) ##

New name: Xenowar. Although 'unflobtactical' is...interesting, it can't be pronounced. 'UFO Attack' is nice, but done, and there is already a Java game with the same name. 'Xenowar' evokes the right sound for me, has a retro feel, and isn't done to death. I've grabbed Xenowar.net, but there isn't a web page yet. (Xeno pronounced ZENN-Oh.)

Art! This is the time of creating lots of art, which is lots of fun. Good to get out of programming for a while. There are new characters models, where I'm really striving for a simple, iconic, cartoony look that will look good on small screens. The look is inspired by chess pieces:

![http://unflobtactical.googlecode.com/svn/wiki/images/chessline1.png](http://unflobtactical.googlecode.com/svn/wiki/images/chessline1.png)

Some features need to be implemented to support the new art. New model skinning, explosive items, new aliens. But mostly art and level design stuff.

### Links ###

Big thanks to UFO:TTS team for linking from their amazing UFO game: [The Two Sides](http://ufotts.ninex.info/). Michal also posted a very nice beta (pre-)review at strategycore.co.uk:

http://www.strategycore.co.uk/forums/UFO-Attack-working-title-X-com-t8160.html&p=100174#entry100174
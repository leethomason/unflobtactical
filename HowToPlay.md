# Xenowar #

Welcome to Xenowar! Xenowar is a turned based tactical combat game. It is inspired by the classic game X-COM, but has changed and taken some new direction over development. (It's been a very fun and rewarding project.)

There are 2 main teams: the Terran Soldiers and the Aliens. You play the Terrans, while the computer plays the Aliens. Some levels also have civilians, haplessly trying to avoid getting blasted.

The game saves whenever you exit. You can quit at any time and choose "Continue" from the menu when you restart to resume where you left off.

## Bugs and Feedback ##

Submit any bugs at: http://code.google.com/p/unflobtactical/issues/list

For discussion, ideas, and sharing about the game, there is a forum at: xenowar.net/forum

## Current and Future Plans ##

The tactical game is fully playable with a variety of scenarios. In the future, I'd like to add a strategic "geoscape" game with the tactical one.

## Android ##

The APK file is beta and not signed. From the marketplace the file is signed. Either way, it is a very simple and safe application. Xenowar does have permission to the use the internet connection to send back the current save game file if it detects a crash.

## PC Version ##

Unzip Xenowar wherever you like. (The desktop is handy.) Double click "xenowar.exe" to play. It doesn't have an installer.

Left mouse button: click to select or tap buttons. Drag to move the camera.

Right mouse button: drag to zoom and rotate.

Note that you can also select units with the arrows keys and rotate with the control-arrow keys. Since you don't need the select and rotate UI, tap the 'u' key to disable it.

# Tactical Screen #

![http://unflobtactical.googlecode.com/svn/wiki/images/xenowar_2.jpg](http://unflobtactical.googlecode.com/svn/wiki/images/xenowar_2.jpg)

## Concepts ##

### Turns & Time ###

The game is played in turns. The turns are:
  * Terran Soldier
  * Civilian
  * Alien

When it is your turn, you can move each unit. Each unit has a certain number of **Time Units (TU)**. Each time unit allows the unit to move one square of distance. Shooting also consumes TU. TUs are replenished at the beginning of each turn.

Inventory management and rotation consume no TU.

#### Reaction Fire ####

During the enemy turn (Alien turn for the player) it is possible to have 'reaction fire'. Shooting that the Terrans can do during the Alien turn, if an Alien is spotted. Reaction fire is important so that you can defend your troops between turns.

Of course, the Aliens get reaction fire too.

The only time rotation matters in the game is for reaction fire. The more a unit has to turn, the less likely the unit will take reaction fire, and the less accurate the shot will be.

Reaction fire is key to defense. Make sure to position you units! It is also how aliens defend the UFOs. Explosives can come it very helpful to clear a room without actually walking through a guarded door.

### Sight and Fog of War ###

Every unit has a 360 degree range of vision. The map is revealed everywhere a team can see. Everything else is in the "fog of war" and rendered in black. (Areas you have previously seen are rendered in dark gray.)

Nighttime significantly reduces the Terran vision. Aliens are less affected, but still impacted. However, additional night vision gives the aliens a significant advantage at night time.

Fire and smoke obscures vision for all teams.

Doors are never locked; you can always walk through doors. They close behind you.

### Weapons ###

Weapons in the game have different fire modes. Each is different, but broadly speaking:
  * Snap: A fast fire mode, low accuracy.
  * Auto/Aimed: A burst fire (3 shots), or aimed mode, high accuracy.
  * An alternate fire mode - incendiary or explosive.

Each weapon (and armor) also has a tech level, from 1 to 3. The higher the tech level, the better the weapon.

Terran Weapons:
  * ASLT - Assault rifle. Moderate accuracy, 3 round burst, secondary explosive fire mode. (RPG)
  * LR - Long range rifle. Deadly accurate, high damage, single fire mode.
  * MCAN - Mini canon. Poor accuracy, but has both incendiary and kinetic explosive shots.

Alien Weapons:
  * RAY - Ray gun.
  * PLS - Plasma Rifle.
  * STRM - Fire storm. High capacity weapon with explosive rounds.

Ammo for all weapons is automatically loaded - by having the ammo in your inventory it will be used. More powerful weapons can destroy buildings and the landscape, and occasionally will start fires.

Each weapon also does different types of damage: Kinetic, Energy, and Incendiary. Alien weapons tend to be energy based, and Terran weapons are generally kinetic.  Which leads us to the next topic, Armor.

### Armor ###

Each unit has HP, which is invariant of the armor worn. Armor reduces the amount of damage you take, but does not change HP. ARM-1, 2, and 3 all provide basic protection against all kinds of weapons.

Additionally a unit can have shield generators. There are shield generators that protect against kinetic, incendiary, and energy weapons.

## Interface ##

### Selected Unit ###

**Possible destinations are highlighted on the map.** Tap to travel there. Destinations in
  * Green leaves enough TUs for **auto** reaction fire.
  * Yellow leaves enough TUs for **snap** reaction fire.
  * Orange means the unit has insufficient time for reaction fire.

**Alien units** tap any alien units to bring up the fire menu.

You can also select the target icon on the left menu to arbitrarily target any location on the map.

### Status Bar at Top ###

The text across the top of the page gives information on the selected character.
  * Rank. As the unit increases in rank, his/her TUs, HP, Accuracy, and Reaction Fire all improve. The ranks are:
    * Rookie
    * Private
    * Sargent
    * Major
    * Captain
    * Commander
  * Name
  * Currently armed weapon

### Control Buttons ###
  * Exit: The lander takes off, leaving behind anyone not on-board.
  * Help: How to play and help screens.
  * Next turn: ends the current turn, aliens and civs move.
  * Target allows you to fire at an arbitrary location on the map. Tap the Target icon and then the map location.
  * Character screen. Tap to manage inventory or pick up items on the world map.
  * Left / Right. Turn the selected unit.

And on the bottom right:
  * Previous selects the previous unit.
  * Next selects the next unit.

## Inventory ##

This is the character inventory screen. On the left side is what the selected unit is carrying. On the right are items on the ground. You can press the "stats" button to toggle between items on the ground and the unit stats.

![http://unflobtactical.googlecode.com/svn/wiki/images/xenowar_inventory.png](http://unflobtactical.googlecode.com/svn/wiki/images/xenowar_inventory.png)

### Carried (left) Items ###
Items in the left boxes are carried by the unit. The top 2 items - one armor slot and one weapon slot - are currently "in use". Items in the units "pack" are shown in the 6 slots below.

You can tap and drag to organize items, or change weapons and armor. A single tap will drop the item on the ground.

Weapons automatically load from rounds carried in the backpack. For the weapon in use, the number of rounds are shown below the name. In this example the ASLT-2 has 30 rounds in the clip (primary fire) and 4 rockets.

### Inventory (right) Items ###
The left (blue) column selects the groups of items to view.
  * Miscellaneous and alien items
  * Armor
  * Alien Weapons
  * Terran Weapons
You can click to move items between the items on the ground and items in your backpack.
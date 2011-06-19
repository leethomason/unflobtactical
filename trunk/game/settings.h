/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef UFO_SETTINGS_INCLUDED
#define UFO_SETTINGS_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glstringutil.h"

class SettingsManager
{
public:
	static SettingsManager* Instance()	{ GLASSERT( instance );	return instance; }

	static void Create( const char* savepath );
	static void Destroy();


	bool GetAudioOn() const				{ return audioOn != 0; }
	void SetAudioOn( bool value );

	bool GetSuppressCrashLog() const	{ return suppressCrashLog != 0; }
	bool GetPlayerAI() const			{ return playerAI != 0; }
	bool GetBattleShipParty() const		{ return battleShipParty != 0; }
	bool GetUseFastBattle() const		{ return useFastBattle != 0; }
	int  GetNumWalkingMaps() const		{ return nWalkingMaps; }

private:
	SettingsManager( const char* path );
	~SettingsManager()	{}
	void Save();

	static SettingsManager* instance;

	int audioOn;
	int suppressCrashLog;
	int playerAI;
	int battleShipParty;
	int useFastBattle;
	int nWalkingMaps;

	grinliz::GLString path;

};


#endif // UFO_SETTINGS_INCLUDED
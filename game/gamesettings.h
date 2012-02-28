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

#ifndef UFO_GAMESETTINGS_INCLUDED
#define UFO_GAMESETTINGS_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glstringutil.h"
#include "../engine/settings.h"

class GameSettingsManager : public SettingsManager
{
public:
	static GameSettingsManager* Instance()	{ GLASSERT( gameInstance );	return gameInstance; }

	static void Create( const char* savepath );
	static void Destroy();

	// read only
	bool GetSuppressCrashLog() const	{ return suppressCrashLog != 0; }
	bool GetPlayerAI() const			{ return playerAI != 0; }
	bool GetBattleShipParty() const		{ return battleShipParty != 0; }
	int GetTestAlien() const			{ return testAlien; }
	
	// read-write
	void SetConfirmMove( bool confirm );
	bool GetConfirmMove() const			{ return confirmMove; }

	void SetCurrentModName( const grinliz::GLString& string );
	const grinliz::GLString& GetCurrentModName() const		{ return currentMod; }

	void SetAllowDrag( bool allow );
	bool GetAllowDrag() const			{ return allowDrag; }

protected:
	GameSettingsManager( const char* path );
	virtual ~GameSettingsManager()	{ gameInstance = 0; }

	virtual void ReadAttributes( const tinyxml2::XMLElement* element );
	virtual void WriteAttributes( tinyxml2::XMLPrinter* );

private:
	static GameSettingsManager* gameInstance;

	int suppressCrashLog;
	int playerAI;
	int battleShipParty;
	int testAlien;
	bool confirmMove;
	bool allowDrag;
	grinliz::GLString currentMod;
};


#endif // UFO_GAMESETTINGS_INCLUDED
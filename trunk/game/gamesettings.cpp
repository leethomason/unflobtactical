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

#include "gamesettings.h"
#include "../tinyxml2/tinyxml2.h"
#include "../engine/serialize.h"

using namespace grinliz;
using namespace tinyxml2;

GameSettingsManager* GameSettingsManager::gameInstance = 0;

void GameSettingsManager::Create( const char* savepath )
{
	gameInstance = new GameSettingsManager( savepath );
	gameInstance->Load();
}


void GameSettingsManager::Destroy()
{
	delete gameInstance;
	gameInstance = 0;
}


GameSettingsManager::GameSettingsManager( const char* savepath ) : SettingsManager( savepath )
{
	// Set defaults.
	suppressCrashLog = 0;
	playerAI = 0;
	battleShipParty = 0;
	confirmMove = 
		#ifdef ANDROID_NDK
			1;
		#else
			0;
		#endif
	allowDrag = true;
	testAlien = 0;
}


void GameSettingsManager::ReadAttributes( const XMLElement* root )
{
	SettingsManager::ReadAttributes( root );

	root->QueryIntAttribute( "suppressCrashLog", &suppressCrashLog );
	root->QueryIntAttribute( "playerAI", &playerAI );
	root->QueryIntAttribute( "battleShipParty", &battleShipParty );
	root->QueryBoolAttribute( "confirmMove", &confirmMove );
	root->QueryBoolAttribute( "allowDrag", &allowDrag );
	root->QueryIntAttribute( "testAlien", &testAlien );
	currentMod = "";
	if ( root->Attribute( "currentMod" ) ) {
		currentMod = root->Attribute( "currentMod" );
	}
}


void GameSettingsManager::WriteAttributes( XMLPrinter* printer )
{
	SettingsManager::WriteAttributes( printer );

	printer->PushAttribute( "currentMod", currentMod.c_str() );
	printer->PushAttribute( "suppressCrashLog", suppressCrashLog );
	printer->PushAttribute( "playerAI", playerAI );
	printer->PushAttribute( "battleShipParty", battleShipParty );
	printer->PushAttribute( "confirmMove", confirmMove );
	printer->PushAttribute( "allowDrag", allowDrag );
	printer->PushAttribute( "testAlien", testAlien );
}


void GameSettingsManager::SetCurrentModName( const GLString& name )
{
	if ( name != currentMod ) {
		currentMod = name;
		Save();
	}
}


void GameSettingsManager::SetConfirmMove( bool confirm )
{
	if ( confirm != confirmMove ) {
		confirmMove = confirm;
		Save();
	}
}


void GameSettingsManager::SetAllowDrag( bool allow ) 
{
	if ( allowDrag != allow ) {
		allowDrag = allow;
		Save();
	}
}

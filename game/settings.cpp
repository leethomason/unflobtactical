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

#include "settings.h"
#include "../tinyxml/tinyxml.h"
#include "../engine/serialize.h"


SettingsManager* SettingsManager::instance = 0;

void SettingsManager::Create( const char* savepath )
{
	GLASSERT( instance == 0 );
	instance = new SettingsManager( savepath );
}


void SettingsManager::Destroy()
{
	delete instance;
	instance = 0;
}


SettingsManager::SettingsManager( const char* savepath )
{
	
	path = savepath;
	path += "settings.xml";

	// Set defaults.
	audioOn = 1;
	suppressCrashLog = 0;
	playerAI = 0;
	battleShipParty = 0;
	useFastBattle = 0;
	nWalkingMaps = 1;
	confirmMove = 
		#ifdef ANDROID_NDK
			1;
		#else
			0;
		#endif

	// Parse actuals.
	TiXmlDocument doc;
	if ( doc.LoadFile( path.c_str() ) ) {
		TiXmlElement* root = doc.RootElement();
		if ( root ) {
			root->QueryIntAttribute( "audioOn", &audioOn );
			root->QueryIntAttribute( "suppressCrashLog", &suppressCrashLog );
			root->QueryIntAttribute( "playerAI", &playerAI );
			root->QueryIntAttribute( "battleShipParty", &battleShipParty );
			root->QueryIntAttribute( "useFastBattle", &useFastBattle );
			root->QueryIntAttribute( "nWalkingMaps", &nWalkingMaps );
			root->QueryBoolAttribute( "confirmMove", &confirmMove );
		}
	}
	nWalkingMaps = grinliz::Clamp( nWalkingMaps, 1, 2 );
}


void SettingsManager::SetNumWalkingMaps( int maps )
{
	maps = grinliz::Clamp( maps, 1, 2 );
	if ( maps != nWalkingMaps ) {
		nWalkingMaps = maps;
		Save();
	}
}


void SettingsManager::SetConfirmMove( bool confirm )
{
	if ( confirm != confirmMove ) {
		confirmMove = confirm;
		Save();
	}
}


void SettingsManager::SetAudioOn( bool _value )
{
	int value = _value ? 1 : 0;

	if ( audioOn != value ) {
		audioOn = value;
		Save();
	}
}


void SettingsManager::Save()
{
	FILE* fp = fopen( path.c_str(), "w" );
	if ( fp ) {
		XMLUtil::OpenElement( fp, 0, "Settings" );

		XMLUtil::Attribute( fp, "audioOn", audioOn );
		XMLUtil::Attribute( fp, "suppressCrashLog", suppressCrashLog );
		XMLUtil::Attribute( fp, "playerAI", playerAI );
		XMLUtil::Attribute( fp, "battleShipParty", battleShipParty );
		XMLUtil::Attribute( fp, "useFastBattle", useFastBattle );
		XMLUtil::Attribute( fp, "nWalkingMaps", nWalkingMaps );
		XMLUtil::Attribute( fp, "confirmMove", confirmMove );

		XMLUtil::SealCloseElement( fp );

		fclose( fp );
	}
}

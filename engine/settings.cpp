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
#include "../tinyxml2/tinyxml2.h"
#include "../engine/serialize.h"

using namespace grinliz;
using namespace tinyxml2;

SettingsManager* SettingsManager::instance = 0;

void SettingsManager::Create( const char* savepath )
{
	GLASSERT( instance == 0 );
	instance = new SettingsManager( savepath );
	instance->Load();
}


void SettingsManager::Destroy()
{
	delete instance;
}


SettingsManager::SettingsManager( const char* savepath )
{
	GLASSERT( instance == 0 );
	instance = this;

	path = savepath;
	path += "settings.xml";

	// Set defaults.
	audioOn = 1;
	nWalkingMaps = 1;

}


void SettingsManager::Load()
{
	// Parse actuals.
	XMLDocument doc;
	doc.LoadFile( path.c_str() );
	if ( !doc.Error() ) {
		XMLElement* root = doc.RootElement();
		if ( root ) {
			ReadAttributes( root );
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
		XMLPrinter printer( fp );

		printer.OpenElement( "Settings" );
		WriteAttributes( &printer );
		printer.CloseElement();
		fclose( fp );
	}
}


void SettingsManager::ReadAttributes( const XMLElement* root )
{
	// Actuals:
	root->QueryIntAttribute( "audioOn", &audioOn );
	root->QueryIntAttribute( "nWalkingMaps", &nWalkingMaps );
	nWalkingMaps = grinliz::Clamp( nWalkingMaps, 1, 2 );
}


void SettingsManager::WriteAttributes( XMLPrinter* printer )
{
	printer->PushAttribute( "audioOn", audioOn );
	printer->PushAttribute( "nWalkingMaps", nWalkingMaps );
}

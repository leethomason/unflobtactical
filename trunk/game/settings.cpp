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
		}
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

		XMLUtil::SealCloseElement( fp );

		fclose( fp );
	}
}

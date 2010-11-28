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

private:
	SettingsManager( const char* path );
	~SettingsManager()	{}
	void Save();

	static SettingsManager* instance;

	int audioOn;
	int suppressCrashLog;
	int playerAI;


	grinliz::GLString path;

};


#endif // UFO_SETTINGS_INCLUDED
#ifndef UFO_SOUND_INCLUDED
#define UFO_SOUND_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../shared/gamedbreader.h"

class SoundManager
{
public:
	static SoundManager* Instance()	{ GLASSERT( instance ); return instance; }

	void QueueSound( const char* name );
	//void PlayQueuedSounds();
	bool PopSound( int* offset, int* size );

	static void Create(  const gamedb::Reader* db );
	static void Destroy();
private:
	SoundManager(  const gamedb::Reader* db ) : database( db ), nSounds( 0 )	{}
	~SoundManager()	{}

	enum { MAX_QUEUE = 8 };
	const char* queue[MAX_QUEUE];
	const gamedb::Reader* database;
	int nSounds;

	static SoundManager* instance;
};


#endif // UFO_SOUND_INCLUDED
#include "ufosound.h"
#include "../grinliz/glstringutil.h"
#include "cgame.h"

using namespace grinliz;

SoundManager* SoundManager::instance = 0;

void SoundManager::Create(  const gamedb::Reader* db )
{
	GLASSERT( instance == 0 );
	instance = new SoundManager( db );
}


void SoundManager::Destroy()
{
	GLASSERT( instance );
	delete instance;
	instance = 0;
}


void SoundManager::QueueSound( const char* name )
{
	if ( nSounds < MAX_QUEUE ) {
		for( int i=0; i<nSounds; ++i ) {
			if ( StrEqual( queue[i], name ) )
				return;
		}
		queue[nSounds++] = name;
	}
}


void SoundManager::PlayQueuedSounds()
{
	if ( nSounds == 0 )
		return;
	const gamedb::Item* data = database->Root()->Child( "data" );

	for( int i=0; i<nSounds; ++i ) {
		const gamedb::Item* sndItem = data->Child( queue[i] );
		GLASSERT( sndItem );
		int size=0;
		const void* sound = database->AccessData( sndItem, "binary", &size );
		GLASSERT( sound );
		if ( sound ) {
			PlayWAVSound( sound, size );
		}
	}
	nSounds = 0;
}

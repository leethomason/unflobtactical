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

#include "ufosound.h"
#include "../grinliz/glstringutil.h"
#include "cgame.h"
#include "settings.h"

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
	SettingsManager* settings = SettingsManager::Instance();
	if ( !settings->GetAudioOn() )
		return;

	if ( nSounds < MAX_QUEUE ) {
		for( int i=0; i<nSounds; ++i ) {
			if ( StrEqual( queue[i], name ) )
				return;
		}
		queue[nSounds++] = name;
	}
}


bool SoundManager::PopSound( int* _offset, int* _size )
{
	*_offset = 0;
	*_size = 0;
	if ( nSounds == 0 )
		return false;

	const gamedb::Item* data = database->Root()->Child( "data" );

	const gamedb::Reader* reader = gamedb::Reader::GetContext( data );

	int offset = 0;
	int size = 0;
	bool compressed = false;

	const gamedb::Item* sndItem = data->Child( queue[nSounds-1] );
	GLRELASSERT( sndItem );

	if ( sndItem ) {
		sndItem->GetDataInfo( "binary", &offset, &size, &compressed );
		GLRELASSERT( size );
		GLRELASSERT( !compressed );

		if ( size && !compressed ) {
			nSounds--;
			*_offset = offset + reader->OffsetFromStart();
			*_size = size;
			return true;
		}
	}
	return false;
}


/*
void SoundManager::PlayQueuedSounds()
{
	if ( nSounds == 0 )
		return;
	const gamedb::Item* data = database->Root()->Child( "data" );

	const gamedb::Reader* reader = gamedb::Reader::GetContext( data );

	int offset=0;
	int size = 0;
	bool compressed = false;

	for( int i=0; i<nSounds; ++i ) {
		const gamedb::Item* sndItem = data->Child( queue[i] );
		GLRELASSERT( sndItem );

		if ( sndItem ) {
			sndItem->GetDataInfo( "binary", &offset, &size, &compressed );
			GLRELASSERT( size );
			GLRELASSERT( !compressed );

			if ( size && !compressed ) {
				PlayWAVSound( offset + reader->OffsetFromStart(), size );
			}
		}
	}
	nSounds = 0;
}
*/
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
	bool PopSound( int* databaseID, int* offset, int* size );

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
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

#ifndef RENDERQUEUE_INCLUDED
#define RENDERQUEUE_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "enginelimits.h"
#include "vertex.h"

class Model;
struct ModelAtom;

/* 
	The prevailing wisdom for GPU performance is to group the submission by 1)render state
	and 2) texture. In general I question this for tile based GPUs, but it's better to start
	with a queue architecture rather than have to retrofit later.

	RenderQueue queues up everything to be rendered. Flush() commits, and should be called 
	at the end of the render pass. Flush() may also be called automatically if the queues 
	are full.
*/
class RenderQueue
{
public:
	enum {
		ALPHA_BLEND		= 0x01,
	};

	enum {
		MAX_STATE  = 128,
		MAX_MODELS = 1024,
	};

	RenderQueue();
	~RenderQueue();

	void Add( U32 flags, U32 textureID, const Model* model, const ModelAtom* atom );

	// The color is set for everything in the queue (although alpha can be set later.)
	void SetColor( float r, float g, float b )	{ color.Set( r, g, b ); }

	void Flush();
	bool Empty() { return nState == 0 && nModel == 0; }

private:
	struct State {
		U32 flags;
		U32 textureID;
		const ModelAtom* atom;
	};
	struct Item {
		union {
			State state;
			const Model* model;
		};
		Item* nextModel;
	};

	int Compare( const State& s0, const State& s1 ) 
	{
		if ( s0.flags == s1.flags ) 
			if ( s0.textureID == s1.textureID )
				if ( s0.atom == s1.atom ) 
					return 0;
				else if ( s0.atom < s1.atom )
					return -1;
				else
					return 1;
			else if ( s0.textureID < s1.textureID )
				return -1;
			else 
				return 1;
		else if ( s0.flags < s1.flags )
			return -1;
		return 1;
	}

	Item* FindState( const State& state );

	int nState;
	int nModel;
	const grinliz::Matrix4 *textureMatrix;

	void FlushBuffers();
	int nVertex;
	int nIndex;
	grinliz::Vector3F color;

	Item statePool[MAX_STATE];
	Item modelPool[MAX_MODELS];
};


#endif //  RENDERQUEUE_INCLUDED
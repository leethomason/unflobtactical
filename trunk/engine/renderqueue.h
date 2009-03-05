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
		ALPHA_TEST		= 0x02,
	};

	enum {
		MAX_STATE  = 128,
		MAX_MODELS = 1024,

		INDEX_BUFFER_SIZE = 64*1024,
		VERTEX_BUFFER_SIZE = 64*1024
	};

	RenderQueue();
	~RenderQueue();

	void Add( U32 flags, U32 textureID, const Model* model, const ModelAtom* atom );

	void Flush();
	bool Empty() { return nState == 0 && nModel == 0; }

	// Attaches texture 3 coordinates to the vertex data. Used for shadow creation tricks.
	void BindTextureToVertex( bool bind )		{ bindTextureToVertex = bind; }

	int GetTriCount()		{ return triCount; }
	void ClearTriCount()	{ triCount = 0; }

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
	int triCount;
	bool bindTextureToVertex;
	const grinliz::Matrix4 *textureMatrix;

	void FlushBuffers();
	int nVertex;
	int nIndex;

	Item statePool[MAX_STATE];
	Item modelPool[MAX_MODELS];
};


#endif //  RENDERQUEUE_INCLUDED
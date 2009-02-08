#ifndef RENDERQUEUE_INCLUDED
#define RENDERQUEUE_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"

class Model;

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
		ALPHA_TEST = 0x0001,
	};

	enum {
		MAX_STATE = 8,
		MAX_MODELS = 32
	};

	RenderQueue();
	~RenderQueue();

	void Add( U32 flags, U32 textureID, Model* model, int group );
	void Flush();
	bool Empty() { return nState == 0 && nModel == 0; }

private:
	struct Item {
		union {
			struct {
				U32 flags;
				U32 textureID;
			} state;
			struct {
				Model* model;
				int group;
			} model;
		};
		Item* next;
	};

	int Compare( U32 flags0, U32 id0, U32 flags1, U32 id1 ) 
	{
		if ( flags0 < flags1 ) 
			return -1;
		else if ( flags0 > flags1 )
			return 1;
		else {
			if ( id0 < id1 ) 
				return -1;
			else if ( id0 > id1 )
				return 1;
		}
		return 0;
	}

	Item* FindState( U32 flags, U32 textureID );
	

	int nState;
	int nModel;

	Item statePool[MAX_STATE];
	Item modelPool[MAX_MODELS];
};


#endif //  RENDERQUEUE_INCLUDED
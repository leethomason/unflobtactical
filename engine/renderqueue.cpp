#include "renderQueue.h"
#include "platformgl.h"
#include "model.h"                                                     

RenderQueue::RenderQueue()
{
	nState = 0;
	nModel = 0;
}


RenderQueue::~RenderQueue()
{
}


RenderQueue::Item* RenderQueue::FindState( const State& state )
{
	int low = 0;
	int high = nState - 1;
	int answer = -1;
	int mid = 0;
	int insert = 0;		// Where to put the new state. 

	while (low <= high) {
		mid = low + (high - low) / 2;
		int compare = Compare( statePool[mid].state, state );

		if (compare > 0) {
			high = mid - 1;
		}
		else if ( compare < 0) {
			insert = mid;	// since we know the mid is less than the state we want to add,
							// we can move insert up at least to that index.
			low = mid + 1;
		}
		else {
           answer = mid;
		   break;
		}
	}
	if ( answer >= 0 ) {
		return &statePool[answer];
	}
	if ( nState == MAX_STATE ) {
		return 0;
	}

	if ( nState == 0 ) {
		insert = 0;
	}
	else {
		while (    insert < nState
			    && Compare( statePool[insert].state, state ) < 0 ) 
		{
			++insert;
		}
	}

	// move up
	for( int i=nState; i>insert; --i ) {
		statePool[i] = statePool[i-1];
	}
	// and insert
	statePool[insert].state = state;
	statePool[insert].nextModel = 0;
	nState++;

#ifdef DEBUG
	for( int i=0; i<nState-1; ++i ) {
		//GLOUTPUT(( " %d:%d:%x", statePool[i].state.flags, statePool[i].state.textureID, statePool[i].state.atom ));
		GLASSERT( Compare( statePool[i].state, statePool[i+1].state ) < 0 );
	}
	//GLOUTPUT(( "\n" ));
#endif

	return &statePool[insert];
}


void RenderQueue::Add( U32 flags, U32 textureID, const Model* model, const ModelAtom* atom )
{
	if ( nModel == MAX_MODELS ) {
		Flush();
	}

	State s0 = { flags, textureID, atom };

	Item* state = FindState( s0 );
	if ( !state ) {
		Flush();
		state = FindState( s0 );
	}
	GLASSERT( state );

	Item* modelItem = &modelPool[nModel++];
	modelItem->model = model;
	modelItem->nextModel = state->nextModel;
	state->nextModel = modelItem;
}


void RenderQueue::Flush()
{
	U32 flags = (U32)(-1);
	U32 textureID = (U32)(-1);
	const ModelAtom* atom = 0;

	int states = 0;
	int textures = 0;
	int atoms = 0;
	int models = 0;

	//GLOUTPUT(( "RenderQueue::Flush nState=%d nModel=%d\n", nState, nModel ));

	for( int i=0; i<nState; ++i ) {
		// Handle state change.
		if ( flags != statePool[i].state.flags ) {
			flags = statePool[i].state.flags;
			++states;

			switch ( flags ) {
				case 0:
					// do nothing.
					break;
				default:
					GLASSERT( 0 );
					break;
			}
		}

		// Handle texture change.
		if ( textureID != statePool[i].state.textureID ) 
		{
			textureID = statePool[i].state.textureID;
			++textures;

			if ( textureID ) {
				glBindTexture( GL_TEXTURE_2D, textureID );
				GLASSERT( glIsEnabled( GL_TEXTURE_2D ) );
			}
			else {
				GLASSERT( !glIsEnabled( GL_TEXTURE_2D ) );
			}
		}

		// Handle atom change
		if ( atom != statePool[i].state.atom ) {
			atom = statePool[i].state.atom;
			++atoms;

			atom->Bind();
		}

		// Send down the VBOs
		Item* item = statePool[i].nextModel;
		while( item ) {
			++models;
			const Model* model = item->model;
			GLASSERT( model );

			model->PushMatrix();
			atom->Draw();
			model->PopMatrix();

			item = item->nextModel;
		}
	}
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	nState = 0;
	nModel = 0;

	GLOUTPUT(( "states=%d textures=%d atoms=%d models=%d\n", states, textures, atoms, models ));
}



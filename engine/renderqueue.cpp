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


RenderQueue::Item* RenderQueue::FindState( U32 flags, U32 textureID )
{
	int low = 0;
	int high = nState - 1;
	int answer = -1;
	int mid = 0;
	int insert = 0;		// Where to put the new state. 

	while (low <= high) {
		mid = low + (high - low) / 2;
		int compare = Compare( statePool[mid].state.flags, statePool[mid].state.textureID, flags, textureID );

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
			    && Compare( statePool[insert].state.flags, statePool[insert].state.textureID, flags, textureID ) < 0 ) 
		{
			++insert;
		}
	}

	// move up
	for( int i=nState; i>insert; --i ) {
		statePool[i] = statePool[i-1];
	}
	// and insert
	statePool[insert].state.flags = flags;
	statePool[insert].state.textureID = textureID;
	statePool[insert].next = 0;
	nState++;

#ifdef DEBUG
	for( int i=0; i<nState-1; ++i ) {
		GLASSERT( Compare( statePool[i].state.flags, statePool[i].state.textureID, 
						   statePool[i+1].state.flags, statePool[i+1].state.textureID) < 0 );
	}
#endif

	return &statePool[insert];
}


void RenderQueue::Add( U32 flags, U32 textureID, Model* model, int group )
{
	if ( nModel == MAX_MODELS ) {
		Flush();
	}

	Item* state = FindState( flags, textureID );
	if ( !state ) {
		Flush();
		state = FindState( flags, textureID );
	}
	GLASSERT( state );

	Item* modelItem = &modelPool[nModel++];
	modelItem->model.model = model;
	modelItem->model.group = group;
	modelItem->next = state->next;
	state->next = modelItem;
}


void RenderQueue::Flush()
{
	U32 flags = (U32)(-1);
	U32 textureID = (U32)(-1);
	//GLOUTPUT(( "RenderQueue::Flush nState=%d nModel=%d\n", nState, nModel ));

	for( int i=0; i<nState; ++i ) {
		// Handle state change.
		if ( flags != statePool[i].state.flags ) {
			flags = statePool[i].state.flags;
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
			if ( textureID ) {
				glBindTexture( GL_TEXTURE_2D, textureID );
				GLASSERT( glIsEnabled( GL_TEXTURE_2D ) );
			}
			else {
				GLASSERT( !glIsEnabled( GL_TEXTURE_2D ) );
			}
		}

		// Send down the VBOs
		Item* item = statePool[i].next;
		while( item ) {
			Model* model = item->model.model;
			if ( model ) {
				model->DrawLow( item->model.group );
			}
			item = item->next;
		}
	}
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	nState = 0;
	nModel = 0;
}



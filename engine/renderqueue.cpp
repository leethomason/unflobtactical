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

#include "renderQueue.h"
#include "platformgl.h"
#include "model.h"    

#include "../grinliz/glperformance.h"

using namespace grinliz;

RenderQueue::RenderQueue()
{
	nState = 0;
	nModel = 0;
	//triCount = 0;
	//bindTextureToVertex = false;
	textureMatrix = 0;
	color.Set( 1.0f, 1.0f, 1.0f );
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

	//triCount += atom->nIndex / 3;
}


void RenderQueue::Flush()
{
	GRINLIZ_PERFTRACK	

	U32 flags = (U32)(-1);
	U32 textureID = 0;
	const ModelAtom* atom = 0;

	int states = 0;
	int textures = 0;
	int atoms = 0;
	int models = 0;

	glColor4f( color.x, color.y, color.z, 1.0f );

	//GLOUTPUT(( "RenderQueue::Flush nState=%d nModel=%d\n", nState, nModel ));

	for( int i=0; i<nState; ++i ) {
		// Handle state change.
		if ( flags != statePool[i].state.flags ) {
			
			flags = statePool[i].state.flags;
			++states;

			if ( flags & ALPHA_BLEND) {
				// FIXME: clean this up.
				// When should it blend? When should it test? What's the performance
				// impact of blend vs. test.
#ifdef MAPMAKER
				// map preview
				glEnable( GL_BLEND );
				glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
#else
				// plants
				glEnable( GL_ALPHA_TEST );
				glAlphaFunc( GL_GREATER, 0.5f );
#endif
			}
			else {
#ifdef MAPMAKER
				glDisable( GL_BLEND );
#else
				glDisable( GL_ALPHA_TEST );
#endif
			}
		}

		// Handle texture change.
		if ( textureID != statePool[i].state.textureID ) 
		{
			//GLASSERT( bindTextureToVertex == false );
			textureID = statePool[i].state.textureID;
			++textures;

			if ( textureID ) {
				glBindTexture( GL_TEXTURE_2D, textureID );
				GLASSERT( glIsEnabled( GL_TEXTURE_2D ) );
			}
		}

		// Handle atom change
		if ( atom != statePool[i].state.atom ) {
			atom = statePool[i].state.atom;
			++atoms;

			atom->Bind( /*bindTextureToVertex*/ );
		}

		// Send down the VBOs
		Item* item = statePool[i].nextModel;
		while( item ) {
			++models;
			const Model* model = item->model;
			GLASSERT( model );

			model->PushMatrix( /*bindTextureToVertex*/ );
			atom->Draw();
			model->PopMatrix( /*bindTextureToVertex*/ );
			item = item->nextModel;
		}
	}
#ifdef EL_USE_VBO
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
#endif
	nState = 0;
	nModel = 0;
	glDisable( GL_ALPHA_TEST );
	glDisable( GL_BLEND );

//	GLOUTPUT(( "states=%d textures=%d atoms=%d models=%d\n", states, textures, atoms, models ));
}

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
#include "texture.h"

#include "../grinliz/glperformance.h"

using namespace grinliz;

RenderQueue::RenderQueue()
{
	nState = 0;
	nItem = 0;
}


RenderQueue::~RenderQueue()
{
}


RenderQueue::State* RenderQueue::FindState( const State& state )
{
	int low = 0;
	int high = nState - 1;
	int answer = -1;
	int mid = 0;
	int insert = 0;		// Where to put the new state. 

	while (low <= high) {
		mid = low + (high - low) / 2;
		int compare = Compare( statePool[mid], state );

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
			    && Compare( statePool[insert], state ) < 0 ) 
		{
			++insert;
		}
	}

	// move up
	for( int i=nState; i>insert; --i ) {
		statePool[i] = statePool[i-1];
	}
	// and insert
	statePool[insert] = state;
	statePool[insert].root = 0;
	nState++;

#ifdef DEBUG
	for( int i=0; i<nState-1; ++i ) {
		//GLOUTPUT(( " %d:%d:%x", statePool[i].state.flags, statePool[i].state.textureID, statePool[i].state.atom ));
		GLASSERT( Compare( statePool[i], statePool[i+1] ) < 0 );
	}
	//GLOUTPUT(( "\n" ));
#endif

	return &statePool[insert];
}


void RenderQueue::Add( Model* model, const ModelAtom* atom )
{
	if ( nItem == MAX_ITEMS ) {
		GLASSERT( 0 );
		return;
	}

	int flags = 0;
	if ( atom->texture->Alpha() )
		flags |= FLAG_ALPHA;

	State s0 = { flags, atom->texture, 0 };

	State* state = FindState( s0 );
	if ( !state ) {
		GLASSERT( 0 );
		return;
	}

	Item* item = &itemPool[nItem++];
	item->model = model;
	item->atom = atom;

	item->next  = state->root;
	state->root = item;
}

/*
void RenderQueue::Prepare()
{
	int count = 0;
	for( int i=0; i<nState; ++i ) {
		for( Item* item = statePool[i].root; item; item=item->next ) {

		}
	}
}
*/


void RenderQueue::Submit( int mode, int required, int excluded, float bbRotation )
{
	GRINLIZ_PERFTRACK	

	U32 flags = (U32)(-1);
	Texture* texture = 0;
	//GLOUTPUT(( "RenderQueue::Flush nState=%d nModel=%d\n", nState, nModel ));

	for( int i=0; i<nState; ++i ) {
		
		// Handle state change.
		if ( flags != statePool[i].flags ) {
			
			flags = statePool[i].flags;

			if ( !(mode & MODE_IGNORE_ALPHA )) {
				if ( flags & FLAG_ALPHA) {
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
		}

		if ( !(mode & MODE_IGNORE_TEXTURE) ) {
			// Handle texture change.
			if ( texture != statePool[i].texture ) 
			{
				//GLASSERT( bindTextureToVertex == false );
				texture = statePool[i].texture;

				if ( texture ) {
					glBindTexture( GL_TEXTURE_2D, texture->GLID() );
					GLASSERT( glIsEnabled( GL_TEXTURE_2D ) );
				}
			}
		}

		for( Item* item = statePool[i].root; item; item=item->next ) 
		{
			Model* model = item->model;
			int modelFlags = model->Flags();

			if (    ( (required & modelFlags) == required)
				 && ( (excluded & modelFlags) == 0 ) )
			{
				if ( model->IsBillboard() )
					model->SetRotation( bbRotation );

				item->atom->Bind();

				const Model* model = item->model;
				GLASSERT( model );

				if ( mode & MODE_PLANAR_SHADOW ) {
					// Override the texture part of the Bind(), above.
					glTexCoordPointer( 3, GL_FLOAT, sizeof(Vertex), (const U8*)item->atom->vertex + Vertex::POS_OFFSET); 

					// Push the xform matrix to the texture and the model view.
					glMatrixMode( GL_TEXTURE );
					model->PushMatrix( false );
					glMatrixMode( GL_MODELVIEW );
					model->PushMatrix( false );

					item->atom->Draw();

					// Unravel all that. Not general purpose code. Call OpenGL directly.
					glMatrixMode( GL_TEXTURE );
					glPopMatrix();
					glMatrixMode( GL_MODELVIEW );
					glPopMatrix();
				}
				else {
					model->PushMatrix( true );
					item->atom->Draw();
					model->PopMatrix( true );
				}
			}
		}
	}
#ifdef EL_USE_VBO
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
#endif

	// Clean up the alpha state if we twiddled with it.
	if ( !(mode & MODE_IGNORE_ALPHA )) {
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glDisable( GL_ALPHA_TEST );
	}
}

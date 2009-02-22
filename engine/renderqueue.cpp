#include "renderQueue.h"
#include "platformgl.h"
#include "model.h"    

using namespace grinliz;

RenderQueue::RenderQueue()
{
	nState = 0;
	nModel = 0;
	triCount = 0;
	bindTextureToVertex = false;
	textureMatrix = 0;
//#if (EL_BATCH_VERTICES==1)
//	nVertex = 0;
//	nIndex = 0;
//#endif
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

	triCount += atom->nIndex / 3;
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

/*
#if (EL_BATCH_VERTICES==1)
	GLASSERT( nVertex == 0 );
	GLASSERT( nIndex == 0 );
	U8* vPtr = (U8*)vertexBuffer + Vertex::POS_OFFSET;
	U8* nPtr = (U8*)vertexBuffer + Vertex::NORMAL_OFFSET;
	U8* tPtr = (U8*)vertexBuffer + Vertex::TEXTURE_OFFSET;

	glVertexPointer(   3, GL_FLOAT, sizeof(Vertex), (const GLvoid*)vPtr);
	glNormalPointer(      GL_FLOAT, sizeof(Vertex), (const GLvoid*)nPtr);		
	glTexCoordPointer( 2, GL_FLOAT, sizeof(Vertex), (const GLvoid*)tPtr);  
	Matrix4 m, r;
#endif
	*/

	//GLOUTPUT(( "RenderQueue::Flush nState=%d nModel=%d\n", nState, nModel ));

	for( int i=0; i<nState; ++i ) {
		// Handle state change.
		if ( flags != statePool[i].state.flags ) {
			
//#if (EL_BATCH_VERTICES==1)
//			FlushBuffers();
//#endif

			flags = statePool[i].state.flags;
			++states;

			if ( flags & ALPHA_TEST ) {
				//glEnable( GL_ALPHA_TEST );
				//glAlphaFunc( GL_GEQUAL, 0.5f );
				glEnable( GL_BLEND );
				glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
			}
			else {
				//glDisable( GL_ALPHA_TEST );
				glDisable( GL_BLEND );
			}
		}

		// Handle texture change.
		if ( textureID != statePool[i].state.textureID ) 
		{
//#if (EL_BATCH_VERTICES==1)
//			FlushBuffers();
//#endif

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

//#if (EL_BATCH_VERTICES==0)
			atom->Bind( bindTextureToVertex );
//#endif
		}

		// Send down the VBOs
		Item* item = statePool[i].nextModel;
		while( item ) {
			++models;
			const Model* model = item->model;
			GLASSERT( model );

//#if (EL_BATCH_VERTICES==0)
			model->PushMatrix( textureMatrix );
			atom->Draw();
			model->PopMatrix( textureMatrix );
//#else
/*
			if (    nIndex + atom->nIndex > INDEX_BUFFER_SIZE
				 || nVertex + atom->nVertex > VERTEX_BUFFER_SIZE )
			{
				FlushBuffers();
			}
			GLASSERT( nIndex + atom->nIndex      < INDEX_BUFFER_SIZE 
				      && nVertex + atom->nVertex < VERTEX_BUFFER_SIZE );

			model->GetXForm( &m, &r );
			U16 offset = (U16)nVertex;

			for( unsigned k=0; k<atom->nVertex; ++k ) {
				vertexBuffer[nVertex].pos		= m * atom->vertex[k].pos;
				vertexBuffer[nVertex].normal	= r * atom->vertex[k].normal;
				vertexBuffer[nVertex].tex		= atom->vertex[k].tex;
				++nVertex;
			}

			for( unsigned k=0; k<atom->nIndex; ++k ) {
				indexBuffer[nIndex] = atom->index[k] + offset;
				++nIndex;
			}
*/
//#endif

			item = item->nextModel;
		}
	}
//#if (EL_BATCH_VERTICES==0)
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
//#else
//	FlushBuffers();
//#endif
	nState = 0;
	nModel = 0;
	glDisable( GL_ALPHA_TEST );
	glDisable( GL_BLEND );

//	GLOUTPUT(( "states=%d textures=%d atoms=%d models=%d\n", states, textures, atoms, models ));
}

/*
#if (EL_BATCH_VERTICES==1)
void RenderQueue::FlushBuffers()
{
	if ( nIndex > 0 ) {
		glDrawElements( GL_TRIANGLES, 
						nIndex,
						GL_UNSIGNED_SHORT, 
						indexBuffer );
	}
	nIndex = 0;
	nVertex = 0;
}
#endif
*/
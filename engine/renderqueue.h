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
#include "gpustatemanager.h"
#include "ufoutil.h"

class Model;
struct ModelAtom;
class Texture;


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
		MAX_STATE  = 128,
		MAX_ITEMS  = 1024,
	};

	RenderQueue();
	~RenderQueue();
	void Add(	Model* model,					// Can be chaned: billboard rotation will be set.
				const ModelAtom* atom, 
				GPUShader* shader,
				const grinliz::Matrix4* textureXForm,
				Texture* replaceAllTextures,
				const grinliz::Vector4F* param );

	enum {
		MODE_PLANAR_SHADOW				= 0x01,		// Do all the fancy tricks to create planar shadows.
	};

	/* If a shader is passed it, it will override the shader set by the Add. */
	void Submit( GPUShader* shader, int mode, int required, int excluded );
	bool Empty() { return nState == 0 && nItem == 0; }
	void Clear() { nState = 0; nItem = 0; }

	int VertexCacheSize() const { return vertexCacheSize; }
	int VertexCacheCap() const	{ return vertexCacheCap; }
	void ResetRenderCache();

private:
	struct Item {
		Model*					model;
		const ModelAtom*		atom;
		int						atomIndex;
		const grinliz::Matrix4*	textureXForm;
		grinliz::Vector4F		param;
		Item*					next;
	};

	struct State {
		GPUShader*			shader;
		Texture*			texture;
		Item*				root;
	};

	static int Compare( const State& s0, const State& s1 ) 
	{
		if ( s0.shader == s1.shader ) {
			if ( s0.texture == s1.texture )
				return 0;
			return ( s0.texture < s1.texture ) ? -1 : 1;
		}
		if ( s0.shader->SortOrder() == s1.shader->SortOrder() ) {
			return s0.shader < s1.shader ? -1 : 1;
		}
		return s0.shader->SortOrder() - s1.shader->SortOrder();
	}

	void CacheAtom( Model* model, int atomIndex );

	State* FindState( const State& state );

	int nState;
	int nItem;

	GPUVertexBuffer vertexCache;
	int vertexCacheSize;
	int vertexCacheCap;

	CDynArray<Vertex> vertexBuf;
	CDynArray<U16>    indexBuf;

	State statePool[MAX_STATE];
	Item itemPool[MAX_ITEMS];
};


#endif //  RENDERQUEUE_INCLUDED
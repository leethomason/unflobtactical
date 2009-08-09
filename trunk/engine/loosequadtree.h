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

#ifndef LOOSEQUADTREE_INCLUDED
#define LOOSEQUADTREE_INCLUDED

#include "enginelimits.h"
#include "model.h"
#include "../grinliz/glmemorypool.h"

/*
	A loose quad tree for culling models. Also stores all the models in the world,
	and models are allocated and free'd from the Tree.

*/
class SpaceTree
{

public:
	SpaceTree( float yMin, float yMax );
	~SpaceTree();

	Model* AllocModel( const ModelResource* );
	void   FreeModel( Model* );

	// Called whenever a model moves. (Usually called automatically be the model.)
	void   Update( Model* );

	// Returns all the models in the planes.
	Model* Query( const grinliz::Plane* planes, int nPlanes, int requiredFlags, int excludedFlags, bool debug=false );

	// Returns the FIRST model impacted.
	Model* QueryRay( const grinliz::Vector3F& origin, const grinliz::Vector3F& direction, 
					 int required, int excluded,
					 HitTestMethod method,
					 grinliz::Vector3F* intersection );

#ifdef DEBUG
	// Draws debugging info about the spacetree.
	void Draw();
#endif

private:

	struct Item {
		Model model;	// Must be first! Gets cast back to Item in destructor.
		Item* next;		// used in the node list.
		Item* prev;

		void Unlink() {
			if ( next ) {
				GLASSERT( prev );
				next->prev = prev;
				prev->next = next;
				prev = next = 0;
			}
			else {
				GLASSERT( !prev );
			}
		}
		void Link( Item* after ) {
			after->next->prev = this;
			this->next = after->next;
			after->next = this;
			this->prev = after;
		}
	};

	struct Node
	{
		int x, z;
		int size;
		int looseX, looseZ;
		int looseSize;
		int depth;
		int queryID;
		Item sentinel;
		Node* parent;
		Node* child[4];

#ifdef DEBUG
		mutable int hit;
#endif

		void CalcAABB( grinliz::Rectangle3F* aabb, const float yMin, const float yMax ) const;
	};

	void InitNode();
	void QueryPlanesRec( const grinliz::Plane* planes, int nPlanes, int intersection, const Node* node, U32  );

	Model* modelRoot;
	float yMin, yMax;

	int nodesVisited;
	int planesComputed;
	int spheresComputed;
	int modelsFound;

	int requiredFlags;
	int excludedFlags;
	int queryID;
	bool debug;

	grinliz::MemoryPool modelPool;

	enum {
		DEPTH = 5,
		NUM_NODES = 1+4+16+64+256
	};

	Node* GetNode( int depth, int x, int z ); 

	int nodeAddedAtDepth[DEPTH];
	Node nodeArr[NUM_NODES];
};

#endif // LOOSEQUADTREE_INCLUDED
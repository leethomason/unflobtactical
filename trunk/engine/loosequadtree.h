#ifndef LOOSEQUADTREE_INCLUDED
#define LOOSEQUADTREE_INCLUDED

#include "model.h"
#include "fixedgeom.h"

class SpaceTree
{

public:
	SpaceTree( grinliz::Fixed yMin, grinliz::Fixed yMax );
	~SpaceTree();

	Model* AllocModel( ModelResource* );
	void   FreeModel( Model* );

	void   Update( Model* );

	Model* Query( const PlaneX* planes, int nPlanes );
	Model* Query( const Vector3X& origin, const Vector3X& direction );

#ifdef DEBUG
	void Draw();
#endif

private:
	struct Item {
		Model model;	// Must be first! Gets cast back to Item in destructor.
		Item* next;
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
		Item sentinel;
		Node* child[4];

#ifdef DEBUG
		mutable int hit;
#endif

		void CalcAABB( Rectangle3X* aabb, const grinliz::Fixed yMin, const grinliz::Fixed yMax ) const;
	};

	void InitNode();
	void QueryPlanesRec( const PlaneX* planes, int nPlanes, int intersection, const Node* node, U32  );
	void QueryPlanesRec( const Vector3X& origin, const Vector3X& direction, int intersection, const Node* node );

	Item freeMemSentinel;
	int allocated;
	Model* modelRoot;
	grinliz::Fixed yMin, yMax;

	int nodesVisited;
	int planesComputed;
	int spheresComputed;
	int modelsFound;

	Item modelPool[EL_MAX_MODELS];
	enum {
		// Depth 6 dropped the count from 13.8K to 13.5K tris. Not worth it.
		//DEPTH = 6,
		//NUM_NODES = 1+4+16+64+256+1024
		DEPTH = 5,
		NUM_NODES = 1+4+16+64+256
	};

	Node* GetNode( int depth, int x, int z ); 

	int nodeAddedAtDepth[DEPTH];
	Node nodeArr[NUM_NODES];
};

#endif // LOOSEQUADTREE_INCLUDED
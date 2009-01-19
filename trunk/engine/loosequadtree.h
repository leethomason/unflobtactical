#ifndef LOOSEQUADTREE_INCLUDED
#define LOOSEQUADTREE_INCLUDED

#include "model.h"

class SpaceTree
{
	friend class TreeLock;

public:
	SpaceTree( FIXED yMin, FIXED yMax );
	~SpaceTree();

	Model* AllocModel( ModelResource* );
	void   FreeModel( Model* );

	void   Update( Model* );

	Model* Query( const PlaneX* planes, int nPlanes );
	Model* Query( const Vector3X& origin, const Vector3X& direction );

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
		int x, y;
		int size;
		int looseX, looseY;
		int looseSize;
		int depth;
		Item sentinel;
		Node* child[4];
	};

	void InitNode();
	void QueryPlanesRec( const PlaneX* planes, int nPlanes, int intersection, const Node* node );
	void QueryPlanesRec( const Vector3X& origin, const Vector3X& direction, int intersection, const Node* node );

	Item freeMemSentinel;
	int allocated;
	Model* modelRoot;
	FIXED yMin, yMax;

	int nodesChecked;
	int modelsFound;

	Item modelPool[EL_MAX_MODELS];
	enum { 
		DEPTH = 5,
		NUM_NODES = 1+4+16+64+256
	};
	Node* GetNode( int depth, int nx, int ny ) 
	{
		GLASSERT( depth >=0 && depth < DEPTH );

		const int base[] = { 0, 1, 5, 21, 85 };
		int dx = (1<<depth);
		GLASSERT( nx < dx );
		GLASSERT( ny < dx );

		Node* result = &nodeArr[ base[depth] + ny*dx+nx ];
		GLASSERT( result >= nodeArr && result < &nodeArr[NUM_NODES] );
		return result;
	}
	Node nodeArr[NUM_NODES];
};

/*
class TreeLock
{
public:
	TreeLock( SpaceTree* tree ) 	{ 
		this->tree = tree;
		GLASSERT( !tree->locked );
		tree->locked = true;
	}
	~TreeLock() {
		tree->locked = false;
	}

private:
	SpaceTree* tree;
};
*/
#endif // LOOSEQUADTREE_INCLUDED
#include "loosequadtree.h"
#include "map.h"
using namespace grinliz;

SpaceTree::SpaceTree( Fixed yMin, Fixed yMax )
{
	allocated = 0;
	this->yMin = yMin;
	this->yMax = yMax;

	freeMemSentinel.next = &freeMemSentinel;
	freeMemSentinel.prev = &freeMemSentinel;

	for( int i=0; i<EL_MAX_MODELS; ++i ) {
		modelPool[i].Link( &freeMemSentinel );
	}
	InitNode();
}


SpaceTree::~SpaceTree()
{
#ifdef DEBUG
	// Un-released models?
	int count = 0;

	for( Item* item=freeMemSentinel.next; item != &freeMemSentinel; item=item->next ) {
		++count;
	}
	GLASSERT( count == EL_MAX_MODELS );
#endif
}


void SpaceTree::InitNode()
{
	int depth = 0;

	while ( depth < DEPTH ) 
	{
		int dx = 1<<depth;
		for( int j=0; j<dx; ++j ) 
		{
			for( int i=0; i<dx; ++i ) 
			{
				int nodeSize = Map::SIZE / dx;
				Node* node = GetNode( depth, i, j );
				node->x = i*nodeSize;
				node->y = j*nodeSize;
				node->size = nodeSize;

				node->looseX = node->x - nodeSize/2;
				node->looseY = node->y - nodeSize/2;
				node->looseSize = nodeSize*2;

				node->depth = depth;
				GLASSERT( node->sentinel.model.Sentinel() );
				node->sentinel.next = &node->sentinel;
				node->sentinel.prev = &node->sentinel;

				if ( depth+1 < DEPTH ) {
					node->child[0] = GetNode( depth+1, i*2, j*2 );
					node->child[1] = GetNode( depth+1, i*2+1, j*2 );
					node->child[2] = GetNode( depth+1, i*2, j*2+1 );
					node->child[3] = GetNode( depth+1, i*2+1, j*2+1 );
				}
				else { 
					node->child[0] = 0;
					node->child[1] = 0;
					node->child[2] = 0;
					node->child[3] = 0;
				}
			}
		}
		++depth;
	}
}


Model* SpaceTree::AllocModel( ModelResource* resource )
{
	GLASSERT( resource );
	GLASSERT( allocated < EL_MAX_MODELS );
	if ( allocated == EL_MAX_MODELS ) {
		return 0;
	}

	GLASSERT( freeMemSentinel.next != &freeMemSentinel );

	Item* item = freeMemSentinel.next;
	GLASSERT( item->next && item->prev );
	item->Unlink();

	item->next = 0;	// very important to clear pointers before Init() - which will cause Link to occur.
	item->prev = 0;
	item->model.Init( resource, this );
	return &item->model;
}


void SpaceTree::FreeModel( Model* model )
{
	Item* item = (Item*)model;	// cast depends on model being first in the structure.
	GLASSERT( item >= &modelPool[0] );
	GLASSERT( item < &modelPool[EL_MAX_MODELS] );

	item->Unlink();
	item->Link( &freeMemSentinel );
}


void SpaceTree::Update( Model* model )
{
	// Unlink if currently in tree.
	Item* item = (Item*)model;	// cast depends on model being first in the structure.
	GLASSERT( item >= &modelPool[0] );
	GLASSERT( item < &modelPool[EL_MAX_MODELS] );

	// circular list - this always works.
	GLASSERT( !model->Sentinel() );
	item->Unlink();

	// Get basics.
	SphereX bSphereX;
	model->CalcBoundSphere( &bSphereX );
	int modelSize = bSphereX.radius.Ceil();

	int x = bSphereX.origin.x;
	int z = bSphereX.origin.z;

	/* 
	I've used a scheme which is like an octree, but tweaked to make it
	easy to move objects.  Basically, I used a fixed level of subdivision
	and pre-allocated (empty) nodes down to that level.  The main tweak
	was to make the nodes "loose" by overlapping them -- so a node which
	in a normal octree would be bounded by an N x N x N cube, I defined as
	being bounded by a 2N x 2N x 2N cube instead, overlapping with the
	neighboring nodes.  It's still a hierarchical partitioning scheme,
	it's just looser than a normal octree.

	I used bounding spheres on the objects.  An object's bounding radius
	completely determines its grid level, and then the particular node
	only depends on the object's position.  I did maintain an "empty" flag
	for each node, since if you do this in 3D, you usually end up with
	lots of empty nodes.

	So the advantage is that moving an object is fairly quick and doesn't
	involved any allocation/deallocation.  The main disadvantages are that
	the bounds are relatively loose, and you need N^3 nodes at the finest
	subdivision level.  Still, it worked pretty well for me.

	I wrote a longer description of this some time back, probably in
	rec.games.programmer.  Check DejaNews for more info.

	--
	Thatcher Ulrich
	http://world.std.com/~ulrich
	*/

	// compute the depth
	int size = Map::SIZE;
	int depth = 0;

	// Handle big or out of range:
	if ( x < 0 || x >= Map::SIZE || z < 0 || z >= Map::SIZE || modelSize >= Map::SIZE ) {
		// stay on top
		x = z = 0;
	}
	else {
		while ( modelSize <= ( size >> 1 ) ) { 
			++depth;
			size /= 2;
			if ( depth == DEPTH-1 ) {
				break;
			}
		}
	}
	int nx = x / size;
	int nz = z / size;
	Node* node = GetNode( depth, nx, nz );

	// Link it in.
	item->Link( &node->sentinel );
}


Model* SpaceTree::Query( const PlaneX* planes, int nPlanes )
{
	modelRoot = 0;
	nodesChecked = 0;
	modelsFound = 0;

	QueryPlanesRec( planes, nPlanes, grinliz::INTERSECT, &nodeArr[0] );
	//GLOUTPUT(( "Query %d/%d nodes, models=%d\n", nodesChecked, NUM_NODES, modelsFound ));
	return modelRoot;
}


Model* SpaceTree::Query( const Vector3X& origin, const Vector3X& direction )
{
	modelRoot = 0;
	nodesChecked = 0;
	modelsFound = 0;

	QueryPlanesRec( origin, direction, grinliz::INTERSECT, &nodeArr[0] );
	GLOUTPUT(( "Query %d/%d nodes, models=%d\n", nodesChecked, NUM_NODES, modelsFound ));
	return modelRoot;

}


void SpaceTree::QueryPlanesRec(	const PlaneX* planes, int nPlanes, int intersection, const Node* node )
{
	bool callChildrenAndAddModels = false;

	if ( intersection == grinliz::POSITIVE ) 
	{
		// we are fully inside, and don't need to check.
		callChildrenAndAddModels = true;
		++nodesChecked;
	}
	else if ( intersection == grinliz::INTERSECT ) 
	{
		Rectangle3X aabb;
		aabb.Set( Fixed( node->looseX ), yMin, Fixed( node->looseY ),
				  Fixed( node->looseX + node->looseSize ), yMax, Fixed( node->looseY + node->looseSize ) );
		
		int intersection = grinliz::POSITIVE;
		for( int i=0; i<nPlanes; ++i ) {
			int comp = ComparePlaneAABBX( planes[i], aabb );
			if ( comp == grinliz::NEGATIVE ) {
				intersection = grinliz::NEGATIVE;
				break;
			}
			else if ( comp == grinliz::INTERSECT ) {
				intersection = grinliz::INTERSECT;
			}
		}
		if ( intersection != grinliz::NEGATIVE ) {
			callChildrenAndAddModels = true;
		}
		++nodesChecked;
	}
	if ( callChildrenAndAddModels ) 
	{
		for( Item* item=node->sentinel.next; item != &node->sentinel; item=item->next ) 
		{
			Model* m = &item->model;
			m->next = modelRoot;
			modelRoot = m;
			++modelsFound;
		}
		
		if ( node->depth+1<DEPTH)  {
			QueryPlanesRec( planes, nPlanes, intersection, node->child[0] );
			QueryPlanesRec( planes, nPlanes, intersection, node->child[1] );
			QueryPlanesRec( planes, nPlanes, intersection, node->child[2] );
			QueryPlanesRec( planes, nPlanes, intersection, node->child[3] );
		}
	}
}


void SpaceTree::QueryPlanesRec(	const Vector3X& origin, const Vector3X& direction, int intersection, const Node* node )
{
	bool callChildrenAndAddModels = false;

	// Note there isn't an obvious way for this to be positive. Rays always INTERSECT,
	// unless you do something tricky to detect special cases.

	if ( intersection == grinliz::POSITIVE ) 
	{
		// we are fully inside, and don't need to check.
		callChildrenAndAddModels = true;
		++nodesChecked;
	}
	else if ( intersection == grinliz::INTERSECT ) 
	{
		Rectangle3X aabb;
		aabb.Set( Fixed( node->looseX ), yMin, Fixed( node->looseY ),
				  Fixed( node->looseX + node->looseSize ), yMax, Fixed( node->looseY + node->looseSize ) );
		//GLOUTPUT(( "  l=%d rect: ", node->depth )); DumpRectangle( aabb ); GLOUTPUT(( "\n" ));
		
		Vector3X intersect;
		Fixed t;

		int comp = IntersectRayAABBX( origin, direction, aabb, &intersect, &t );
		if ( comp == grinliz::INTERSECT || comp == grinliz::INSIDE ) {
			intersection = grinliz::INTERSECT;
			callChildrenAndAddModels = true;
		}
		++nodesChecked;
	}
	if ( callChildrenAndAddModels ) 
	{
		for( Item* item=node->sentinel.next; item != &node->sentinel; item=item->next ) 
		{
			Model* m = &item->model;
			m->next = modelRoot;
			modelRoot = m;
			++modelsFound;
		}
		
		if ( node->depth+1<DEPTH)  {
			QueryPlanesRec( origin, direction, intersection, node->child[0] );
			QueryPlanesRec( origin, direction, intersection, node->child[1] );
			QueryPlanesRec( origin, direction, intersection, node->child[2] );
			QueryPlanesRec( origin, direction, intersection, node->child[3] );
		}
	}
}

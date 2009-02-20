#include "loosequadtree.h"
#include "map.h"

#ifdef DEBUG
#include "platformgl.h"
#endif
using namespace grinliz;

/*
	Tuning:
	Unit tree:  Query checked=153, computed=145 of 341 nodes, models=347 [35,44,40,26,606]
	Classic:	Query checked=209, computed=209 of 341 nodes, models=361 [1,36,36,51,627]
	Unit tree has less compution and fewer models, slightly less balanced. 
*/

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
	for( int i=0; i<DEPTH; ++i )
		nodeAddedAtDepth[i] = 0;
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
	int nodeSize = Map::SIZE;

	while ( depth < DEPTH ) 
	{
		for( int j=0; j<Map::SIZE; j+=nodeSize ) 
		{
			for( int i=0; i<Map::SIZE; i+=nodeSize ) 
			{
				Node* node = GetNode( depth, i, j );
				node->x = i;
				node->z = j;
				node->size = nodeSize;

				/*
				// looseSize = size*2, "classic" tree.
				node->looseX = node->x - nodeSize/2;
				node->looseZ = node->z - nodeSize/2;
				node->looseSize = nodeSize*2;
				*/

				// Most objects are 1x1. Take advantage of that:
				node->looseX = node->x - 1;
				node->looseZ = node->z - 1;
				node->looseSize = nodeSize+2;

				node->depth = depth;
				GLASSERT( node->sentinel.model.Sentinel() );
				node->sentinel.next = &node->sentinel;
				node->sentinel.prev = &node->sentinel;

				if ( depth+1 < DEPTH ) {
					node->child[0] = GetNode( depth+1, i,				j );
					node->child[1] = GetNode( depth+1, i+nodeSize/2,	j );
					node->child[2] = GetNode( depth+1, i,				j+nodeSize/2 );
					node->child[3] = GetNode( depth+1, i+nodeSize/2,	j+nodeSize/2 );
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
		nodeSize >>= 1;
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

	allocated++;
	//GLOUTPUT(( "Alloc model: %d/%d\n", allocated, EL_MAX_MODELS ));
	return &item->model;
}


void SpaceTree::FreeModel( Model* model )
{
	Item* item = (Item*)model;	// cast depends on model being first in the structure.
	GLASSERT( item >= &modelPool[0] );
	GLASSERT( item < &modelPool[EL_MAX_MODELS] );

	item->Unlink();
	item->Link( &freeMemSentinel );

	allocated--;
	//GLOUTPUT(( "Free model: %d/%d\n", allocated, EL_MAX_MODELS ));
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
	CircleX circlex;
	model->CalcBoundCircle( &circlex );
	//int modelSize = circlex.radius.Ceil();

	int x = circlex.origin.x;
	int z = circlex.origin.y;

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

	// Since the tree is somewhat modified from the ideal, start with the 
	// most idea node and work up. Note that everything fits at the top node.
	int depth = DEPTH-1;

	Node* node = 0;
	while( depth > 0 ) {
		node = GetNode( depth, x, z );
		if (    circlex.origin.x - circlex.radius >= Fixed( node->looseX )
			 && circlex.origin.y - circlex.radius >= Fixed( node->looseZ )
			 && circlex.origin.x + circlex.radius <= Fixed( node->looseX + node->looseSize )
			 && circlex.origin.y + circlex.radius <= Fixed( node->looseX + node->looseSize ) )
		{
			// fits.
			break;
		}
		--depth;
	}
	++nodeAddedAtDepth[depth];

#ifdef DEBUG
	if ( depth > 0 ) {
		CircleX c;
		Rectangle3X aabb;
		node->CalcAABB( &aabb, yMin, yMax );

		model->CalcBoundCircle( &c );

		GLASSERT( c.origin.x - c.radius >= aabb.min.x );
		GLASSERT( c.origin.y - c.radius >= aabb.min.z );
		GLASSERT( c.origin.x + c.radius <= aabb.max.x );
		GLASSERT( c.origin.y + c.radius <= aabb.max.z );
	}
#endif

	// Link it in.
	item->Link( &node->sentinel );
}



SpaceTree::Node* SpaceTree::GetNode( int depth, int x, int z )
{
	GLASSERT( depth >=0 && depth < DEPTH );
	int size = Map::SIZE >> depth;
	int nx = x / size;	// FIXME: do it all with shifts
	int nz = z / size;

	const int base[] = { 0, 1, 5, 21, 85 };
	int dx = (1<<depth);
	GLASSERT( nx < dx );
	GLASSERT( nz < dx );

	Node* result = &nodeArr[ base[depth] + nz*dx+nx ];
	GLASSERT( result >= nodeArr && result < &nodeArr[NUM_NODES] );
	return result;
}



Model* SpaceTree::Query( const PlaneX* planes, int nPlanes )
{
	modelRoot = 0;
	nodesChecked = 0;
	nodesComputed = 0;
	modelsFound = 0;

#ifdef DEBUG
	for( int i=0; i<NUM_NODES; ++i ) {
		nodeArr[i].hit = 0;
	}
#endif

	QueryPlanesRec( planes, nPlanes, grinliz::INTERSECT, &nodeArr[0] );
	/*
	GLOUTPUT(( "Query checked=%d, computed=%d of %d nodes, models=%d [%d,%d,%d,%d,%d]\n", nodesChecked, nodesComputed, NUM_NODES, modelsFound,
				nodeAddedAtDepth[0],
				nodeAddedAtDepth[1],
				nodeAddedAtDepth[2],
				nodeAddedAtDepth[3],
				nodeAddedAtDepth[4] ));
	*/
	return modelRoot;
}


Model* SpaceTree::Query( const Vector3X& origin, const Vector3X& direction )
{
	modelRoot = 0;
	nodesChecked = 0;
	modelsFound = 0;

	QueryPlanesRec( origin, direction, grinliz::INTERSECT, &nodeArr[0] );
	//GLOUTPUT(( "Query %d/%d nodes, models=%d\n", nodesChecked, NUM_NODES, modelsFound ));
	return modelRoot;

}


void SpaceTree::Node::CalcAABB( Rectangle3X* aabb, const Fixed yMin, const Fixed yMax ) const
{
	GLASSERT( yMin < yMax );
	GLASSERT( looseSize > 0 );

	aabb->Set(	Fixed( looseX ), 
				yMin, 
				Fixed( looseZ ),
				
				Fixed( looseX + looseSize ), 
				yMax, 
				Fixed( looseZ + looseSize ) );
/*
	aabb->Set(	Fixed( x ), 
				yMin, 
				Fixed( z ),
				
				Fixed( x + size ), 
				yMax, 
				Fixed( z + size ) );
*/
}

void SpaceTree::QueryPlanesRec(	const PlaneX* planes, int nPlanes, int intersection, const Node* node )
{
	if ( intersection == grinliz::POSITIVE ) 
	{
		// we are fully inside, and don't need to check.
		++nodesChecked;
	}
	else if ( intersection == grinliz::INTERSECT ) 
	{
		Rectangle3X aabb;
		node->CalcAABB( &aabb, yMin, yMax );
		
		int nPositive = 0;

		for( int i=0; i<nPlanes; ++i ) {
			int comp = ComparePlaneAABBX( planes[i], aabb );

			// If the aabb is negative of any plane, it is culled.
			if ( comp == grinliz::NEGATIVE ) {
				intersection = grinliz::NEGATIVE;
				break;
			}
			// If the aabb intersects the plane, the result is subtle:
			// intersecting a plane doesn't meen it is in the frustrum. 
			// The intersection of the aabb and the plane is often 
			// completely outside the frustum.

			// If the aabb is positive of ALL the planes then we are in good shape.
			else if ( comp == grinliz::POSITIVE ) {
				++nPositive;
			}
		}
		if ( nPositive == nPlanes ) {
			// All positive is quick:
			intersection = grinliz::POSITIVE;
		}
		++nodesChecked;
		++nodesComputed;
	}
	if ( intersection != grinliz::NEGATIVE ) 
	{
#ifdef DEBUG
		if ( intersection == grinliz::INTERSECT )
			node->hit = 1;
		else if ( intersection == grinliz::POSITIVE )
			node->hit = 2;
#endif
		for( Item* item=node->sentinel.next; item != &node->sentinel; item=item->next ) 
		{
			Model* m = &item->model;
			if ( !m->IsHiddenFromTree() ) {
				m->next = modelRoot;
				modelRoot = m;
				++modelsFound;
			}
		}
		
		if ( node->child[0] )  {
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
		aabb.Set( Fixed( node->looseX ), yMin, Fixed( node->looseZ ),
				  Fixed( node->looseX + node->looseSize ), yMax, Fixed( node->looseZ + node->looseSize ) );
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
			if ( !m->IsHiddenFromTree() ) {
				m->next = modelRoot;
				modelRoot = m;
				++modelsFound;
			}
		}
		
		if ( node->depth+1<DEPTH)  {
			QueryPlanesRec( origin, direction, intersection, node->child[0] );
			QueryPlanesRec( origin, direction, intersection, node->child[1] );
			QueryPlanesRec( origin, direction, intersection, node->child[2] );
			QueryPlanesRec( origin, direction, intersection, node->child[3] );
		}
	}
}


#ifdef DEBUG
void SpaceTree::Draw()
{
	for( int i=0; i<NUM_NODES; ++i ) {
		if ( nodeArr[i].hit )
		{
			const Node& node = nodeArr[i];
			if ( node.depth < 4 )
				continue;

			const float y = 0.2f;
			const float offset = 0.05f;
			float v[12] = {	
							(float)node.x+offset, y, (float)node.z+offset,
							(float)node.x+offset, y, (float)(node.z+node.size)-offset,
							(float)(node.x+node.size)-offset, y, (float)(node.z+node.size)-offset,
							(float)(node.x+node.size)-offset, y, (float)node.z+offset
						  };

			if ( node.hit == 1 )
				glColor4f( 1.f, 0.f, 0.f, 0.5f );
			else
				glColor4f( 0.f, 1.f, 0.f, 0.5f );

			glVertexPointer( 3, GL_FLOAT, 0, v );

 			glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );	
			CHECK_GL_ERROR;
		}
	}
}
#endif

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

SpaceTree::SpaceTree( float yMin, float yMax )
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
	Circle circle;
	model->CalcBoundCircle( &circle );
	//int modelSize = circlex.radius.Ceil();

	int x = (int)circle.origin.x;
	int z = (int)circle.origin.y;

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
		if (    circle.origin.x - circle.radius >= float( node->looseX )
			 && circle.origin.y - circle.radius >= float( node->looseZ )
			 && circle.origin.x + circle.radius <= float( node->looseX + node->looseSize )
			 && circle.origin.y + circle.radius <= float( node->looseX + node->looseSize ) )
		{
			// fits.
			break;
		}
		--depth;
	}
	++nodeAddedAtDepth[depth];

#ifdef DEBUG
	if ( depth > 0 ) {
		Circle c;
		Rectangle3F aabb;
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

	//const int base[DEPTH] = { 0, 1, 1+4, 1+4+16, 1+4+16+64, 1+4+16+64+256 };
	const int base[DEPTH] = { 0, 1, 1+4, 1+4+16, 1+4+16+64 };
	int dx = (1<<depth);
	GLASSERT( nx < dx );
	GLASSERT( nz < dx );

	Node* result = &nodeArr[ base[depth] + nz*dx+nx ];
	GLASSERT( result >= nodeArr && result < &nodeArr[NUM_NODES] );
	return result;
}



Model* SpaceTree::Query( const Plane* planes, int nPlanes )
{
	modelRoot = 0;
	nodesVisited = 0;
	planesComputed = 0;
	spheresComputed = 0;
	modelsFound = 0;

#ifdef DEBUG
	for( int i=0; i<NUM_NODES; ++i ) {
		nodeArr[i].hit = 0;
	}
#endif

	QueryPlanesRec( planes, nPlanes, grinliz::INTERSECT, &nodeArr[0], 0 );
	
	/*
	GLOUTPUT(( "Query visited=%d of %d nodes, comptued planes=%d spheres=%d models=%d [%d,%d,%d,%d,%d]\n", 
				nodesVisited, NUM_NODES, 
				planesComputed,
				spheresComputed,
				modelsFound,
				nodeAddedAtDepth[0],
				nodeAddedAtDepth[1],
				nodeAddedAtDepth[2],
				nodeAddedAtDepth[3],
				nodeAddedAtDepth[4] ));
	*/
	
	return modelRoot;
}


Model* SpaceTree::Query( const Vector3F& origin, const Vector3F& direction )
{
	modelRoot = 0;
	nodesVisited = 0;
	modelsFound = 0;

	QueryPlanesRec( origin, direction, grinliz::INTERSECT, &nodeArr[0] );
	//GLOUTPUT(( "Query %d/%d nodes, models=%d\n", nodesChecked, NUM_NODES, modelsFound ));
	return modelRoot;

}


void SpaceTree::Node::CalcAABB( Rectangle3F* aabb, const float yMin, const float yMax ) const
{
	GLASSERT( yMin < yMax );
	GLASSERT( looseSize > 0 );

	aabb->Set(	float( looseX ), 
				yMin, 
				float( looseZ ),
				
				float( looseX + looseSize ), 
				yMax, 
				float( looseZ + looseSize ) );
/*
	aabb->Set(	Fixed( x ), 
				yMin, 
				Fixed( z ),
				
				Fixed( x + size ), 
				yMax, 
				Fixed( z + size ) );
*/
}

void SpaceTree::QueryPlanesRec(	const Plane* planes, int nPlanes, int intersection, const Node* node, U32 positive )
{
	#define POSITIVE( pos, i ) ( pos & (1<<i) )

	if ( intersection == grinliz::POSITIVE ) 
	{
		// we are fully inside, and don't need to check.
		++nodesVisited;
	}
	else if ( intersection == grinliz::INTERSECT ) 
	{
		Rectangle3F aabb;
		node->CalcAABB( &aabb, yMin, yMax );
		
		int nPositive = 0;

		for( int i=0; i<nPlanes; ++i ) {
			// Assume positive bit is set, check if not. Once we go POSITIVE, we don't need 
			// to check the sub-nodes. They will still be POSITIVE. Saves about 1/2 the Plane
			// and Sphere checks.
			//
			int comp = grinliz::POSITIVE;
			if ( POSITIVE( positive, i ) == 0 ) {
				comp = ComparePlaneAABB( planes[i], aabb );
				++planesComputed;
			}

			// If the aabb is negative of any plane, it is culled.
			if ( comp == grinliz::NEGATIVE ) {
				intersection = grinliz::NEGATIVE;
				break;
			}
			else if ( comp == grinliz::POSITIVE ) {
				// If the aabb intersects the plane, the result is subtle:
				// intersecting a plane doesn't meen it is in the frustrum. 
				// The intersection of the aabb and the plane is often 
				// completely outside the frustum.

				// If the aabb is positive of ALL the planes then we are in good shape.
				positive |= (1<<i);
				++nPositive;
			}
		}
		if ( nPositive == nPlanes ) {
			// All positive is quick:
			intersection = grinliz::POSITIVE;
		}
		++nodesVisited;
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
				
				if ( intersection == grinliz::INTERSECT ) {
					Sphere sphere;
					m->CalcBoundSphere( &sphere );
					
					int compare = grinliz::INTERSECT;
					for( int k=0; k<nPlanes; ++k ) {
						// Since the bounding sphere is in the AABB, we
						// can check for the positive plane again.
						if ( POSITIVE( positive, k ) == 0 ) {
							compare = ComparePlaneSphere( planes[k], sphere );
							++spheresComputed;
							if ( compare == grinliz::NEGATIVE ) {
								break;
							}
						}
					}
					if ( compare == grinliz::NEGATIVE )
						continue;
				}
				
				m->next = modelRoot;
				modelRoot = m;
				++modelsFound;
			}
		}
		
		if ( node->child[0] )  {
			QueryPlanesRec( planes, nPlanes, intersection, node->child[0], positive );
			QueryPlanesRec( planes, nPlanes, intersection, node->child[1], positive );
			QueryPlanesRec( planes, nPlanes, intersection, node->child[2], positive );
			QueryPlanesRec( planes, nPlanes, intersection, node->child[3], positive );
		}
	}
	#undef POSITIVE
}


void SpaceTree::QueryPlanesRec(	const Vector3F& origin, const Vector3F& direction, int intersection, const Node* node )
{
	bool callChildrenAndAddModels = false;

	// Note there isn't an obvious way for this to be positive. Rays always INTERSECT,
	// unless you do something tricky to detect special cases.

	if ( intersection == grinliz::POSITIVE ) 
	{
		// we are fully inside, and don't need to check.
		callChildrenAndAddModels = true;
		++nodesVisited;
	}
	else if ( intersection == grinliz::INTERSECT ) 
	{
		Rectangle3F aabb;
		aabb.Set( float( node->looseX ), yMin, float( node->looseZ ),
				  float( node->looseX + node->looseSize ), yMax, float( node->looseZ + node->looseSize ) );
		//GLOUTPUT(( "  l=%d rect: ", node->depth )); DumpRectangle( aabb ); GLOUTPUT(( "\n" ));
		
		Vector3F intersect;
		float t;

		int comp = IntersectRayAABB( origin, direction, aabb, &intersect, &t );
		if ( comp == grinliz::INTERSECT || comp == grinliz::INSIDE ) {
			intersection = grinliz::INTERSECT;
			callChildrenAndAddModels = true;
		}
		++planesComputed;
		++nodesVisited;
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

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
#include "../grinliz/glperformance.h"
#include "gpustatemanager.h"
using namespace grinliz;

/*
	Tuning:
	Unit tree:  Query checked=153, computed=145 of 341 nodes, models=347 [35,44,40,26,606]
	Classic:	Query checked=209, computed=209 of 341 nodes, models=361 [1,36,36,51,627]
	Unit tree has less compution and fewer models, slightly less balanced. 
*/

SpaceTree::SpaceTree( float yMin, float yMax ) 
	:	modelPool( "SpaceTreeModelPool", sizeof( Item ), EL_ALLOCATED_MODELS*sizeof( Item ), true )
{
	this->yMin = yMin;
	this->yMax = yMax;
	queryID = 0;

	InitNode();
}


SpaceTree::~SpaceTree()
{
	GLASSERT( modelPool.Empty() );
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
				//node->x = (float)i;
				//node->z = (float)j;
				//node->size = (float)nodeSize;
				node->queryID = 0;
				node->parent = 0;
				node->nModels = 0;
				if ( depth > 0 ) {
					node->parent = GetNode( depth-1, i, j );
				}

				/*
				// looseSize = size*2, "classic" tree.
				node->looseX = node->x - nodeSize/2;
				node->looseZ = node->z - nodeSize/2;
				node->looseSize = nodeSize*2;
				*/
				int border = nodeSize / 4;
				GLASSERT( border > 0 );
				//node->looseX = node->x - (float)border;
				//node->looseZ = node->z - (float)border;
				//node->looseSize = (float)(nodeSize+border*2);

				node->looseAABB.Set( (float)(i-border), yMin, (float)(j-border),
									 (float)(i+nodeSize+border), yMax, (float)(j+nodeSize+border) );

				/* Seems like a good plan, but isn't. An object that is on the edge of a small node
				   will be on the edge of a parent node...and goes to the top.

				// Most objects are 1x1. Take advantage of that:
				node->looseX = node->x - 1;
				node->looseZ = node->z - 1;
				node->looseSize = nodeSize+2;
				*/

				node->depth = depth;
				node->root = 0;

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


Model* SpaceTree::AllocModel( const ModelResource* resource )
{
	GLASSERT( resource );
	Item* item = (Item*) modelPool.Alloc();

	item->node = 0;
	item->next = 0;	// very important to clear pointers before Init() - which will cause Link to occur.
	item->prev = 0;
	item->model.Init( resource, this );

	return &item->model;
}


void SpaceTree::FreeModel( Model* model )
{
	Item* item = (Item*)model;	// cast depends on model being first in the structure.

	GLASSERT( item->node );
	item->node->Remove( item );
	item->model.Free();
	modelPool.Free( item );
}

// Based on this idea:
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

void SpaceTree::Update( Model* model )
{
	// Unlink if currently in tree.
	Item* item = (Item*)model;	// cast depends on model being first in the structure.

	if ( item->node ) 
		item->node->Remove( item );
	item->node = 0;

	// Since the tree is somewhat modified from the ideal, start with the 
	// most idea node and work up. Note that everything fits at the top node.
	int depth = DEPTH-1;
	Node* node = 0;
	
	Rectangle3F bounds = model->AABB();

	// Clamp to tree range.
	if (bounds.min.y < yMin ) 
		bounds.min.y = yMin;
	if (bounds.max.y > yMax )
		bounds.max.y = yMax;

	int x = (int)bounds.min.x;
	int z = (int)bounds.min.z;

	while( depth > 0 ) {
		node = GetNode( depth, x, z );
		if ( node->looseAABB.Contains( bounds ) ) {
			// fits.
			item->node = node;
			break;
		}
		--depth;
	}

	GLASSERT( item->node );
	item->node->Add( item );
}



SpaceTree::Node* SpaceTree::GetNode( int depth, int x, int z )
{
	GLASSERT( depth >=0 && depth < DEPTH );
	int size = Map::SIZE >> depth;
	int nx = x / size;
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



Model* SpaceTree::Query( const Plane* planes, int nPlanes, int required, int excluded, bool debug )
{
	GRINLIZ_PERFTRACK
	
	modelRoot = 0;
	nodesVisited = 0;
	planesComputed = 0;
	spheresComputed = 0;
	modelsFound = 0;
	requiredFlags = required;
	excludedFlags = excluded | Model::MODEL_HIDDEN_FROM_TREE;
	this->debug = debug;

#ifdef DEBUG
	for( int i=0; i<NUM_NODES; ++i ) {
		nodeArr[i].hit = 0;
	}
#endif

	QueryPlanesRec( planes, nPlanes, grinliz::INTERSECT, &nodeArr[0], 0 );

	/*
	if ( debug ) {
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
	}
	*/
	this->debug = false;
	return modelRoot;
}

void SpaceTree::Node::Add( Item* item ) 
{
	GLASSERT( item->next == 0 );
	GLASSERT( item->prev == 0 );
	GLASSERT( item->node == this );

	if ( root ) { 
		root->prev = item;
	}
	item->next = root;
	item->prev = 0;
	root = item;

	for( Node* it=this; it; it=it->parent )
		it->nModels++;
}


void SpaceTree::Node::Remove( Item* item ) 
{
	if ( root == item )
		root = item->next;
	if ( item->prev )
		item->prev->next = item->next;
	if ( item->next )
		item->next->prev = item->prev;

	item->next = 0;
	item->prev = 0;

	for( Node* it=this; it; it=it->parent )
		it->nModels--;
}


#ifdef DEBUG
void SpaceTree::Node::Dump()
{
	for( int i=0; i<depth; ++i )
		GLOUTPUT(( "  " ));
	GLOUTPUT(( "depth=%d nModels=%d (%.1f,%.1f)-(%.1f,%.1f)\n",
				depth, nModels, 
				looseAABB.min.x, looseAABB.min.z, 
				looseAABB.max.x, looseAABB.max.z ));
}


void SpaceTree::Dump( Node* node )
{
	node->Dump();
	for( int i=0; i<4; ++i )
		if ( node->child[i] )
			Dump( node->child[i] );
}
#endif




void SpaceTree::QueryPlanesRec(	const Plane* planes, int nPlanes, int intersection, const Node* node, U32 positive )
{
	#define IS_POSITIVE( pos, i ) ( pos & (1<<i) )

	if ( intersection == grinliz::POSITIVE ) 
	{
		// we are fully inside, and don't need to check.
		++nodesVisited;
	}
	else if ( intersection == grinliz::INTERSECT ) 
	{
		const Rectangle3F& aabb = node->looseAABB;
		
		int nPositive = 0;

		for( int i=0; i<nPlanes; ++i ) {
			// Assume positive bit is set, check if not. Once we go POSITIVE, we don't need 
			// to check the sub-nodes. They will still be POSITIVE. Saves about 1/2 the Plane
			// and Sphere checks.
			//
			int comp = grinliz::POSITIVE;
			if ( IS_POSITIVE( positive, i ) == 0 ) {
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
		const int _requiredFlags = requiredFlags;
		const int _excludedFlags = excludedFlags;

		for( Item* item=node->root; item; item=item->next ) 
		{
			Model* m = &item->model;
			const int flags = m->Flags();

			if (    ( (_requiredFlags & flags) == _requiredFlags)
				 && ( (_excludedFlags & flags) == 0 ) )
			{	
				if ( debug ) {
					GLOUTPUT(( "%*s[%d] Testing: 0x%x %s", node->depth, " ", node->depth, (int)m, m->GetResource()->header.name.c_str() ));
				}
				if ( intersection == grinliz::INTERSECT ) {
					const Rectangle3F& aabb = m->AABB();
					int compare = grinliz::INTERSECT;

					for( int k=0; k<nPlanes; ++k ) {
						// Since the bounding sphere is in the AABB, we
						// can check for the positive plane again.
						if ( IS_POSITIVE( positive, k ) == 0 ) {
							compare = ComparePlaneAABB( planes[k], aabb );
							++spheresComputed;
							if ( compare == grinliz::NEGATIVE ) {
								break;
							}
						}
					}
					if ( compare == grinliz::NEGATIVE ) {
						if ( debug ) {
							GLOUTPUT(( "...NEGATIVE\n" ));
						}
						continue;
					}
				}
				if ( debug ) {
					GLOUTPUT(( "...yes\n" ));
				}
				
				m->next = modelRoot;
				modelRoot = m;
				++modelsFound;
			}
		}
		
		if ( node->child[0] )  {
			for( int i=0; i<4; ++i ) {
				if ( node->child[i]->nModels )
					QueryPlanesRec( planes, nPlanes, intersection, node->child[i], positive );
			}
		}
	}
	#undef IS_POSITIVE
}




Model* SpaceTree::QueryRay( const Vector3F& _origin, 
							const Vector3F& _direction, 
							int required, int excluded, const Model** ignore,
							HitTestMethod testType,
							Vector3F* intersection ) 
{
	//GLOUTPUT(( "query ray\n" ));
	modelRoot = 0;
	nodesVisited = 0;
	modelsFound = 0;
	requiredFlags = required;
	excludedFlags = excluded | Model::MODEL_HIDDEN_FROM_TREE;
	++queryID;

	Vector3F dummy;
	if ( !intersection ) {
		intersection = &dummy;
	}

	Vector3F dir = _direction;
	dir.Normalize();

	Rectangle3F aabb;
	aabb.min.Set( 0, yMin, 0 );
	aabb.max.Set( Map::SIZE, yMax, Map::SIZE );

	// Where does this ray enter and leave the spaceTree?
	// It enters at 'p0' and leaves at 'p1'
	int p0Test, p1Test;
	Vector3F p0, p1;
	int test = IntersectRayAllAABB( _origin, dir, aabb, &p0Test, &p0, &p1Test, &p1 );
	if ( test != grinliz::INTERSECT ) {
		// Can click outside of AABB pretty commonly, actually.
		return 0;
	}
	Plane planes[6];
	Rectangle3F rect;
	rect.Set( p0, p1 );
	Plane::CreatePlanes( rect, planes );

	Model* modelRoot = Query( planes, 6, required, excluded );

	// We now have a batch of models. Are any of them a hit??
	GLASSERT( testType == TEST_HIT_AABB || testType == TEST_TRI );

	float close = FLT_MAX;
	Model* closeModel = 0;
	Vector3F testInt;
	float t;

	for( Model* root=modelRoot; root; root=root->next ) {
		if ( Ignore( root, ignore ) )
			continue;

		//GLOUTPUT(( "Consider: %s\n", root->GetResource()->header.name ));
		int result = grinliz::REJECT;

		if ( testType == TEST_HIT_AABB ) {
			Rectangle3F modelAABB;

			root->CalcHitAABB( &modelAABB );
			result = IntersectRayAABB( p0, dir, modelAABB, &testInt, &t );
		}
		else if ( testType == TEST_TRI ) {
			t = FLT_MAX;
			result = root->IntersectRay( p0, dir, &testInt );

			if ( result == grinliz::INTERSECT ) {
				Vector3F delta = p0 - testInt;
				t = delta.LengthSquared();
				//GLOUTPUT(( "Hit: %s t=%.2f\n", root->GetResource()->header.name, t ));
			}	
		}

		if ( result == grinliz::INTERSECT ) {
			// Ugly little bug: check for t>=0, else could collide with objects
			// that touch the bounding box but are before the ray starts.
			if ( t >= 0.0f && t < close ) {
				closeModel = root;
				*intersection = testInt;
				close = t;
			}
		}
	}
	if ( closeModel ) {
		return closeModel;
	}
	return 0;
}


#ifdef DEBUG
void SpaceTree::Draw()
{
	CompositingShader shader( true );

	for( int i=0; i<NUM_NODES; ++i ) {
		if ( nodeArr[i].hit )
		{
			const Node& node = nodeArr[i];
			if ( node.depth < 4 )
				continue;

			const float y = 0.2f;
			const float offset = 0.05f;
//			float v[12] = {	
//							node.x+offset, y, node.z+offset,
//							node.x+offset, y, (node.z+node.size)-offset,
//							(node.x+node.size)-offset, y, (node.z+node.size)-offset,
//							(node.x+node.size)-offset, y, node.z+offset
//						  };
			GLASSERT( 0 );

			if ( node.hit == 1 )
				shader.SetColor( 1.f, 0.f, 0.f, 0.5f );
			else
				shader.SetColor( 0.f, 1.f, 0.f, 0.5f );

			static const int index[6] = { 0, 1, 2, 0, 2, 3 };
//			shader.SetVertex( 3, 0, v );
			shader.Draw( 6, index );
		}
	}
}
#endif

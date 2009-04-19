/*
Copyright (c) 2000-2009 Lee Thomason (www.grinninglizard.com)

Grinning Lizard Utilities.

This software is provided 'as-is', without any express or implied 
warranty. In no event will the authors be held liable for any 
damages arising from the use of this software.

Permission is granted to anyone to use this software for any 
purpose, including commercial applications, and to alter it and 
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must 
not claim that you wrote the original software. If you use this 
software in a product, an acknowledgment in the product documentation 
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and 
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source 
distribution.
*/

#ifdef _MSC_VER
#pragma warning( disable : 4786 )	// Debugger truncating names.
#pragma warning( disable : 4530 )	// Exception handler isn't used
#endif

#ifndef MICROPATHER_STATIC_MEMORY
#	include <vector>
#	include <memory.h>
#endif
#include <stdio.h>

//#define DEBUG_PATH
//#define DEBUG_PATH_DEEP

#include "micropather.h"

using namespace std;
using namespace micropather;

class OpenQueue
{
  public:
	OpenQueue( Graph* _graph )
	{ 
		graph = _graph; 
		sentinel = (PathNode*) sentinelMem;
		sentinel->Init( 0, 0, FLT_MAX, FLT_MAX, 0 );
		sentinel->totalCost = FLT_MAX;
		sentinel->next = sentinel;
		sentinel->prev = sentinel;
			#ifdef DEBUG
				sentinel->CheckList();
			#endif
	}
	~OpenQueue()	{}

	void Push( PathNode* pNode );
	PathNode* Pop();
	void Update( PathNode* pNode );
    
	bool Empty()	{ return sentinel->next == sentinel; }

  private:
	OpenQueue( const OpenQueue& );	// undefined and unsupported
	void operator=( const OpenQueue& );
  
	PathNode* sentinel;
	int sentinelMem[ ( sizeof( PathNode ) + sizeof( int ) ) / sizeof( int ) ];
	Graph* graph;	// for debugging
};


void OpenQueue::Push( PathNode* pNode )
{
	
	assert( pNode->inOpen == 0 );
	assert( pNode->inClosed == 0 );
	
#ifdef DEBUG_PATH_DEEP
	printf( "Open Push: " );
	graph->PrintStateInfo( pNode->state );
	printf( " total=%.1f\n", pNode->totalCost );		
#endif
	
	// Add sorted. Lowest to highest cost path. Note that the sentinel has
	// a value of FLT_MAX, so it should always be sorted in.
	assert( pNode->totalCost < FLT_MAX );
	PathNode* iter = sentinel->next;
	while ( true )
	{
		if ( pNode->totalCost < iter->totalCost ) {
			iter->AddBefore( pNode );
			pNode->inOpen = 1;
			break;
		}
		iter = iter->next;
	}
	assert( pNode->inOpen );	// make sure this was actually added.
#ifdef DEBUG
	sentinel->CheckList();
#endif
}

PathNode* OpenQueue::Pop()
{
	assert( sentinel->next != sentinel );
	PathNode* pNode = sentinel->next;
	pNode->Unlink();
#ifdef DEBUG
	sentinel->CheckList();
#endif
	
	assert( pNode->inClosed == 0 );
	assert( pNode->inOpen == 1 );
	pNode->inOpen = 0;
	
#ifdef DEBUG_PATH_DEEP
	printf( "Open Pop: " );
	graph->PrintStateInfo( pNode->state );
	printf( " total=%.1f\n", pNode->totalCost );		
#endif
	
	return pNode;
}

void OpenQueue::Update( PathNode* pNode )
{
#ifdef DEBUG_PATH_DEEP
	printf( "Open Update: " );		
	graph->PrintStateInfo( pNode->state );
	printf( " total=%.1f\n", pNode->totalCost );		
#endif
	
	assert( pNode->inOpen );
	
	// If the node now cost less than the one before it,
	// move it to the front of the list.
	if ( pNode->prev != sentinel && pNode->totalCost < pNode->prev->totalCost ) {
		pNode->Unlink();
		sentinel->next->AddBefore( pNode );
	}
	
	// If the node is too high, move to the right.
	if ( pNode->totalCost > pNode->next->totalCost ) {
		PathNode* it = pNode->next;
		pNode->Unlink();
		
		while ( pNode->totalCost > it->totalCost )
			it = it->next;
		
		it->AddBefore( pNode );
#ifdef DEBUG
		sentinel->CheckList();
#endif
	}
}


class ClosedSet
{
  public:
	ClosedSet( Graph* _graph )		{ this->graph = _graph; }
	~ClosedSet()	{}

	void Add( PathNode* pNode )
	{
		#ifdef DEBUG_PATH_DEEP
			printf( "Closed add: " );		
			graph->PrintStateInfo( pNode->state );
			printf( " total=%.1f\n", pNode->totalCost );		
		#endif
		#ifdef DEBUG
		assert( pNode->inClosed == 0 );
		assert( pNode->inOpen == 0 );
		#endif
		pNode->inClosed = 1;
	}

	void Remove( PathNode* pNode )
	{
		#ifdef DEBUG_PATH_DEEP
			printf( "Closed remove: " );		
			graph->PrintStateInfo( pNode->state );
			printf( " total=%.1f\n", pNode->totalCost );		
		#endif
		assert( pNode->inClosed == 1 );
		assert( pNode->inOpen == 0 );

		pNode->inClosed = 0;
	}

  private:
	ClosedSet( const ClosedSet& );
	void operator=( const ClosedSet& );
	Graph* graph;
};


#ifdef MICROPATHER_STATIC_MEMORY
MicroPather::MicroPather( Graph* _graph, void* memory, unsigned memorySize, unsigned maxAdjacent )
	:	graph( _graph ),
		pathNodeCount( 0 ),
		frame( 0 ),
		checksum( 0 )
{
	neighborVec = (StateCost*)memory;
	neighborNodeVec = (NodeCost*)(neighborVec+maxAdjacent);
	pathNodeMem = (PathNode*)(neighborNodeVec+maxAdjacent);

	assert( (char*)pathNodeMem < (char*)memory + memorySize );	// not enough memory
	assert( ((int)memory) % 4 == 0 );
	assert( ((int)neighborVec) % 4 == 0 );
	assert( ((int)neighborNodeVec) % 4 == 0 );
	assert( ((int)pathNodeMem) % 4 == 0 );

	ALLOCATE = (((char*)memory + memorySize) - (char*)pathNodeMem ) / sizeof( PathNode );
	BLOCKSIZE = ALLOCATE;
	availMem = ALLOCATE;

	MAX_ADJACENT = maxAdjacent;

	memset( hashTable, 0, sizeof( PathNode* )*HASH_SIZE );
}
#else
MicroPather::MicroPather( Graph* _graph, unsigned allocate )
	:	ALLOCATE( allocate ),
		BLOCKSIZE( allocate-1 ),
		graph( _graph ),
		pathNodeMem( 0 ),
		availMem( 0 ),
		pathNodeCount( 0 ),
		frame( 0 ),
		checksum( 0 )
{
	memset( hashTable, 0, sizeof( PathNode* )*HASH_SIZE );
}
#endif


MicroPather::~MicroPather()
{
#ifndef MICROPATHER_STATIC_MEMORY
	PathNode *temp;
	while( pathNodeMem ) {
		temp = pathNodeMem;
		pathNodeMem = pathNodeMem[ ALLOCATE-1 ].left;
		free( temp );	// c-allocation
	}
#endif
}

    
void MicroPather::Reset()
{
#ifdef MICROPATHER_STATIC_MEMORY
	availMem = ALLOCATE;
#else
	while( pathNodeMem ) {
		PathNode* temp = pathNodeMem;
		pathNodeMem = pathNodeMem[ ALLOCATE-1 ].left;
		free( temp );	// c-allocation
	}
	pathNodeMem = 0;
	availMem = 0;
#endif
	memset( hashTable, 0, sizeof( PathNode* )*HASH_SIZE );
	pathNodeCount = 0;
	frame = 0;
	checksum = 0;
}


PathNode* MicroPather::AllocatePathNode() 
{
#ifdef MICROPATHER_STATIC_MEMORY
	if ( availMem == 0 ) {
		return 0;
	}
#else
	if ( availMem == 0 ) {
		PathNode* newBlock = (PathNode*) malloc( sizeof(PathNode) * ALLOCATE );
		// set up the "next" pointer
		newBlock[ALLOCATE-1].left = pathNodeMem;
		pathNodeMem = newBlock;
		availMem = BLOCKSIZE;
	}
	assert( availMem );
#endif

	PathNode* result = pathNodeMem + (BLOCKSIZE	- availMem );
	--availMem;
	++pathNodeCount;
	return result;
}


PathNode* MicroPather::NewPathNode( void* state, float costFromStart, float estToEnd, PathNode* parent )
{
	// Try to find an existing node for this state.
	unsigned key = Hash( state );   //(HASH_SIZE-1) & ( (unsigned)state + (((unsigned)state)>>8) + (((unsigned)state)>>16) + (((unsigned)state)>>24) );

	if ( !hashTable[key] ) {
		// There isn't even a hashtable yet - create and initialize the PathNode.
		PathNode* newNode = AllocatePathNode();
		if ( !newNode ) { 
			return 0;
		}
		hashTable[key] = newNode;
		hashTable[key]->Init( frame, state, costFromStart, estToEnd, parent );
		return hashTable[key];
	}

	PathNode* root = hashTable[key];
	PathNode* up = 0;
	while ( root ) {
		up = root;
		if ( root->state == state ) {
			root->Reuse( frame, costFromStart, estToEnd, parent );
			assert( root->state == state );
			return root;
		}
		else {
			root = root->left;
		}
	}

	assert( up );
	PathNode* pNode = AllocatePathNode();
	if ( pNode ) {
		pNode->Init( frame, state, costFromStart, estToEnd, parent );
		up->left = pNode;
	}
	return pNode;
}

#ifdef MICROPATHER_STATIC_MEMORY
void MicroPather::GoalReached( PathNode* node, void* start, void* end, int maxPath, void* path[], int *pathLen )
#else
void MicroPather::GoalReached( PathNode* node, void* start, void* end, vector< void* > *_path )
#endif
{
#ifdef MICROPATHER_STATIC_MEMORY
	*pathLen = 0;
#else
	std::vector< void* >& path = *_path;
	path.clear();
#endif

	// We have reached the goal.
	// How long is the path? Used to allocate the vector which is returned.
	int count = 1;
	PathNode* it = node;
	while( it->parent )
	{
		++count;
		it = it->parent;
	}

#ifdef MICROPATHER_STATIC_MEMORY
	if ( count > maxPath ) {
		*pathLen = 0;
 		return;
	}
#endif

	// Now that the path has a known length, allocate
	// and fill the vector that will be returned.
	if ( count < 3 )
	{
		// Handle the short, special case.
#ifdef MICROPATHER_STATIC_MEMORY
		*pathLen = 2;
		path[0] = start;
		path[1] = end;
#else
		path.resize(2);
		path[0] = start;
		path[1] = end;
#endif
	}
	else
	{
#ifdef MICROPATHER_STATIC_MEMORY
		*pathLen = count;
		path[0] = start;
		path[count-1] = end;
#else
		path.resize(count);

		path[0] = start;
		path[count-1] = end;
#endif
		count-=2;
		it = node->parent;

		while ( it->parent )
		{
			path[count] = it->state;
			it = it->parent;
			--count;
		}
	}
	checksum = 0;
	#ifdef DEBUG_PATH
	printf( "Path: " );
	int counter=0;
	#endif
#ifdef MICROPATHER_STATIC_MEMORY
	for ( unsigned k=0; k<(unsigned)(*pathLen); ++k )
#else
	for ( unsigned k=0; k<path.size(); ++k )
#endif
	{
		checksum += ((UPTR)(path[k])) << (k%8);

		#ifdef DEBUG_PATH
		graph->PrintStateInfo( path[k] );
		printf( " " );
		++counter;
		if ( counter == 8 )
		{
			printf( "\n" );
			counter = 0;
		}
		#endif
	}
	#ifdef DEBUG_PATH
	printf( "Cost=%.1f Checksum %d\n", node->costFromStart, checksum );
	#endif
}


PathNode* MicroPather::FindPathNode( void* state )
{
	unsigned key = Hash( state );
	PathNode* root = hashTable[key];
	
	while( root ) {
		if ( root->state == state ) {
			if ( root->frame == frame )
				return root;
			else
				return 0;
		}
		else {
			root = root->left;
		}
	}
	return 0;
}	

#ifdef MICROPATHER_STATIC_MEMORY
const NodeCost* MicroPather::GetNodeNeighbors( PathNode* node, NodeCost* neighborNode, int* nNeighborNode )
#else
const NodeCost* MicroPather::GetNodeNeighbors( PathNode* node, std::vector< NodeCost >* _neighborNode )
#endif
{
	NodeCost* nodeCost = 0;

	if (    node->numAdjacent < 0 
		 || node->numAdjacent > PathNode::MAX_CACHE )
	{
		// It has not been computed yet (<0) or
		// can not be cached (>MAX)
#ifdef MICROPATHER_STATIC_MEMORY
		*nNeighborNode = 0;
		graph->AdjacentCost( node->state, neighborVec, nNeighborNode );
		#ifdef DEBUG
		{
			// If this assert fires, you have passed a state
			// as its own neighbor state. This is impossible --
			// bad things will happen.
			for ( int i=0; i<*nNeighborNode; ++i )
				assert( neighborVec[i].state != node->state );
		}
		#endif
#else
		std::vector< NodeCost >& neighborNode = *_neighborNode;
		neighborVec.resize( 0 );
		graph->AdjacentCost( node->state, &neighborVec );
		#ifdef DEBUG
		{
			// If this assert fires, you have passed a state
			// as its own neighbor state. This is impossible --
			// bad things will happen.
			for ( unsigned i=0; i<neighborVec.size(); ++i )
				assert( neighborVec[i].state != node->state );
		}
		#endif
#endif
#ifdef MICROPATHER_STATIC_MEMORY
		node->numAdjacent = *nNeighborNode;
#else
		node->numAdjacent = (int) neighborVec.size();
#endif

		// Now convert to pathNodes, and put in cache if possible.
		if ( node->numAdjacent <= PathNode::MAX_CACHE ) 
		{
			// Can fit in the cache:
			StateCost* neighborPtr = &neighborVec[0];
			for( int i=0; i<node->numAdjacent; ++i, ++neighborPtr ) 
			{
				node->adjacent[i].cost = neighborPtr->cost;
				node->adjacent[i].node = FindPathNode( neighborPtr->state );
				if ( !node->adjacent[i].node ) 
				{
					node->adjacent[i].node = NewPathNode( neighborPtr->state, FLT_BIG, FLT_BIG, 0 );
					if ( !node->adjacent[i].node ) {
						return 0;
					}
				}
			}
			nodeCost = node->adjacent;

			#ifdef DEBUG_PATH_DEEP
				printf( "State " );
				graph->PrintStateInfo( node->state );
				printf( "--> cache\n" );
			#endif
		}
		else {
			// Too big for cache.
#ifdef MICROPATHER_STATIC_MEMORY
			node->numAdjacent = *nNeighborNode;
			*nNeighborNode = *nNeighborNode;
#else
			node->numAdjacent = (int) neighborVec.size();
			neighborNode.resize( neighborVec.size() );
#endif

			for( int i=0; i<node->numAdjacent; ++i ) {
				const StateCost& neighborPtr = neighborVec[i];
				NodeCost* neighborNodePtr = &neighborNode[i];
				
				neighborNodePtr->cost = neighborPtr.cost;
				neighborNodePtr->node = FindPathNode( neighborPtr.state );
				if ( !neighborNodePtr->node ) 
				{
					neighborNodePtr->node = NewPathNode( neighborPtr.state, FLT_BIG, FLT_BIG, 0 );
					if ( !neighborNodePtr->node ) {
						return 0;
					}
				}
			}		
			nodeCost = &neighborNode[0];
			#ifdef DEBUG_PATH_DEEP
				printf( "State " );
				graph->PrintStateInfo( node->state );
				printf( "no cache\n" );
			#endif
		}
	}
	else {
		// In the cache!
		nodeCost = node->adjacent;

		for( int i=0; i<node->numAdjacent; ++i ) {
			if ( node->adjacent[i].node->frame != frame )
				node->adjacent[i].node->Reuse( frame, FLT_BIG, FLT_BIG, 0 );
		}
		#ifdef DEBUG_PATH_DEEP
			printf( "State " );
			graph->PrintStateInfo( node->state );
			printf( "cache HIT\n" );
		#endif
	}
	assert( nodeCost );

	return nodeCost;
}


#ifdef DEBUG
void MicroPather::DumpStats()
{
	int hashTableEntries = 0;
	for( int i=0; i<HASH_SIZE; ++i )
		if ( hashTable[i] )
			++hashTableEntries;
	
	int pathNodeBlocks = 0;
#ifndef MICROPATHER_STATIC_MEMORY
	for( PathNode* node = pathNodeMem; node; node = node[ALLOCATE-1].left )
		++pathNodeBlocks;
#endif
	printf( "HashTableEntries=%d/%d PathNodeBlocks=%d [%dk] PathNodes=%d SolverCalled=%d\n",
			  hashTableEntries, HASH_SIZE, pathNodeBlocks, 
			  pathNodeBlocks*ALLOCATE*sizeof(PathNode)/1024,
			  pathNodeCount,
			  frame );
}
#endif

#ifndef MICROPATHER_STATIC_MEMORY
void MicroPather::StatesInPool( std::vector< void* >* stateVec )
{
 	stateVec->clear();
	
    for ( PathNode* mem = pathNodeMem; mem; mem = mem[ALLOCATE-1].left )
    {
        unsigned count = BLOCKSIZE;
        if ( mem == pathNodeMem )
        	count = BLOCKSIZE - availMem;
    	
    	for( unsigned i=0; i<count; ++i )
    	{
    	    if ( mem[i].frame == frame )
	    	    stateVec->push_back( mem[i].state );
    	}    
	}           
}    
#endif

#ifdef MICROPATHER_STATIC_MEMORY
int MicroPather::Solve( void* startNode, void* endNode, int maxPath, void* path[], int *nPath, float* cost )
#else
int MicroPather::Solve( void* startNode, void* endNode, vector< void* >* path, float* cost )
#endif
{
	#ifdef DEBUG_PATH
	printf( "Path: " );
	graph->PrintStateInfo( startNode );
	printf( " --> " );
	graph->PrintStateInfo( endNode );
	printf( " min cost=%f\n", graph->LeastCostEstimate( startNode, endNode ) );
	#endif

	*cost = 0.0f;

#ifdef MICROPATHER_STATIC_MEMORY
	*nPath = 0;
#endif

	if ( startNode == endNode )
		return START_END_SAME;

	++frame;

	OpenQueue open( graph );
	ClosedSet closed( graph );
	
	PathNode* newPathNode = NewPathNode( startNode,										// node
										0,												// cost from start
										graph->LeastCostEstimate( startNode, endNode ),
										0 );
	if ( !newPathNode ) {
		Reset();
		return OUT_OF_MEMORY;
	}
	
	open.Push( newPathNode );
	
#ifdef MICROPATHER_STATIC_MEMORY
	int nNeighborNodeVec = 0;
#else
	std::vector< NodeCost > neighborNodeVec; // really local vars to Solve, but put here to reduce memory allocation
	neighborNodeVec.resize(0);
#endif

	while ( !open.Empty() )
	{
		PathNode* node = open.Pop();
		
		if ( node->state == endNode )
		{
#ifdef MICROPATHER_STATIC_MEMORY
			GoalReached( node, startNode, endNode, maxPath, path, nPath );
#else
			GoalReached( node, startNode, endNode, path );
#endif
			*cost = node->costFromStart;
			#ifdef DEBUG_PATH
			DumpStats();
			#endif
			return SOLVED;
		}
		else
		{
			// We have not reached the goal - add the neighbors.
#ifdef MICROPATHER_STATIC_MEMORY
			const NodeCost* nodeCost = GetNodeNeighbors( node, neighborNodeVec, &nNeighborNodeVec );
			if ( !nodeCost ) {
				Reset();
				return OUT_OF_MEMORY;
			}
#else
			const NodeCost* nodeCost = GetNodeNeighbors( node, &neighborNodeVec );
#endif

			for( int i=0; i<node->numAdjacent; ++i )
			{
				if ( nodeCost[i].cost == FLT_MAX ) {
					continue;
				}

				float newCost = node->costFromStart + nodeCost[i].cost;

				PathNode* inOpen   = nodeCost[i].node->inOpen ? nodeCost[i].node : 0;
				PathNode* inClosed = nodeCost[i].node->inClosed ? nodeCost[i].node : 0;
				assert( !( inOpen && inClosed ) );
				PathNode* inEither = inOpen ? inOpen : inClosed;

				assert( inEither != node );

				if ( inEither )
				{
    				// Is this node is in use, and the cost is not an improvement,
    				// continue on.
					if ( inEither->costFromStart <= newCost )
						continue;	// Do nothing. This path is not better than existing.

					// Groovy. We have new information or improved information.
					inEither->parent = node;
					inEither->costFromStart = newCost;
					inEither->estToGoal = graph->LeastCostEstimate( inEither->state, endNode );
					inEither->totalCost = inEither->costFromStart + inEither->estToGoal;
				}

				if ( inClosed )
				{
					closed.Remove( inClosed );
					open.Push( inClosed );
				}	
				else if ( inOpen )
				{
					// Need to update the sort!
					open.Update( inOpen );
				}
				else if (!inEither)
				{
					assert( !inEither );
					assert( nodeCost[i].node );

					PathNode* pNode = nodeCost[i].node;
					pNode->parent = node;
					pNode->costFromStart = newCost;
					pNode->estToGoal = graph->LeastCostEstimate( pNode->state, endNode ),
					pNode->totalCost = pNode->costFromStart + pNode->estToGoal;
					
					open.Push( pNode );
				}
			}
		}					
		closed.Add( node );
	}
	#ifdef DEBUG_PATH
	DumpStats();
	#endif
	return NO_SOLUTION;		
}	



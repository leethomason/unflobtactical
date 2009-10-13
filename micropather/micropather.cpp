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

#include <vector>
#include <memory.h>
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
		sentinel->InitSentinel();
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
	
	MPASSERT( pNode->inOpen == 0 );
	MPASSERT( pNode->inClosed == 0 );
	
#ifdef DEBUG_PATH_DEEP
	printf( "Open Push: " );
	graph->PrintStateInfo( pNode->state );
	printf( " total=%.1f\n", pNode->totalCost );		
#endif
	
	// Add sorted. Lowest to highest cost path. Note that the sentinel has
	// a value of FLT_MAX, so it should always be sorted in.
	MPASSERT( pNode->totalCost < FLT_MAX );
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
	MPASSERT( pNode->inOpen );	// make sure this was actually added.
#ifdef DEBUG
	sentinel->CheckList();
#endif
}

PathNode* OpenQueue::Pop()
{
	MPASSERT( sentinel->next != sentinel );
	PathNode* pNode = sentinel->next;
	pNode->Unlink();
#ifdef DEBUG
	sentinel->CheckList();
#endif
	
	MPASSERT( pNode->inClosed == 0 );
	MPASSERT( pNode->inOpen == 1 );
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
	
	MPASSERT( pNode->inOpen );
	
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
		MPASSERT( pNode->inClosed == 0 );
		MPASSERT( pNode->inOpen == 0 );
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
		MPASSERT( pNode->inClosed == 1 );
		MPASSERT( pNode->inOpen == 0 );

		pNode->inClosed = 0;
	}

  private:
	ClosedSet( const ClosedSet& );
	void operator=( const ClosedSet& );
	Graph* graph;
};


PathNodePool::PathNodePool( unsigned _allocate, unsigned _typicalAdjacent ) 
	: firstBlock( 0 ),
	  blocks( 0 ),
#if defined( MICROPATHER_STRESS )
	  allocate( 32 ),
#else
	  allocate( _allocate ),
#endif
	  nAllocated( 0 ),
	  nAvailable( 0 )
{
	freeMemSentinel.InitSentinel();

	cacheCap = allocate * _typicalAdjacent;
	cacheSize = 0;
	cache = (NodeCost*)malloc(cacheCap * sizeof(NodeCost));

	hashShift = 3;	// 8 
#if !defined( MICROPATHER_STRESS )
	while( HashSize()*2 <= allocate )
		++hashShift;
#endif
	hashTable = (PathNode**)calloc( HashSize(), sizeof(PathNode*) );

	blocks = firstBlock = NewBlock();
}


PathNodePool::~PathNodePool()
{
	Clear();
	free( firstBlock );
	free( cache );
	free( hashTable );
}


bool PathNodePool::PushCache( const NodeCost* nodes, int nNodes, int* start ) {
	*start = -1;
	if ( nNodes+cacheSize <= cacheCap ) {
		for( int i=0; i<nNodes; ++i ) {
			cache[i+cacheSize] = nodes[i];
		}
		*start = cacheSize;
		cacheSize += nNodes;
		return true;
	}
	return false;
}


void PathNodePool::Clear()
{
	Block* b = blocks;
	while( b ) {
		Block* temp = b->nextBlock;
		if ( b != firstBlock ) {
			free( b );
		}
		b = temp;
	}
	blocks = firstBlock;	// Don't delete the first block (we always need at least that much memory.)

	// Set up for new allocations (but don't do work we don't need to. Reset/Clear can be called frequently.)
	if ( nAllocated > 0 ) {
		freeMemSentinel.next = &freeMemSentinel;
		freeMemSentinel.prev = &freeMemSentinel;
	
		memset( hashTable, 0, sizeof(PathNode*)*HashSize() );
		for( unsigned i=0; i<allocate; ++i ) {
			freeMemSentinel.AddBefore( &firstBlock->pathNode[i] );
		}
	}
	nAvailable = allocate;
	nAllocated = 0;
	cacheSize = 0;
}


PathNodePool::Block* PathNodePool::NewBlock()
{
	Block* block = (Block*) calloc( 1, sizeof(Block) + sizeof(PathNode)*(allocate-1) );
	block->nextBlock = 0;

	nAvailable += allocate;
	for( unsigned i=0; i<allocate; ++i ) {
		freeMemSentinel.AddBefore( &block->pathNode[i] );
	}
	return block;
}


PathNode* PathNodePool::Alloc()
{
	if ( freeMemSentinel.next == &freeMemSentinel ) {
		MPASSERT( nAvailable == 0 );

		Block* b = NewBlock();
		b->nextBlock = blocks;
		blocks = b;
		MPASSERT( freeMemSentinel.next != &freeMemSentinel );
	}
	PathNode* pathNode = freeMemSentinel.next;
	pathNode->Unlink();

	++nAllocated;
	MPASSERT( nAvailable > 0 );
	--nAvailable;
	return pathNode;
}


PathNode* PathNodePool::FindPathNode( unsigned int frame, void* state )
{
	unsigned key = Hash( state );

	PathNode* root = hashTable[key];
	while( root ) {
		if ( root->state == state ) {
			if ( root->frame == frame )
				return root;
			return 0;
		}
		root = root->left;
	}
	return root;
}


PathNode* PathNodePool::NewPathNode(	unsigned frame,
										void* state,
										float costFromStart, 
										float estToGoal, 
										PathNode* parent )
{
	// Find one and reuse if the states match.
	unsigned key = Hash( state );

	PathNode* root = hashTable[key];
	while( root ) {
		if ( root->state == state ) {
			break;
		}
		root = root->left;
	}

	if ( root ) {
		// Found an allocated block of memory - reuse and return.
		// We can re-use the neighbor cache, since all hash table lookups
		// fail after a reset.
		MPASSERT( root->frame != frame );	// If fires, this is actually in use!
		root->Init( frame, state, costFromStart, estToGoal, parent );
		return root;
	}

	// No existing block - open one up.
	PathNode* pathNode = Alloc();
	pathNode->Clear();
	pathNode->Init( frame, state, costFromStart, estToGoal, parent );
	pathNode->left = hashTable[key];
	hashTable[key] = pathNode;
	return pathNode;
}


void PathNode::Init(	unsigned _frame,
						void* _state,
						float _costFromStart, 
						float _estToGoal, 
						PathNode* _parent )
{
	state = _state;
	costFromStart = _costFromStart;
	estToGoal = _estToGoal;
	CalcTotalCost();
	parent = _parent;
	frame = _frame;
	inOpen = 0;
	inClosed = 0;
}

MicroPather::MicroPather( Graph* _graph, unsigned allocate, unsigned typicalAdjacent )
	:	pathNodePool( allocate, typicalAdjacent ),
		graph( _graph ),
		frame( 0 ),
		checksum( 0 )
{}


MicroPather::~MicroPather()
{
}

    
void MicroPather::Reset()
{
	pathNodePool.Clear();
	frame = 0;
	checksum = 0;
}


void MicroPather::GoalReached( PathNode* node, void* start, void* end, vector< void* > *_path )
{
	std::vector< void* >& path = *_path;
	path.clear();

	// We have reached the goal.
	// How long is the path? Used to allocate the vector which is returned.
	int count = 1;
	PathNode* it = node;
	while( it->parent )
	{
		++count;
		it = it->parent;
	}

	// Now that the path has a known length, allocate
	// and fill the vector that will be returned.
	if ( count < 3 )
	{
		// Handle the short, special case.
		path.resize(2);
		path[0] = start;
		path[1] = end;
	}
	else
	{
		path.resize(count);

		path[0] = start;
		path[count-1] = end;
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
	for ( unsigned k=0; k<path.size(); ++k )
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


void MicroPather::GetNodeNeighbors( PathNode* node, std::vector< NodeCost >* pNodeCost )
{
	if ( node->numAdjacent == 0 ) {
		// it has no neighbors.
		pNodeCost->resize( 0 );
		return;
	}
	if ( node->cacheIndex < 0 )
	{
		// Not in the cache. Either the first time or just didn't fit. We don't know
		// the number of neighbors and need to call back to the client.
		stateCostVec.resize( 0 );
		graph->AdjacentCost( node->state, &stateCostVec );

		#ifdef DEBUG
		{
			// If this assert fires, you have passed a state
			// as its own neighbor state. This is impossible --
			// bad things will happen.
			for ( unsigned i=0; i<stateCostVec.size(); ++i )
				MPASSERT( stateCostVec[i].state != node->state );
		}
		#endif

		pNodeCost->resize( stateCostVec.size() );
		node->numAdjacent = stateCostVec.size();

		// Now convert to pathNodes.
		for( unsigned i=0; i<stateCostVec.size(); ++i ) {
			NodeCost nc;
			void* state = stateCostVec[i].state;

			nc.cost = stateCostVec[i].cost;
			nc.node = pathNodePool.FindPathNode( frame, state );
			if ( !nc.node ) {
				nc.node = pathNodePool.NewPathNode(	frame,
													state, 
													FLT_MAX, FLT_MAX, 
													0 );
			}
			(*pNodeCost)[i] = nc;
		}

		// Can this be cached?
		int start = 0;
		if ( pNodeCost->size() > 0 && pathNodePool.PushCache( &pNodeCost->at(0), pNodeCost->size(), &start ) ) {
			node->cacheIndex = start;
		}
	}
	else {
		// In the cache!
		pNodeCost->resize( node->numAdjacent );
		pathNodePool.GetCache( node->cacheIndex, node->numAdjacent, &pNodeCost->at(0) );

		// A node is uninitialized (even if memory is allocated) if it is from a previous frame.
		// Check for that, and Init() as necessary.
		for( int i=0; i<node->numAdjacent; ++i ) {
			PathNode* pNode = (*pNodeCost)[i].node;
			if ( pNode->frame != frame ) {
				pNode->Init( frame, pNode->state, FLT_MAX, FLT_MAX, 0 );
			}
		}
	}
}


#ifdef DEBUG
/*
void MicroPather::DumpStats()
{
	int hashTableEntries = 0;
	for( int i=0; i<HASH_SIZE; ++i )
		if ( hashTable[i] )
			++hashTableEntries;
	
	int pathNodeBlocks = 0;
	for( PathNode* node = pathNodeMem; node; node = node[ALLOCATE-1].left )
		++pathNodeBlocks;
	printf( "HashTableEntries=%d/%d PathNodeBlocks=%d [%dk] PathNodes=%d SolverCalled=%d\n",
			  hashTableEntries, HASH_SIZE, pathNodeBlocks, 
			  pathNodeBlocks*ALLOCATE*sizeof(PathNode)/1024,
			  pathNodeCount,
			  frame );
}
*/
#endif

/*
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
*/


int MicroPather::Solve( void* startNode, void* endNode, vector< void* >* path, float* cost )
{
	#ifdef DEBUG_PATH
	printf( "Path: " );
	graph->PrintStateInfo( startNode );
	printf( " --> " );
	graph->PrintStateInfo( endNode );
	printf( " min cost=%f\n", graph->LeastCostEstimate( startNode, endNode ) );
	#endif

	*cost = 0.0f;

	if ( startNode == endNode )
		return START_END_SAME;

	++frame;

	OpenQueue open( graph );
	ClosedSet closed( graph );
	
	PathNode* newPathNode = pathNodePool.NewPathNode(	frame, 
														startNode, 
														0, 
														graph->LeastCostEstimate( startNode, endNode ), 
														0 );

	open.Push( newPathNode );	
	stateCostVec.resize(0);
	nodeCostVec.resize(0);

	while ( !open.Empty() )
	{
		PathNode* node = open.Pop();
		
		if ( node->state == endNode )
		{
			GoalReached( node, startNode, endNode, path );
			*cost = node->costFromStart;
			#ifdef DEBUG_PATH
			DumpStats();
			#endif
			return SOLVED;
		}
		else
		{
			// We have not reached the goal - add the neighbors.
			GetNodeNeighbors( node, &nodeCostVec );

			for( int i=0; i<node->numAdjacent; ++i )
			{
				if ( nodeCostVec[i].cost == FLT_MAX ) {
					continue;
				}

				float newCost = node->costFromStart + nodeCostVec[i].cost;

				PathNode* inOpen   = nodeCostVec[i].node->inOpen ? nodeCostVec[i].node : 0;
				PathNode* inClosed = nodeCostVec[i].node->inClosed ? nodeCostVec[i].node : 0;
				MPASSERT( !( inOpen && inClosed ) );
				PathNode* inEither = inOpen ? inOpen : inClosed;

				MPASSERT( inEither != node );

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
					MPASSERT( !inEither );
					MPASSERT( nodeCostVec[i].node );

					PathNode* pNode = nodeCostVec[i].node;
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


int MicroPather::SolveForNearStates( void* startState, std::vector< StateCost >* near, float maxCost )
{
	/*	 http://en.wikipedia.org/wiki/Dijkstra%27s_algorithm

		 1  function Dijkstra(Graph, source):
		 2      for each vertex v in Graph:           // Initializations
		 3          dist[v] := infinity               // Unknown distance function from source to v
		 4          previous[v] := undefined          // Previous node in optimal path from source
		 5      dist[source] := 0                     // Distance from source to source
		 6      Q := the set of all nodes in Graph
				// All nodes in the graph are unoptimized - thus are in Q
		 7      while Q is not empty:                 // The main loop
		 8          u := vertex in Q with smallest dist[]
		 9          if dist[u] = infinity:
		10              break                         // all remaining vertices are inaccessible from source
		11          remove u from Q
		12          for each neighbor v of u:         // where v has not yet been removed from Q.
		13              alt := dist[u] + dist_between(u, v) 
		14              if alt < dist[v]:             // Relax (u,v,a)
		15                  dist[v] := alt
		16                  previous[v] := u
		17      return dist[]
	*/

	++frame;

	OpenQueue open( graph );			// nodes to look at
	ClosedSet closed( graph );

	nodeCostVec.resize(0);
	stateCostVec.resize(0);

	PathNode closedSentinel;
	closedSentinel.Clear();
	closedSentinel.Init( frame, 0, FLT_MAX, FLT_MAX, 0 );
	closedSentinel.next = closedSentinel.prev = &closedSentinel;

	PathNode* newPathNode = pathNodePool.NewPathNode( frame, startState, 0, 0, 0 );
	if ( !newPathNode ) {
		Reset();
		return OUT_OF_MEMORY;
	}
	open.Push( newPathNode );
	
	while ( !open.Empty() )
	{
		PathNode* node = open.Pop();	// smallest dist
		closed.Add( node );				// add to the things we've looked at
		closedSentinel.AddBefore( node );
			
		if ( node->totalCost > maxCost )
			continue;		// Too far away to ever get here.

		GetNodeNeighbors( node, &nodeCostVec );

		for( int i=0; i<node->numAdjacent; ++i )
		{
			MPASSERT( node->costFromStart < FLT_MAX );
			float newCost = node->costFromStart + nodeCostVec[i].cost;

			PathNode* inOpen   = nodeCostVec[i].node->inOpen ? nodeCostVec[i].node : 0;
			PathNode* inClosed = nodeCostVec[i].node->inClosed ? nodeCostVec[i].node : 0;
			MPASSERT( !( inOpen && inClosed ) );
			PathNode* inEither = inOpen ? inOpen : inClosed;
			MPASSERT( inEither != node );

			if ( inEither && inEither->costFromStart <= newCost ) {
				continue;	// Do nothing. This path is not better than existing.
			}
			// Groovy. We have new information or improved information.
			PathNode* child = nodeCostVec[i].node;
			MPASSERT( child->state != newPathNode->state );	// should never re-process the parent.

			child->parent = node;
			child->costFromStart = newCost;
			child->estToGoal = 0;
			child->totalCost = child->costFromStart;

			if ( inOpen ) {
				open.Update( inOpen );
			}
			else if ( !inClosed ) {
				open.Push( child );
			}
		}
	}	
	near->clear();

	for( PathNode* pNode=closedSentinel.next; pNode != &closedSentinel; pNode=pNode->next ) {
		if ( pNode->totalCost <= maxCost ) {
			StateCost sc;
			sc.cost = pNode->totalCost;
			sc.state = pNode->state;

			near->push_back( sc );
		}
	}
#ifdef DEBUG
	for( unsigned i=0; i<near->size(); ++i ) {
		for( unsigned k=i+1; k<near->size(); ++k ) {
			MPASSERT( near->at(i).state != near->at(k).state );
		}
	}
#endif

	return SOLVED;
}





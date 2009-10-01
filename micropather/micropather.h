/*
Copyright (c) 2000-2003 Lee Thomason (www.grinninglizard.com)
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


#ifndef GRINNINGLIZARD_MICROPATHER_INCLUDED
#define GRINNINGLIZARD_MICROPATHER_INCLUDED


/** @mainpage MicroPather
	
	MicroPather is a path finder and A* solver (astar or a-star) written in platform independent 
	C++ that can be easily integrated into existing code. MicroPather focuses on being a path 
	finding engine for video games but is a generic A* solver. MicroPather is open source, with 
	a license suitable for open source or commercial use.

	An overview of using MicroPather is in the <A HREF="../readme.htm">readme</A> or
	on the Grinning Lizard website: http://grinninglizard.com/micropather/
*/

#include <vector>
#include <assert.h>
#include <float.h>

#ifdef _DEBUG
	#ifndef DEBUG
		#define DEBUG
	#endif
#endif

#ifndef GRINLIZ_TYPES_INCLUDED
	#if defined(_MSC_VER) && (_MSC_VER >= 1400 )
		#include <stdlib.h>
		typedef uintptr_t		UPTR;
	#elif defined (__GNUC__) && (__GNUC__ >= 3 )
		#include <stdlib.h>
		typedef uintptr_t		UPTR;
	#else
		// Assume not 64 bit pointers. Get a new compiler.
		typedef unsigned UPTR;
	#endif
#endif

namespace micropather
{
	const float FLT_BIG = FLT_MAX / 2.0f;	

	/**
		Used to pass the cost of states from the cliet application to MicroPather. This
		structure is copied in a vector.

		@sa AdjacentCost
	*/
	struct StateCost
	{
		void* state;			///< The state as a void*
		float cost;				///< The cost to the state. Use FLT_MAX for infinite cost.
	};


	/**
		A pure abstract class used to define a set of callbacks. 
		The client application inherits from 
		this class, and the methods will be called when MicroPather::Solve() is invoked.

		The notion of a "state" is very important. It must have the following properties:
		- Unique
		- Unchanging (unless MicroPather::Reset() is called)

		If the client application represents states as objects, then the state is usually
		just the object cast to a void*. If the client application sees states as numerical
		values, (x,y) for example, then state is an encoding of these values. MicroPather
		never interprets or modifies the value of state.
	*/
	class Graph
	{
	  public:
		virtual ~Graph() {}
	  
		/**
			Return the least possible cost between 2 states. For example, if your pathfinding 
			is based on distance, this is simply the straight distance between 2 points on the 
			map. If you pathfinding is based on minimum time, it is the minimal travel time 
			between 2 points given the best possible terrain.
		*/
		virtual float LeastCostEstimate( void* stateStart, void* stateEnd ) = 0;

		/** 
			Return the exact cost from the given state to all its neighboring states. This
			may be called multiple times, or cached by the solver. It *must* return the same
			exact values for every call to MicroPather::Solve(). It should generally be a simple,
			fast function with no callbacks into the pather.
		*/	
		virtual void AdjacentCost( void* state, std::vector< micropather::StateCost > *adjacent ) = 0;

		/**
			This function is only used in DEBUG mode - it dumps output to stdout. Since void* 
			aren't really human readable, normally you print out some concise info (like "(1,2)") 
			without an ending newline.

			@note If you are using other grinning lizard utilities, you should use GLOUTPUT for output.
		*/
		virtual void  PrintStateInfo( void* state ) = 0;
	};


	class PathNode;

	struct NodeCost
	{
		PathNode* node;
		float cost;
	};


	class PathNode
	{
	  public:
		void Init(	unsigned _frame,
					void* _state,
					float _costFromStart, 
					float _estToGoal, 
					PathNode* _parent,
					bool clearCache = true )
		{
			state = _state;
			costFromStart = _costFromStart;
			estToGoal = _estToGoal;
			CalcTotalCost();
			parent = _parent;
			frame = _frame;
			if ( clearCache ) {
				numAdjacent = -1;
				cacheIndex = -1;
			}
			left = 0;
			next = 0;
			prev = 0;
			inOpen = 0;
			inClosed = 0;
		}

		void *state;			// the client state
		float costFromStart;	// exact
		float estToGoal;		// estimated
		float totalCost;		// could be a function, but save some math.
		PathNode* parent;		// the parent is used to reconstruct the path
		unsigned frame;			// unique id for this path, so the solver can distinguish
								// correct from stale values

		// If there are 4 or less adjacent states, they will be cached as *nodes*.
		//NodeCost adjacent[ MAX_CACHE ];
		int numAdjacent;				// -1  is unknown & needs to be queried
		int cacheIndex;					// position in cache

		PathNode* left;			// Used as a "next" pointer for memory layout.
		PathNode *next, *prev;	// used by open queue

		bool inOpen;
		bool inClosed;

		void Unlink() {
			next->prev = prev;
			prev->next = next;
			next = prev = 0;
		}
		void AddBefore( PathNode* addThis ) {
			addThis->next = this;
			addThis->prev = prev;
			prev->next = addThis;
			prev = addThis;
		}
		#ifdef DEBUG
		void CheckList()
		{
			assert( totalCost == FLT_MAX );
			for( PathNode* it = next; it != this; it=it->next ) {
				assert( it->prev == this || it->totalCost >= it->prev->totalCost );
				assert( it->totalCost <= it->next->totalCost );
			}
		}
		#endif

	  private:
		void CalcTotalCost() {
			if ( costFromStart < FLT_MAX && estToGoal < FLT_MAX )
				totalCost = costFromStart + estToGoal;
			else
				totalCost = FLT_MAX;
		}

		void operator=( const PathNode& );
	};


	class PathNodePool
	{
	public:
		PathNodePool( unsigned allocate, unsigned typicalAdjacent );
		~PathNodePool();

		// Free all the memory except the first block. Resets all memory.
		void Clear();

		PathNode* FindPathNode(		unsigned frame, 
									void* state );
		PathNode* NewPathNode(		unsigned frame,
									void* _state,
									float _costFromStart, 
									float _estToGoal, 
									PathNode* _parent );

		// Note - always access this with an offset. Can get re-allocated.
		bool PushCache( const NodeCost* nodes, int nNodes, int* start ) {
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
		void GetCache( int start, int nNodes, NodeCost* nodes ) {
			assert( start >= 0 && start < cacheCap );
			assert( nNodes > 0 );
			assert( start + nNodes <= cacheCap );
			for( int i=0; i<nNodes; ++i )
				nodes[i] = cache[start+i];
		}

	private:
		struct Block
		{
			Block* nextBlock;
			PathNode pathNode[1];
		};

		unsigned Hash( void* voidval ) {
			// FNV-1a
			// http://isthe.com/chongo/tech/comp/fnv/
			// public domain.
			UPTR val = (UPTR)(voidval);
			unsigned char *bp = (unsigned char *)(&val);
			unsigned char *be = bp + sizeof(UPTR);
			unsigned int h = 2166136261;

			while (bp < be) {
				h ^= *bp++;
				h *= 16777619;
			}
			return h & (hashSize-1);
		}

		Block* NewBlock();
		PathNode* Alloc();

		PathNode**	hashTable;
		Block*		firstBlock;
		Block*		blocks;
		NodeCost*	cache;
		int			cacheCap;
		int			cacheSize;

		PathNode	sentinel;
		unsigned	allocate;				// how big a block of pathnodes to allocate at once
		unsigned	nAllocated;				// number of pathnodes allocated (from Alloc())
		unsigned	nAvailable;				// number available for allocation
		unsigned	hashSize;
	};


	/**
		Create a MicroPather object to solve for a best path. Detailed usage notes are
		on the main page.
	*/
	class MicroPather
	{
		friend class micropather::PathNode;

	  public:
		enum
		{
			SOLVED,
			NO_SOLUTION,
			START_END_SAME,
			OUT_OF_MEMORY
		};

		/**
			Construct the pather, passing a pointer to the object that implements
			the Graph callbacks.

			@param graph		The "map" that implements the Graph callbacks.
			@param allocate		The block size that the node cache is allocated from. In some
								cases setting this parameter will improve the perfomance of
								the pather.
								- If you have a small map, for example the size of a chessboard, set allocate
								  to be the number of states+1. 65 for a chessboard. This will allow
								  MicroPather to used a fixed amount of memory.
								- If your map is large, something like 1/4 the number of possible
								  states is good. For example, Lilith3D normally has about 16,000 
								  states, so 'allocate' should be about 4000.
			@param noAllocate	If true, then the "allocate" parameter is a hard limit. It will return
								OUT_OF_MEMORY if enough memory isn't available.
		*/
		MicroPather( Graph* graph, unsigned allocate = 250, unsigned typicalAdjacent=4 );
		~MicroPather();

		/**
			Solve for the path from start to end.

			@param startState	Input, the starting state for the path.
			@param endState		Input, the ending state for the path.
			@param path			Output, a vector of states that define the path. Empty if not found.
			@param totalCost	Output, the cost of the path, if found.
			@return				Success or failure, expressed as SOLVED, NO_SOLUTION, or START_END_SAME.
		*/
		int Solve( void* startState, void* endState, std::vector< void* >* path, float* totalCost );

		int SolveForNearStates( void* startState, std::vector< StateCost >* near, float maxCost );

		/// Should be called whenever the cost between states or the connection between states changes.
		void Reset();

		/**
			Return the "checksum" of the last path returned by Solve(). Useful for debugging,
			and a quick way to see if 2 paths are the same.
		*/
		UPTR Checksum()	{ return checksum; }
		//unsigned PathNodesAllocated() { return pathNodeCount; }


		// Debugging function to return all states that were used
		// by the last "solve" 
		void StatesInPool( std::vector< void* >* stateVec );

	  private:
		MicroPather( const MicroPather& );	// undefined and unsupported
		void operator=( const MicroPather ); // undefined and unsupported
		
		void GoalReached( PathNode* node, void* start, void* end, std::vector< void* > *path );

		void GetNodeNeighbors(	PathNode* node, std::vector< NodeCost >* neighborNode );
		
		// Allocates and returns memory for a new, unititialized node.
		//PathNode* AllocatePathNode();

		// Returns a path node that is ready to go. 
		// Strictly speaking, the name is somewhat misleading, as it
		// may be reused from the cache, but it will be ready for use regardless.
		//PathNode* NewPathNode( void* state, float costFromStart, float estToEnd, PathNode* parent );

		// Finds the node for state, or returns null. 
		// A node that doesn't have a correct "frame" will never be found.
		//PathNode* FindPathNode( void* state ); 

		// When solving for near states, populates the possible open set.
		//int PopulateOpen( float maxCost, OpenQueue* open, void* startState, PathNode* parentNode )

		#ifdef DEBUG
		//void DumpStats();
		#endif

		//unsigned BLOCKSIZE;					// how many useable pathnodes are in a block
		//unsigned MAX_ADJACENT;				// maximum # of adjacent states (static memory only)
		//unsigned MAX_PATH;					// maximum length of path (static memory only)

		Graph* graph;
		unsigned frame;						// incremented with every solve, used to determine
											// if cached data needs to be refreshed
		UPTR checksum;						// the checksum of the last successful "Solve".

		PathNodePool				pathNodePool;
		std::vector< StateCost >	stateCostVec;	// local to Solve, but put here to reduce memory allocation
		std::vector< NodeCost >		nodeCostVec;	// local to Solve, but put here to reduce memory allocation
		std::vector< PathNode* >	closedSet;		// local to SolveForNearStates, but put here to reduce memory allocation
	};
};	// namespace grinliz

#endif


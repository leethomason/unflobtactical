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


#ifndef GRINLIZ_PUBLISHER_INCLUDED
#define GRINLIZ_PUBLISHER_INCLUDED

#ifdef _MSC_VER
#pragma warning( disable : 4786 )
#endif

#include <set>

namespace grinliz {

/**
	A Publisher communicates to a Listener by calling its methods.
	If a publisher is deleted, it will clear the pointers to its listener,
	and if a listener is deleted it will clear the pointes to its
	publisher. Therefore they can have independent lifespans.
	
	A publisher is aggregated into the class that wants to publish:
	
	@verbatim
	class Switch
	{
		Publisher<SwitchListener> publish; 	
	}
	@endverbatim
*/
template< class LISTENER_CLASS >
class Publisher
{
  public:

    typedef typename std::set<LISTENER_CLASS*>::iterator iterator;
	/// Used to iterate through the listeners.
    typedef typename std::set<LISTENER_CLASS*>::const_iterator const_iterator;
     
	Publisher() : copyValid( false )	{}
	  
	~Publisher()
	{
		for(	iterator listenerIterator = listeners.begin();
				listenerIterator != listeners.end();
				++listenerIterator )
		{
			(*listenerIterator)->publishers.erase( this );
		}
	}

	/// Add a listener.
	void AddListener( LISTENER_CLASS* add )			
	{ 
		copyValid = false;
		listeners.insert( add ); 
		add->publishers.insert( this );
	}

	/** Remove a listener. This is safe to call at any time, even during a callback.
	*/
	void RemoveListener( LISTENER_CLASS* remove )	
	{ 
		copyValid = false;
		remove->publishers.erase( this );
		listeners.erase( remove );
	}
 
 	/** An iterator for the publisher to emit messages.
		The code usually looks like:
		@verbatim
		for( Publisher<SwitchListener>::const_iterator it = publish.begin();
			 it != publish.end();
			 ++it )
		{
			(*it)->Click();
		}
		@endverbatim
	*/
    const_iterator begin()	{	if ( !copyValid )
								{
									listenersCopy.clear();
									listenersCopy = listeners;
									copyValid = true;
								}
								return listenersCopy.begin(); 
							}

 	/// An iterator for the publisher to emit messages.
    const_iterator end()      { return listenersCopy.end(); }

	bool empty() { return listeners.empty(); }
 
  private:
	std::set<LISTENER_CLASS*> listeners;
	// Okay, it's very natural to want to Add/Remove while iterating
	// through the set. Even with documentation, it's just too easy
	// a mistake to make. Changing the set while iterating will 
	// crash and invalidate the	set. To overcome this, begin() makes a copy.
	// But we keep a flag around so this doesn't happen unless it needs to.
	bool copyValid;
	std::set<LISTENER_CLASS*> listenersCopy;
};


/**
	An interface used by any class that wants to listen to a publisher.
	Trickiness has gone into making this class very easy to use. To use
	it:
	
	@verbatim
	class SwitchListener : public Listener< SwitchListener >
	{
	  public:
	  	void Click() = 0;
	}
	@endverbatim
	
	Note that the type of the Listener template is the interface being
	created.
	
	The client code would then use this by:
	@verbatim
	class Light : public SwitchListener
	@endverbatim
	and implementing the abstract methods.
*/
template <class LISTENER_CLASS>
class Listener
{
	friend class Publisher< LISTENER_CLASS >;

  private:
	std::set< Publisher<LISTENER_CLASS>* > publishers;
    typedef typename std::set< Publisher<LISTENER_CLASS>* >::iterator iterator;

  public:
	virtual ~Listener()
	{
	    // This a strange bit of code. The RemoveListener call
	    // will result the Publisher class reaching into this class
	    // to erase the Publisher*. Therefore, the iterator will 
	    // continually be invalidated; hence always use begin() and 
	    // break if end().
	    //
	    // The other odd bit of code is the back-cast to our child
	    // type - LISTENER_CLASS. Sleazy c++ trick.
	    //
		while ( true )
		{
			iterator it = publishers.begin();
			if ( it == publishers.end() ) break;
			(*it)->RemoveListener( static_cast< LISTENER_CLASS* >( this ) );
		}
	}
};

};	// namespace grinliz
#endif

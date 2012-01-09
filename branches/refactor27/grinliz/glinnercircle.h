/*
Copyright (c) 2000-2007 Lee Thomason (www.grinninglizard.com)
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

#ifndef GRINLIZ_INNER_CIRCLE_INCLUDED
#define GRINLIZ_INNER_CIRCLE_INCLUDED

namespace grinliz
{

template<class T>
class InnerCircle
{
  public:
	enum Sentinel
	{
		SENTINEL
	};

	InnerCircle( Sentinel ) {
		next = prev = this;
		container = 0;
	} 

	InnerCircle( T* _container )	{ 
		GLASSERT( _container );
		this->container = _container;
		next = prev = 0;
	}

	~InnerCircle() {
		if ( !Sentinel() )
			Remove();
		container = 0;	// silly, but useful for debugging
	}

	void Add( InnerCircle* addThis )
	{
		GLASSERT( addThis->next == 0 );
		GLASSERT( addThis->prev == 0 );
		GLASSERT( addThis->Sentinel() == false );

		if ( next )
		{
			next->prev = addThis;
			addThis->next = next;
		}
		next = addThis;
		addThis->prev = this;
	}

	void Remove() {
		GLASSERT( ( prev && next ) || (!prev && !next ) );
		GLASSERT( !Sentinel() );
		if ( prev ) {
			prev->next = next;
			next->prev = prev;
			prev = next = 0;
		}
	}

	void RemoveAndDelete() {
		Remove();
		delete container;
	}

	bool InList() {
		GLASSERT( !Sentinel() );
		GLASSERT( ( prev && next ) || (!prev && !next ) );
		return prev != 0;
	}

	T* Container()			{ return container; }
	InnerCircle<T>* Next()	{ return next; }
	InnerCircle<T>* Prev()	{ return prev; }
	bool Sentinel()			{ return !container; }

  private:
	InnerCircle *next, *prev;
	T* container;
};


};	// namespace grinliz

#endif

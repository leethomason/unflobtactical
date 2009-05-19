#ifndef UFOATTACK_SCREENPORT_INCLUDED
#define UFOATTACK_SCREENPORT_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"

/*
	The default iPhone coordinate system:
	0--->x
	|
	|
	|
	|
	y
	   O

	At rotation=1
	y-----0
		  |
		  x
*/
class Screenport
{
public:
	Screenport( int physWidth, int physHeight, int rotation ) {
		GLASSERT( physWidth == 320 );	// can change, but need to test
		GLASSERT( physHeight == 480 );
		//GLASSERT( rotation = 1 );

		this->physicalWidth = physWidth;
		this->physicalHeight = physHeight;
		this->rotation = rotation;
	}

	Screenport( const Screenport& other ) {
		this->physicalWidth = other.physicalWidth;
		this->physicalHeight = other.physicalHeight;
		this->rotation = other.rotation;
	}

	int Rotation() const		{ return rotation; }

	// These values reflect the rotated screen:
	int ViewWidth()	const	{ return (rotation&1) ? physicalHeight : physicalWidth; }
	int ViewHeight() const	{ return (rotation&1) ? physicalWidth : physicalHeight; }
	void ScreenToView( int physicalX, int physicalY, int* viewX, int* viewY ) const;

	// These reflect the physical screen:
	int PhysicalWidth() const	{ return physicalWidth; }
	int PhysicalHeight() const	{ return physicalHeight; }

	void PushView() const;
	void PopView() const;
private:
	int rotation;			// 1
	int physicalWidth;		// 480 
	int physicalHeight;		// 320
};

#endif	// UFOATTACK_SCREENPORT_INCLUDED
#ifndef UFOATTACK_SCREENPORT_INCLUDED
#define UFOATTACK_SCREENPORT_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"

/*
	PHYSICAL / SCREEN coordinates:

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

  VIEW coordinates:
  y
  |
  |
  0----x

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

	// These values reflect the rotated screen. A very simple transform that moves
	// the origin to the lower left, independent of rotation.
	void ScreenToView( int physicalX, int physicalY, int* viewX, int* viewY ) const;


	// These reflect the physical screen:
	int PhysicalWidth() const	{ return physicalWidth; }
	int PhysicalHeight() const	{ return physicalHeight; }

	// UI: origin in lower left, oriented with device.
	void PushUI() const;
	void PopUI() const;
	void ViewToUI( int physicalX, int physicalY, int* viewX, int* viewY ) const;
	int UIWidth() const	{ return ViewWidth(); }
	int UIHeight() const	{ return ViewHeight(); }
private:
	// too easy to screw up
	int ViewWidth()	const	{ return (rotation&1) ? physicalHeight : physicalWidth; }
	int ViewHeight() const	{ return (rotation&1) ? physicalWidth : physicalHeight; }

	int rotation;			// 1
	int physicalWidth;		// 480 
	int physicalHeight;		// 320
};

#endif	// UFOATTACK_SCREENPORT_INCLUDED
#ifndef UFOATTACK_SCREENPORT_INCLUDED
#define UFOATTACK_SCREENPORT_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"

/*
	** PHYSICAL / SCREEN coordinates:

	The default iPhone coordinate system:
	0--->x
	|
	|
	|
	|
	y
    Independent of rotation.
	On the iPod, x=320 and y=480

	So at rotation=1, from the viewpoint of the person holding the device:
	y-----0
		  |
		  x

	** VIEW coordinates:
	y
	|
	|
	0----x
	Independent of rotation.

	** UI coordinates:

	Put the origin in the lower left accounting for rotation. So the lower left
	from the point of view of the person holding the device.

*/
class Screenport
{
public:
	Screenport( int physWidth, int physHeight, int rotation ) {
		//GLASSERT( physWidth == 320 );	// can change, but need to test
		//GLASSERT( physHeight == 480 );
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

	void ViewToUI( int vX, int vY, int* uiX, int* uiY ) const;
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
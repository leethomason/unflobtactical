#ifndef UFOATTACK_SCREENPORT_INCLUDED
#define UFOATTACK_SCREENPORT_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glmatrix.h"

namespace grinliz {
	struct Rectangle2I;
};

/*
	** SCREEN coordinates:
	Screen coordinates reflect the "natural" coordinates of the device. Generally
	(0,0) in the upper left at the natural rotation. Currently these are required
	to be 320x480 even if the screen is a different size. (The viewport will scale.)

	The default iPhone (and bitmap) coordinate system:
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
	View coordinates are the OpenGL based coordinates. OpenGL moves the origin
	to the lower left. These are still in pixels, not normalized.

	y
	|
	|
	0----x
	Independent of rotation.

	** UI coordinates:
	Put the origin in the lower left accounting for rotation. So the lower left
	from the point of view of the person holding the device. The w=480 and the 
	h=320 always, regardless of actual screen. Used to define UI positions in
	a rational way.

*/
class Screenport
{
public:
	Screenport( int screenWidth, int screenHeight, int rotation ) 
	{
		this->screenWidth = screenWidth;
		this->screenHeight = screenHeight;
		this->rotation = rotation;
		for( int i=0; i<4; ++i )
			this->viewport[i] = 0;
		uiPushed = false;
	}

	Screenport( const Screenport& other ) 
	{
		this->screenWidth = other.screenWidth;
		this->screenHeight = other.screenHeight;
		this->rotation = other.rotation;
		for( int i=0; i<4; ++i )
			this->viewport[i] = other.viewport[i];
		uiPushed = other.uiPushed;
	}

	int Rotation() const		{ return rotation; }

	// These values reflect the rotated screen. A very simple transform that moves
	// the origin to the lower left, independent of rotation.
	void ScreenToView( int physicalX, int physicalY, int* viewX, int* viewY ) const;

	// These reflect the physical screen:
	int ScreenWidth() const		{ return screenWidth; }
	int ScreenHeight() const	{ return screenHeight; }

	// UI: origin in lower left, oriented with device.
	void PushUI();
	void PopUI();

	void SetPerspective( float frustumLeft, float frustumRight, float frustumBottom, float frustumTop, float frustumNear, float frustumFar );
	void SetView( const grinliz::Matrix4& view );

	const grinliz::Matrix4& ProjectionMatrix()	{ return projection; }
	const grinliz::Matrix4& ViewMatrix()		{ return view; }

	void ViewToUI( int vX, int vY, int* uiX, int* uiY ) const;
	int UIWidth() const			{ return (rotation&1) ? screenHeight : screenWidth; }
	int UIHeight() const		{ return (rotation&1) ? screenWidth : screenHeight; }

	// Set the viewport and clipping - null sets to full screen.
	void SetClipping( const grinliz::Rectangle2I* uiClip );

	// Convert from UI coordinates to scissor coordinates. Does the 
	// UI to pixel (accounting for viewport) back xform.
	void UIToScissor( int x, int y, int w, int h, grinliz::Rectangle2I* clip ) const;

private:

	int rotation;			// 1
	int screenWidth;		// 480 
	int screenHeight;		// 320
	int viewport[4];		// from the gl call

	bool uiPushed;
	grinliz::Matrix4 projection, view;
	grinliz::Matrix4 savedProjection, savedView;
};

#endif	// UFOATTACK_SCREENPORT_INCLUDED
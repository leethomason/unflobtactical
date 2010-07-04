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

#ifndef UFOATTACK_SCREENPORT_INCLUDED
#define UFOATTACK_SCREENPORT_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glmatrix.h"
#include "../grinliz/glrectangle.h"
#include "../grinliz/glgeometry.h"

namespace grinliz {
	struct Rectangle2I;
};


struct Frustum
{
	float left, right, bottom, top, zNear, zFar;
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
	Screenport( int screenWidth, int screenHeight, int rotation ); 

	int Rotation() const		{ return rotation; }

	// These values reflect the rotated screen. A very simple transform that moves
	// the origin to the lower left, independent of rotation.
	void ScreenToView( int screenX, int screenY, grinliz::Vector2I* view ) const
	{
		view->x = screenX;
		view->y = (screenHeight-1)-screenY;
	}
	void ViewToScreen( int viewX, int viewY, grinliz::Vector2I* screen ) const
	{
		screen->x = viewX;
		screen->y = (screenHeight-1)-viewY;
	}
	void UIToView( const grinliz::Vector2I& in, grinliz::Vector2I* out ) const;

	void ScreenToWorld( int screenX, int screenY, grinliz::Ray* world ) const;
	bool ViewToWorld( const grinliz::Vector3F& screen, const grinliz::Matrix4& mvpi, grinliz::Vector3F* world ) const;
	void WorldToScreen( const grinliz::Vector3F& p0, grinliz::Vector2F* view ) const;
	void WorldToUI( const grinliz::Vector3F& p0, grinliz::Vector2I* ui ) const;

	// These reflect the physical screen:
	int ScreenWidth() const		{ return screenWidth; }
	int ScreenHeight() const	{ return screenHeight; }

	// UI: origin in lower left, oriented with device.
	// Sets both the MODELVIEW and the PROJECTION for UI coordinates. (The view is not set.)
	// The clip is interpreted as the location where the UI can be.
	void SetUI( const grinliz::Rectangle2I* clipInUI, bool flipped );

	// Set the perspective PROJECTION.
	void SetPerspective( float near, float far, float fovDegrees, const grinliz::Rectangle2I* clipInUI );

	// Set the MODELVIEW from the camera.
	void SetView( const grinliz::Matrix4& view );

	const grinliz::Matrix4& ProjectionMatrix3D() const				{ return projection3D; }
	const grinliz::Matrix4& ViewMatrix3D() const					{ return view3D; }
	void ViewProjection3D( grinliz::Matrix4* vp ) const				{ grinliz::MultMatrix4( projection3D, view3D, vp ); }
	void ViewProjectionInverse3D( grinliz::Matrix4* vpi ) const		{ grinliz::Matrix4 vp;
																	  ViewProjection3D( &vp );
																	  vp.Invert( vpi );
																	}

	const Frustum&		    GetFrustum()		{ GLASSERT( uiMode == false ); return frustum; }

	void ViewToUI( int vX, int vY, int* uiX, int* uiY ) const;
	void ViewToGUI( int x0, int y0, float *x1, float *y1 ) const;

	int UIWidth() const										{ return (rotation&1) ? screenHeight : screenWidth; }
	int UIHeight() const									{ return (rotation&1) ? screenWidth : screenHeight; }
	void UIBoundsClipped3D( grinliz::Rectangle2I* b ) const	{ *b = clipInUI3D; }
	void UIBoundsClipped2D( grinliz::Rectangle2I* b ) const	{ *b = clipInUI2D; }
	bool UIMode() const										{ return uiMode; }

private:
	Screenport( const Screenport& other );
	void operator=( const Screenport& other );

	// Convert from UI coordinates to scissor coordinates. Does the 
	// UI to pixel (accounting for viewport) back xform. Makes it a somewhat
	// evil transform - coordinates are used (actual pixels) that aren't used
	// in any other xform.
	void UIToScissor( int x, int y, int w, int h, grinliz::Rectangle2I* clip ) const;
	void SetViewport( const grinliz::Rectangle2I* uiClip );

	int rotation;			// 1
	int screenWidth;		// 480 
	int screenHeight;		// 320

	int viewport[4];		// from the gl call
	float viewPortScale;

	bool uiMode;
	grinliz::Rectangle2I clipInUI2D, clipInUI3D;
	Frustum frustum;
	grinliz::Matrix4 projection2D, view2D;
	grinliz::Matrix4 projection3D, view3D;
};

#endif	// UFOATTACK_SCREENPORT_INCLUDED
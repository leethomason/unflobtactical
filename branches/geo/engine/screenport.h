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
	** PHYSICAL/WINDOW coordinates:
	THe actual dimensions, in pixels, of the screen.

	** VIEW coordinates:
	View coordinates are the OpenGL based coordinates. OpenGL moves the origin
	to the lower left. This is a scaled pixel coordinate system. (Eek.)

	y
	|
	|
	0----x
	Independent of rotation.

	** UI coordinates:
	Put the origin in the upper left accounting for rotation. So the upper left
	from the point of view of the person holding the device. The h=320 
	always, regardless of actual screen, so these are scaled coordinates.
	Used to define UI positions in a rational way.
*/
class Screenport
{
public:
	Screenport( int screenWidth, int screenHeight, int rotation ); 
	//Screenport( const Screenport& other );

	void Resize( int w, int h, int r );
	float UIAspectRatio()		{ return UIHeight() / UIWidth(); }

	int Rotation() const		{ return rotation; }

	void UIToView( const grinliz::Vector2F& in, grinliz::Vector2F* view ) const;
	void ViewToUI( const grinliz::Vector2F& in, grinliz::Vector2F* ui ) const;

	bool ViewToWorld( const grinliz::Vector2F& view, const grinliz::Matrix4* mvpi, grinliz::Ray* world ) const;
	void WorldToView( const grinliz::Vector3F& world, grinliz::Vector2F* view ) const;

	void WorldToUI( const grinliz::Vector3F& world, grinliz::Vector2F* ui ) const {
		grinliz::Vector2F view;
		WorldToView( world, &view );
		ViewToUI( view, ui );
	}

	// Primarily used in scissor. Returs in physical window coordinates.
	void ViewToWindow( const grinliz::Vector2F& view, grinliz::Vector2F* window ) const;
	void WindowToView( const grinliz::Vector2F& window, grinliz::Vector2F* view ) const;

	// UI: origin in lower left, oriented with device.
	// Sets both the MODELVIEW and the PROJECTION for UI coordinates. (The view is not set.)
	// The clip is interpreted as the location where the UI can be.
	void SetUI( const grinliz::Rectangle2I* clipInUI );

	// Set the perspective PROJECTION.
	void SetPerspective( const grinliz::Rectangle2I* clipInUI );

	// Set the MODELVIEW from the camera.
	void SetView( const grinliz::Matrix4& view );
	void SetViewMatrices( const grinliz::Matrix4& _view )			{ view2D = _view; view3D = _view; }

	const grinliz::Matrix4& ProjectionMatrix3D() const				{ return projection3D; }
	const grinliz::Matrix4& ViewMatrix3D() const					{ return view3D; }
	void ViewProjection3D( grinliz::Matrix4* vp ) const				{ grinliz::MultMatrix4( projection3D, view3D, vp ); }
	void ViewProjectionInverse3D( grinliz::Matrix4* vpi ) const		{ grinliz::Matrix4 vp;
																	  ViewProjection3D( &vp );
																	  vp.Invert( vpi );
																	}

	const Frustum&		    GetFrustum()		{ GLASSERT( uiMode == false ); return frustum; }

	float UIWidth() const									{ return (rotation&1) ? screenHeight : screenWidth; }
	float UIHeight() const									{ return (rotation&1) ? screenWidth : screenHeight; }
	const grinliz::Rectangle2F& UIBoundsClipped3D() const	{ return clipInUI3D; }
	const grinliz::Rectangle2F& UIBoundsClipped2D() const	{ return clipInUI2D; }

	bool UIMode() const										{ return uiMode; }

private:
	//void operator=( const Screenport& other );

	
	void UIToWindow( const grinliz::Rectangle2F& ui, grinliz::Rectangle2F* clip ) const;

	int rotation;		
	float screenWidth; 
	float screenHeight;		// if rotation==0, 320

	float physicalWidth;
	float physicalHeight;

	//float vScale, hScale;

	bool uiMode;
	grinliz::Rectangle2F clipInUI2D, clipInUI3D;
	Frustum frustum;
	grinliz::Matrix4 projection2D, view2D;
	grinliz::Matrix4 projection3D, view3D;
};

#endif	// UFOATTACK_SCREENPORT_INCLUDED
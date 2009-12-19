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

#pragma warning ( disable : 4530 )		// Don't warn about unused exceptions.

#include "glew.h"
#include "SDL.h"

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glutil.h"
#include "../grinliz/glvector.h"
#include "../game/cgame.h"
#include "framebuffer.h"

#if defined(MAPMAKER) || defined(_MSC_VER)
#include "../game/game.h"
#endif

#include "wglew.h"


#define IPOD_SCREEN_WIDTH	320
#define IPOD_SCREEN_HEIGHT	480
//#define FRAMEBUFFER_ROTATE

int multisample = 0;
bool fullscreen = false;
//#ifdef FRAMEBUFFER_ROTATE
//	const int rotation = 1;
//#else
	const int rotation = 0;
//#endif

void ScreenCapture( const char* baseFilename );


void TransformXY( int x0, int y0, int* x1, int* y1 )
{
//#ifdef FRAMEBUFFER_ROTATE
//	*x1 = y0;
//	*y1 = IPOD_SCREEN_HEIGHT-1-x0;
//#else
	*x1 = x0;
	*y1 = y0;
//#endif	
}


void XferTexture( U32 id, int _w, int _h )
{
	float x = 0.f;
	float y = 0.f;
	float w = (float)_w;
	float h = (float)_h;

	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, id );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	// DO NOT ENABLE THIS. BLUESCREENs my ATI card.
	/*
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmapEXT(GL_TEXTURE_2D);
	*/

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();					// save projection
	glLoadIdentity();				// projection

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();					// model
	glLoadIdentity();				// model

	int viewport[ 4 ];
	glGetIntegerv( GL_VIEWPORT, viewport );
	glOrtho( viewport[0], viewport[2], viewport[1], viewport[3], -1, 1 );

	glBegin( GL_QUADS );
	glTexCoord2f( 1.0f, 0.0f );
	glVertex2f( x, y );

	glTexCoord2f( 1.0f, 1.0f );
	glVertex2f( x+w, y );

	glTexCoord2f( 0.0f, 1.0f );
	glVertex2f( x+w, y+h );

	glTexCoord2f( 0.0f, 0.0f );
	glVertex2f( x, y+h );
	glEnd();

	glPopMatrix();					// model
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();					// projection
	glMatrixMode(GL_MODELVIEW);
}


Uint32 TimerCallback(Uint32 interval, void *param)
{
	SDL_Event user;
	memset( &user, 0, sizeof(user ) );
	user.type = SDL_USEREVENT;

	SDL_PushEvent( &user );
	return interval;
}


int main( int argc, char **argv )
{    
	MemStartCheck();
	{ char* test = new char[16]; delete [] test; }

#ifdef MAPMAKER	
	std::string filename = "./resin/";
	if ( argc > 1 ) {
		filename += argv[1];
		filename += ".map";
	}
#endif


	SDL_Surface *surface = 0;

	// SDL initialization steps.
    if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER ) < 0 )
	{
	    fprintf( stderr, "SDL initialization failed: %s\n", SDL_GetError( ) );
		exit( 1 );
	}
	SDL_EnableKeyRepeat( 0, 0 );
	SDL_EnableUNICODE( 1 );

	const SDL_version* sversion = SDL_Linked_Version();
	GLOUTPUT(( "SDL: major %d minor %d patch %d\n", sversion->major, sversion->minor, sversion->patch ));

	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	//SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 0 );
	//SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8);

	if ( multisample ) {
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 1 );
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, multisample );
	}

	int	videoFlags  = SDL_OPENGL;          /* Enable OpenGL in SDL */
		videoFlags |= SDL_GL_DOUBLEBUFFER; /* Enable double buffering */

	if ( fullscreen )
		videoFlags |= SDL_FULLSCREEN;

	// Note that our output surface is rotated from the iPod.
	surface = SDL_SetVideoMode( IPOD_SCREEN_HEIGHT, IPOD_SCREEN_WIDTH, 32, videoFlags );
	GLASSERT( surface );

	int stencil = 0;
	int depth = 0;
	SDL_GL_GetAttribute( SDL_GL_STENCIL_SIZE, &stencil );
	glGetIntegerv( GL_DEPTH_BITS, &depth );
	GLOUTPUT(( "SDL surface created. w=%d h=%d bpp=%d stencil=%d depthBits=%d\n", 
				surface->w, surface->h, surface->format->BitsPerPixel, stencil, depth ));

    /* Verify there is a surface */
    if ( !surface ) {
	    fprintf( stderr,  "Video mode set failed: %s\n", SDL_GetError( ) );
	    exit( 1 );
	}

	int r = glewInit();
	GLASSERT( r == GL_NO_ERROR );
	wglSwapIntervalEXT( 1 );	// vsync

	const unsigned char* vendor   = glGetString( GL_VENDOR );
	const unsigned char* renderer = glGetString( GL_RENDERER );
	const unsigned char* version  = glGetString( GL_VERSION );

	#if defined ( _WIN32 )
		const char* system = "Windows";
	#elif defined ( _LINUX )
		const char* system = "Linux";
	#elif defined ( __APPLE__ )
		const char* system = "Apple OS X";
	#else
		const char* system = "??";
	#endif

	GLLOG(( "OpenGL %s: Vendor: '%s'  Renderer: '%s'  Version: '%s'\n", system, vendor, renderer, version ));

	// Set the viewport to be the entire window
    glViewport(0, 0, surface->w, surface->h );
	//#ifdef FRAMEBUFFER_ROTATE
	//FrameBuffer* frameBuffer = new FrameBuffer( IPOD_SCREEN_WIDTH, IPOD_SCREEN_HEIGHT );
	//#endif
	
	bool done = false;
	bool dragging = false;
	bool zooming = false;
    SDL_Event event;

	float yRotation = 45.0f;
	grinliz::Vector2I mouseDown = { 0, 0 };
	grinliz::Vector2I prevMouseDown = { 0, 0 };
	U32 prevMouseDownTime = 0;

	void* game = NewGame( IPOD_SCREEN_HEIGHT, IPOD_SCREEN_WIDTH, rotation, ".\\" );

	SDL_TimerID timerID = SDL_AddTimer( 30, TimerCallback, 0 );

	// ---- Main Loop --- //
	while ( !done && SDL_WaitEvent( &event ) )
	{
		switch( event.type )
		{
			case SDL_KEYDOWN:
			{
				switch ( event.key.keysym.sym )
				{
					case SDLK_ESCAPE:
						done = true;
						break;

					case SDLK_KP_PLUS:
					case SDLK_KP_MINUS:
						{
							float zoom;
							GameCameraGet( game, GAME_CAMERA_ZOOM, &zoom );
							zoom += event.key.keysym.sym == SDLK_KP_PLUS ? 0.1f : -0.1f;
							GameCameraSet( game, GAME_CAMERA_ZOOM, zoom );
						}
						break;

					//case SDLK_PAGEDOWN:		GameTiltCamera( game, 2.0f );				break;
					//case SDLK_PAGEUP:		GameTiltCamera( game, -2.0f );				break;
					//case SDLK_UP:			GameMoveCamera( game, 0.0f, 1.0f, 0.0f);	break;
					//case SDLK_DOWN:			GameMoveCamera( game, 0.0f, -1.0f, 0.0f);	break;
					//case SDLK_RIGHT:		GameAdjustPerspective( game, 2.0f );		break;
					//case SDLK_LEFT:			GameAdjustPerspective( game, -2.0f );		break;
					//case SDLK_RIGHT:		yRotation += 2.0f; GameYRotateCamera( game, yRotation );		break;
					//case SDLK_LEFT:			yRotation -= 2.0f; GameYRotateCamera( game, yRotation );		break;
					//case SDLK_s:			GameShadowMode( game );						break;

#if !defined( MAPMAKER )
					case SDLK_p:
						{
						//	#ifdef FRAMEBUFFER_ROTATE
						//		frameBuffer->Bind();
						//	#endif
							ScreenCapture( "cap" );
						//	#ifdef FRAMEBUFFER_ROTATE
						//		frameBuffer->Bind();
						//	#endif
						}
						break;
#endif

#if defined( MAPMAKER )
					case SDLK_s:			
						{
							FILE* fp = fopen( filename.c_str(), "wb" );
							//((Game*)game)->SaveMap( fp );
							fclose( fp );
							GLOUTPUT(( "Save\n" ));
						}
						break;
					case SDLK_l:
						{
							FILE* fp = fopen( filename.c_str(), "rb" );
							if ( fp ) {
								//((Game*)game)->LoadMap( fp );
							}
							fclose( fp );
							GLOUTPUT(( "Load\n" ));
						}
						break;
					case SDLK_DELETE:	
						((Game*)game)->DeleteAtSelection(); 
						break;

					case SDLK_KP9:			
						((Game*)game)->RotateSelection( -1 );			
						break;
					case SDLK_r:
					case SDLK_KP7:			
						((Game*)game)->RotateSelection( 1 );			
						break;

					case SDLK_UP:			
					case SDLK_KP8:			
						((Game*)game)->DeltaCurrentMapItem(16);			
						break;
					case SDLK_KP5:			
					case SDLK_DOWN:			
						((Game*)game)->DeltaCurrentMapItem(-16);		
						break;
					case SDLK_KP6:			
					case SDLK_RIGHT:		
						((Game*)game)->DeltaCurrentMapItem(1); 			
						break;
					case SDLK_KP4:			
					case SDLK_LEFT:			
						((Game*)game)->DeltaCurrentMapItem(-1);			
						break;

					case SDLK_p:
						((Game*)game)->ShowPathing( !((Game*)game)->IsShowingPathing() );
						break;
#else
//						case SDLK_RIGHT:		GameRotate( game, --rotation );				break;
//						case SDLK_LEFT:			GameRotate( game, ++rotation );				break;
#endif

					default:
						break;
				}
/*					GLOUTPUT(( "fov=%.1f rot=%.1f h=%.1f\n", 
							game->engine.fov, 
							game->engine.camera.Tilt(), 
							game->engine.camera.PosWC().y ));
*/
			}
			break;

			case SDL_MOUSEBUTTONDOWN:
			{
				int x, y;
				TransformXY( event.button.x, event.button.y, &x, &y );

				mouseDown.Set( event.button.x, event.button.y );

				if ( event.button.button == 1 ) {
					GameDrag( game, GAME_DRAG_START, x, y );
					dragging = true;
				}
				else if ( event.button.button == 3 ) {
					zooming = true;
					GameZoom( game, GAME_ZOOM_START, -event.button.y );
					GameCameraRotate( game, GAME_ROTATE_START, 0.0f );
				}
			}
			break;

			case SDL_MOUSEBUTTONUP:
			{
				int x, y;
				TransformXY( event.button.x, event.button.y, &x, &y );

				if ( dragging ) {
					if ( event.button.button == 1 ) {
						GameDrag( game, GAME_DRAG_END, x, y );
						dragging = false;
					}
				}
				if ( event.button.button == 3 ) {
					zooming = false;
				}
				if ( event.button.button == 1 ) {
					if (    abs( mouseDown.x - event.button.x ) < 3 
						 && abs( mouseDown.y - event.button.y ) < 3 ) 
					{
						Uint8 *keystate = SDL_GetKeyState(NULL);
						int tap = 1;
						if ( SDL_GetModState() & (KMOD_LSHIFT|KMOD_RSHIFT) ) {
							tap = 2;
						}
						if (	abs( mouseDown.x - prevMouseDown.x ) < 3
							 && abs( mouseDown.y - prevMouseDown.y ) < 3
							 && ( SDL_GetTicks() - prevMouseDownTime ) < 300 ) {
								 tap =2 ;
						}
						GameTap( game, tap, x, y );

						prevMouseDown = mouseDown;
						prevMouseDownTime = SDL_GetTicks();
					}
				}
			}
			break;

			case SDL_MOUSEMOTION:
			{
				int state = SDL_GetMouseState(NULL, NULL);
				int x, y;
				TransformXY( event.button.x, event.button.y, &x, &y );
				//GLOUTPUT(( "move: ipod=%d,%d\n", x, y ));

				if ( dragging && ( state & SDL_BUTTON(1) ) )
				{
					if ( event.button.button == 1 ) {
						GameDrag( game, GAME_DRAG_MOVE, x, y );
					}
				}
				else if ( zooming && (state & SDL_BUTTON(3)) ) {

					GameZoom( game, GAME_ZOOM_MOVE, -event.button.y );
					
					//GLOUTPUT(( "x,y=%d,%d  down=%d,%d\n", x, y, mouseDown.x, mouseDown.y ));
					GameCameraRotate( game, GAME_ROTATE_MOVE, (float)(event.button.x-mouseDown.x)*0.5f );
				}
#if defined(MAPMAKER) || defined(_MSC_VER)
				else {
					((Game*)game)->MouseMove( x, y );
				}
#endif
			}
			break;

			case SDL_QUIT:
			{
				done = true;
			}
			break;

			case SDL_USEREVENT:
			{
				glEnable( GL_DEPTH_TEST );
				glDepthFunc( GL_LEQUAL );

				GameDoTick( game, SDL_GetTicks() );

				SDL_GL_SwapBuffers();
			};

			default:
				break;
		}
	}
	SDL_RemoveTimer( timerID );
	DeleteGame( game );

	SDL_Quit();

	MemLeakCheck();
	return 0;
}



void ScreenCapture( const char* baseFilename )
{
	int viewPort[4];
	glGetIntegerv(GL_VIEWPORT, viewPort);
	int width  = viewPort[2]-viewPort[0];
	int height = viewPort[3]-viewPort[1];

	SDL_Surface* surface = SDL_CreateRGBSurface( SDL_SWSURFACE, width, height, 
												 32, 0xff, 0xff<<8, 0xff<<16, 0xff<<24 );
	if ( !surface )
		return;

	glReadPixels( 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels );

	// This is a fancy swap, for the screen pixels:
	int i;
	U32* buffer = new U32[width];

	for( i=0; i<height/2; ++i )
	{
		memcpy( buffer, 
				( (U32*)surface->pixels + i*width ), 
				width*4 );
		memcpy( ( (U32*)surface->pixels + i*width ), 
				( (U32*)surface->pixels + (height-1-i)*width ),
				width*4 );
		memcpy( ( (U32*)surface->pixels + (height-1-i)*width ),
				buffer,
				width*4 );
	}
	delete [] buffer;

	// And now, set all the alphas to opaque:
	for( i=0; i<width*height; ++i )
		*( (U32*)surface->pixels + i ) |= 0xff000000;

	int index = 0;
	char buf[ 256 ];
	for( index = 0; index<100; ++index )
	{
		sprintf( buf, "%s%02d.bmp", baseFilename, index );
		FILE* fp = fopen( buf, "rb" );
		if ( fp )
			fclose( fp );
		else
			break;
	}
	if ( index < 100 )
		SDL_SaveBMP( surface, buf );
	SDL_FreeSurface( surface );
}

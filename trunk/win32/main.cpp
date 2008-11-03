#pragma warning ( disable : 4530 )		// Don't warn about unused exceptions.

#include "glew.h"
#include "SDL.h"

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../game/cgame.h"
/*
#include "../grinliz/glvector.h"
#include "../grinliz/glmatrix.h"
#include "../engine/engine.h"
#include "../engine/text.h"
#include "../engine/surface.h"
#include "../game/game.h"
*/

//using namespace grinliz;

int multisample = 4;
bool fullscreen = false;

int main( int argc, char **argv )
{    
	MemStartCheck();
	{ char* test = new char[16]; delete [] test; }

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
	SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 0 );
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

	surface = SDL_SetVideoMode( SCREEN_WIDTH, SCREEN_HEIGHT, 32, videoFlags );
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

	glewInit();

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
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	//engine->SetPerspective( NEAR_PLANE, FAR_PLANE, FOV );
	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_LEQUAL );

	bool done = false;
	bool dragging = false;
    SDL_Event event;

	//Game* game = new Game();
	void* game = NewGame();

	// ---- Main Loop --- //
	while ( !done )
	{
		// Clean out all the SDL events waiting for processing.
		while ( SDL_PollEvent( &event ) )
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

						case SDLK_PAGEDOWN:		GameTiltCamera( game, 2.0f );				break;
						case SDLK_PAGEUP:		GameTiltCamera( game, -2.0f );				break;
						case SDLK_UP:			GameMoveCamera( game, 0.0f, 1.0f, 0.0f);	break;
						case SDLK_DOWN:			GameMoveCamera( game, 0.0f, -1.0f, 0.0f);	break;
						case SDLK_RIGHT:		GameAdjustPerspective( game, 2.0f );		break;
						case SDLK_LEFT:			GameAdjustPerspective( game, -2.0f );		break;
/*
						case SDLK_PAGEDOWN:		game->engine.camera.DeltaTilt( 2.0f );				break;
						case SDLK_PAGEUP:		game->engine.camera.DeltaTilt( -2.0f );				break;
						case SDLK_UP:			game->engine.camera.DeltaPosWC( 0.0f, 1.0f, 0.0f);	break;
						case SDLK_DOWN:			game->engine.camera.DeltaPosWC( 0.0f, -1.0f, 0.0f);	break;

						case SDLK_RIGHT:
							game->engine.fov += 2.0f;
							game->engine.SetPerspective();
							break;

						case SDLK_LEFT:
							game->engine.fov -= 2.0f;
							game->engine.SetPerspective();
							break;
*/
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
					//game->engine.DragStart( event.button.x, HEIGHT-1-event.button.y );
					GameDragStart( game, event.button.x, SCREEN_HEIGHT-1-event.button.y );
					dragging = true;
				}
				break;

				case SDL_MOUSEBUTTONUP:
				{
					if ( dragging ) {
						GameDragEnd( game, event.button.x, SCREEN_HEIGHT-1-event.button.y );
						dragging = false;
					}
				}
				break;

				case SDL_MOUSEMOTION:
				{
					if ( dragging && event.motion.state == SDL_PRESSED ) {
						GameDragMove( game, event.motion.x, SCREEN_HEIGHT-1-event.motion.y );
					}
				}
				break;

				case SDL_QUIT:
				{
					done = true;
				}
				break;

				default:
					break;
			}
		}
		//game->DoTick( SDL_GetTicks() );
		GameDoTick( game, SDL_GetTicks() );
		SDL_GL_SwapBuffers();
	}
	//delete game;
	DeleteGame( game );
	SDL_Quit();

	MemLeakCheck();
	return 0;
}

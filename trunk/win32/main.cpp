#pragma warning ( disable : 4530 )		// Don't warn about unused exceptions.

#include "glew.h"
#include "SDL.h"

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glmatrix.h"
#include "../engine/engine.h"
#include "../engine/text.h"
#include "../engine/surface.h"

using namespace grinliz;

int multisample = 4;
bool fullscreen = false;
int WIDTH  = 480;
int HEIGHT = 320;
int GLOW_SOURCE = 1024;
int GLOW_TARGET = 256;
float FOV = 45.0f;
float ASPECT = (float)(WIDTH) / (float)(HEIGHT);


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

	surface = SDL_SetVideoMode( WIDTH, HEIGHT, 32, videoFlags );
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
    glViewport(0, 0, WIDTH, HEIGHT);

	//engine->SetPerspective( NEAR_PLANE, FAR_PLANE, FOV );
	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_LEQUAL );

	U32 startTime = SDL_GetTicks();
	U32 markFrame = startTime;
	U32 frameCountSinceMark = 0;
	float framesPerSecond = 0.0f;

	bool done = false;
    SDL_Event event;

	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	glDisable( GL_DEPTH_TEST );

	Surface* s = new Surface();

	FILE* fp = fopen( "./res/green.tex", "rb" );
	U32 textureID = s->LoadTexture( fp );
	fclose( fp );

	fp = fopen( "./res/stdfont.tex", "rb" );
	U32 textTextureID = s->LoadTexture( fp );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	UFOInitDrawText( textTextureID );
	fclose( fp );

	EngineData engineData;
	Engine* engine = new Engine( WIDTH, HEIGHT, engineData );
	//float fov = 30.0f;
	//engine->SetPerspective( 1.f, 100.f, fov );

	//Vector3F mouseYPlane = { 0.f, 0.f, 0.f };

	// ---- Main Loop --- //
	while ( !done )
	{
		// Clean out all the SDL events waiting for processing.
		while ( SDL_PollEvent( &event ) )
		//SDL_WaitEvent( &event );
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

						case SDLK_PAGEDOWN:		engine->camera.DeltaTilt( 2.0f );				break;
						case SDLK_PAGEUP:		engine->camera.DeltaTilt( -2.0f );				break;
						case SDLK_UP:			engine->camera.DeltaPosWC( 0.0f, 1.0f, 0.0f);	break;
						case SDLK_DOWN:			engine->camera.DeltaPosWC( 0.0f, -1.0f, 0.0f);	break;

						case SDLK_RIGHT:
							engine->fov += 2.0f;
							engine->SetPerspective();
							break;

						case SDLK_LEFT:
							engine->fov -= 2.0f;
							engine->SetPerspective();
							break;

						default:
							break;
					}
					GLOUTPUT(( "fov=%.1f rot=%.1f h=%.1f\n", engine->fov, engine->camera.Tilt(), engine->camera.PosWC().y ));
				}
				break;

				case SDL_MOUSEBUTTONDOWN:
				{
					engine->DragStart( event.button.x, HEIGHT-1-event.button.y );
				}
				break;

				case SDL_MOUSEBUTTONUP:
				{
					if ( engine->IsDragging() ) {
						engine->DragEnd( event.button.x, HEIGHT-1-event.button.y );
					}
				}
				break;

				case SDL_MOUSEMOTION:
				{
					//engine->RayFromScreenToYPlane( event.motion.x, HEIGHT-1-event.motion.y, &mouseYPlane );
					//GLOUTPUT(( "world (%.1f, %.1f, %.1f)\n", mouseYPlane.x, mouseYPlane.y, mouseYPlane.z ));
					if ( engine->IsDragging() && event.motion.state == SDL_PRESSED ) {
						engine->DragMove( event.motion.x, HEIGHT-1-event.motion.y );
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

		// And check the key downs:
 		const Uint8 *keystate = SDL_GetKeyState( NULL );

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBindTexture( GL_TEXTURE_2D, textureID );
		engine->Draw();

		/*{
			const float D=0.1f;
			float v[12] = {	mouseYPlane.x-D, 0.001f, mouseYPlane.z-D, 
							mouseYPlane.x+D, 0.001f, mouseYPlane.z-D, 
							mouseYPlane.x+D, 0.001f, mouseYPlane.z+D, 
							mouseYPlane.x-D, 0.001f, mouseYPlane.z+D };

			glColor3f( 1.f, 1.f, 1.f );
			glBindTexture( GL_TEXTURE_2D, 0 );
			glVertexPointer( 3, GL_FLOAT, 0, v );
			glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
		}*/
		++frameCountSinceMark;
		U32 currentTime = SDL_GetTicks();

		if ( currentTime - markFrame > 500 ) {
			framesPerSecond = 1000.0f*(float)(frameCountSinceMark) / ((float)(currentTime - markFrame));
			markFrame = currentTime;
			frameCountSinceMark = 0;
		}

		UFODrawText( 0, 0, "UFO Attack! %.1ffps", framesPerSecond );
		SDL_GL_SwapBuffers();
	}
	delete engine;
	delete s;
	glDeleteTextures( 1, &textureID );
	glDeleteTextures( 1, &textTextureID );
	SDL_Quit();

	MemLeakCheck();
	return 0;
}

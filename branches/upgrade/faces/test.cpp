#include <stdlib.h>
#include <string>
#include <time.h>
#include <algorithm>
#include "SDL.h"
//#include "SDL/SDL_image.h"
#include "faces.h"

typedef SDL_Surface* (SDLCALL * PFN_IMG_LOAD) (const char *file);
PFN_IMG_LOAD IMG_Load;

U32 GetPixel( const SDL_Surface *surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    U8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        return *p;

    case 2:
        return *(Uint16 *)p;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;

    case 4:
        return *(U32 *)p;

    default:
        return 0;       /* shouldn't happen, but avoids warnings */
    }
}


void load_image( const char* name, Surface* surface )
{
	if (IMG_Load == 0 ) {
		void* handle = SDL_LoadObject( "SDL_image" );
		IMG_Load = (PFN_IMG_LOAD) SDL_LoadFunction( handle, "IMG_Load" );
	}

  // load image file into image
  SDL_Surface *image = IMG_Load(name);
  GLASSERT( image );
  surface->Set( Surface::RGBA16, image->w, image->h );

  grinliz::Color4U8 rgba;

	// Pull the pixels out, put in surface.
	for( int y=0; y<image->h; ++y ) {
		for( int x=0; x<image->w; ++x ) {
			U32 c = GetPixel( image, x, y );
			SDL_GetRGBA( c, image->format, &rgba.r, &rgba.g, &rgba.b, &rgba.a );
			surface->SetImg16( x, y, Surface::CalcRGBA16( rgba ) );
		}
	}
	SDL_FreeSurface( image );
}


int main(int argc, char* argv[]) {
  SDL_Init( SDL_INIT_EVERYTHING );
  srand ( (unsigned)time(NULL) );
  
  //Set up screen
  SDL_Surface *screen = NULL;
  screen = SDL_SetVideoMode( 320, 320, 32, SDL_SWSURFACE );

	FaceGenerator faceGen;
	load_image( "face_parts/chins_64w.png", &faceGen.chins );
	faceGen.nChins = 17;
	load_image( "face_parts/mouths_64.png", &faceGen.mouths );
	faceGen.nMouths = 14;
	load_image( "face_parts/noses_64.png", &faceGen.noses );
	faceGen.nNoses = 5;
	load_image( "face_parts/hairs_64b.png", &faceGen.hairs );
	faceGen.nHairs = 17;
	load_image( "face_parts/eyes_64w2.png", &faceGen.eyes);
	faceGen.nEyes = 15;
	load_image( "face_parts/glasses_64.png", &faceGen.glasses);
	faceGen.nGlasses = 5;

	static const int NCOLOR=7;
  Uint32 skin_colors[NCOLOR] = {
    0xe4cc8c,
    0xd5b589,
    0xEDDEC2,
    0xd5b589,
    0xa46c3c,
    0xcd9668,
    0xcca48c,
  };

  Uint32 hair_colors[NCOLOR] = {
    0x52482a,
    0x36300b,
    0x586481,
    0x811515,
    0x8b8554,
    0x586481,
    0xa0882c
  };

  SDL_Rect rect = { 0, 0, 64, 64 };
  Surface faceSurface;
  faceSurface.Set( Surface::RGBA16, 64, 64 );
  FaceGenerator::FaceParam param;

  SDL_Surface* face_image = SDL_CreateRGBSurface( SDL_SWSURFACE, 64, 64, 16, 0xf000, 0x0f00, 0x00f0, 0x000f ); 

  for (int i=0; i<25; i++) {
    rect.x = (i%5)*64;
    rect.y = (i/5)*64;

	param.Generate( i&1, rand() );

	Uint32 c = skin_colors[i%NCOLOR];
	param.skinColor.Set( c>>16, (c>>8)&0xff, c&0xff, 255 );
	c = hair_colors[rand()%NCOLOR];
	param.hairColor.Set( c>>16, (c>>8)&0xff, c&0xff, 255 );
	param.glassesColor.Set( 100, 100, 200, 255 );

	faceGen.GenerateFace( param, &faceSurface );
	memcpy( face_image->pixels, faceSurface.Pixels(), 2*64*64 );

    // show the result
	SDL_FillRect(screen, &rect, SDL_MapRGB( screen->format, rand()%255, rand()%255, rand()%255 ) );
    SDL_BlitSurface( face_image, NULL, screen, &rect );
  }
  SDL_SaveBMP( screen, "cap.bmp" );
  SDL_Flip( screen );
  
  SDL_Event event;
  int running = 1;

  while(SDL_WaitEvent(&event)) {
    if(event.type == SDL_QUIT) {
      break;
    }
  }

  SDL_FreeSurface( face_image );
  SDL_Quit();
  return 0;
}

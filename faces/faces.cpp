#include "faces.h"
#include "../grinliz/glrandom.h"
#include "../grinliz/glutil.h"

using namespace grinliz;

// g++ -o faces faces.cpp -lSDL -lSDL_image
void FaceGenerator::ChangeColor( Surface* surface, U16 src, U16 dst ) 
{
	const int n = surface->Width() * surface->Height();
	U16* p = (U16*) surface->Pixels();

	for( int i=0; i<n; ++i, ++p ) {
		if ( *p == src ) *p = dst;
	}
}


Surface::RGBA FaceGenerator::CalcShadowColor( Surface::RGBA in, float factor )
{
	Surface::RGBA c = { U8(in.r*factor), U8(in.g*factor), U8(in.b*factor), 255 };
	return c;
}


void FaceGenerator::Composite( const Surface& srcSurface, const Rect2I& srcRect, 
							   Surface* dstSurface, const Rect2I& dstRect )
{
	GLASSERT( srcSurface.Format()  == Surface::RGBA16 );
	GLASSERT( dstSurface->Format() == Surface::RGBA16 );

	for( int y=0; y<dstRect.h; ++y ) {
		for( int x=0; x<dstRect.w; ++x ) {
			U16 color16 = srcSurface.GetTex16( x+srcRect.x, y+srcRect.y );
			Surface::RGBA color = Surface::CalcRGBA16( color16 );
			GLASSERT( color.a == 0 || color.a == 255 );
			if ( color.a > 0 ) {
				dstSurface->SetTex16( x+dstRect.x, y+dstRect.y, color16 );
			}
		}
	}
}


void FaceGenerator::GenerateFace( const FaceParam& param, Surface* image )
{
	GLASSERT( image->Width() == 64 );
	GLASSERT( image->Height() == 64 );
	GLASSERT( image->Format() == Surface::RGBA16 );
	image->Clear( 0 );

	Rect2I srcRect, dstRect;

	// Chin
	srcRect.Set( 0, param.chin*(chins.Height()/nChins),
				 chins.Width(), chins.Height()/nChins );
	dstRect.Set( image->Width()/2 - srcRect.w/2, 16,
				  srcRect.w, srcRect.h );			 
	Composite( chins, srcRect, image, dstRect );

	// Nose
	srcRect.Set( 0, param.nose*(noses.Height()/nNoses),
				 noses.Width(), noses.Height()/nNoses );
	dstRect.Set( image->Width()/2 - srcRect.w/2, 34+param.noseOffset,
				 srcRect.w, srcRect.h );
	Composite( noses, srcRect, image, dstRect );

	// Mouth
	srcRect.Set( 0, param.mouth*(mouths.Height()/nMouths),
				 mouths.Width(), mouths.Height()/nMouths );
	dstRect.Set( image->Width()/2 - srcRect.w/2, 42+param.mouthOffset,
				 srcRect.w, srcRect.h );
	Composite( mouths, srcRect, image, dstRect );

	// Eyes
	srcRect.Set( 0, param.eyes*(eyes.Height()/nEyes),
				 eyes.Width(), eyes.Height()/nEyes );
	dstRect.Set( image->Width()/2 - srcRect.w/2, 22+param.eyeOffset,
				 srcRect.w, srcRect.h );
	Composite( eyes, srcRect, image, dstRect );

	// Glasses
	srcRect.Set( 0, param.glasses*(glasses.Height()/nGlasses),
				 glasses.Width(), glasses.Height()/nGlasses );
	dstRect.Set( image->Width()/2-srcRect.w/2, 28+param.glassesOffset,
				 srcRect.w, srcRect.h );
	Composite( glasses, srcRect, image, dstRect );

	// Hair
	srcRect.Set( 0, param.hair*(hairs.Height()/nHairs),
				 hairs.Width(), hairs.Height()/nHairs );
	dstRect.Set( image->Width()/2-srcRect.w/2, 0,
				 srcRect.w, srcRect.h );
	Composite( hairs, srcRect, image, dstRect );

	// Patch the colors.

	// skin
	ChangeColor( image, 
				 Surface::CalcRGBA16( 0, 255, 0, 255 ), 
				 Surface::CalcRGBA16( param.skinColor ) ); 

	// skin shadow mask
	ChangeColor( image, 
				 Surface::CalcRGBA16( 255, 0, 0, 255 ), 
				 Surface::CalcRGBA16( CalcShadowColor( param.skinColor, 0.66f )) ); 

	// hair 
	ChangeColor( image,
		Surface::CalcRGBA16( 241, 198, 96, 255 ),
		Surface::CalcRGBA16( param.hairColor ));

	ChangeColor( image,
		Surface::CalcRGBA16( 221, 136, 68, 255 ),
		Surface::CalcRGBA16( CalcShadowColor( param.hairColor, 0.66f )));

	// glasses
	ChangeColor( image,
		Surface::CalcRGBA16( 153, 0, 255, 255 ),
		Surface::CalcRGBA16( param.glassesColor ));

	ChangeColor( image,
		Surface::CalcRGBA16( 102, 0, 204, 255 ),
		Surface::CalcRGBA16( CalcShadowColor( param.glassesColor, 0.66f )));

	// beard
	ChangeColor( image,
		Surface::CalcRGBA16( 0, 0, 255, 255 ),
		Surface::CalcRGBA16( param.unshaven ? CalcShadowColor( param.skinColor, 0.85f ) : param.skinColor ));

}


void FaceGenerator::FaceParam::Generate( int gender, int seed )
{
	static const int male_chins[]	= {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
	static const int female_chins[]	= {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	static const int male_hairs[]	= {1, 2, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15, 16};
	static const int female_hairs[]	= {0, 2, 3, 4, 6, 7, 11, 12, 13, 14, 15};
	static const int male_eyes[]	= {0, 1, 4, 8, 9, 10, 11, 12, 13, 14};
	static const int female_eyes[]	= {0, 2, 3, 4, 5, 6, 7, 8};
	static const int male_noses[]	= {0, 1, 2, 3};
	static const int female_noses[]	= {1, 2, 3, 4};
	static const int male_mouths[]	= {0,1,2,3,6,8,9,10,11,12,13};
	static const int female_mouths[]= {0,3,4,5,6,7,8,9,12,13};

	grinliz::Random random( seed );

	if ( gender == 0 ) {
		chin = male_chins[ random.Rand( (sizeof(male_chins)/sizeof(int) )) ];
		unshaven = random.Boolean();

		eyes = male_eyes[ random.Rand( (sizeof(male_eyes)/sizeof(int) )) ];
		eyeOffset = random.Rand( 2 );
		eyesFlip = random.Boolean();

		mouth = male_mouths[ random.Rand(( sizeof(male_mouths)/sizeof(int) )) ];
		mouthOffset = random.Rand( 2 );
		mouthFlip = false;

		nose = male_noses[ random.Rand((sizeof(male_noses)/sizeof(int) )) ];
		noseOffset = random.Rand( 3 );

		hair = male_hairs[ random.Rand((sizeof(male_hairs)/sizeof(int) )) ];
		hairFlip = random.Boolean();

		glasses = grinliz::Max( 0, int(random.Rand(15)-10) );
		glassesOffset = 0;
	}
	else {
		chin = female_chins[ random.Rand( (sizeof(female_chins)/sizeof(int) )) ];
		unshaven = false;

		eyes = female_eyes[ random.Rand( (sizeof(female_eyes)/sizeof(int) )) ];
		eyeOffset = random.Rand( 3 ) + 2;
		eyesFlip = random.Boolean();

		mouth = female_mouths[ random.Rand(( sizeof(female_mouths)/sizeof(int) )) ];
		mouthOffset = random.Rand( 2 );
		mouthFlip = false;

		nose = female_noses[ random.Rand((sizeof(female_noses)/sizeof(int) )) ];
		noseOffset = random.Rand( 2 )+1;

		hair = female_hairs[ random.Rand((sizeof(female_hairs)/sizeof(int) )) ];
		hairFlip = random.Boolean();

		glasses = grinliz::Max( 0, int(random.Rand(10)-5) );
		glassesOffset = random.Rand( 3 );
	}
}


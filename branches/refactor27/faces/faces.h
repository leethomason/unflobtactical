#ifndef FACE_GENERATOR_INCLUDED
#define FACE_GENERATOR_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glcolor.h"

#include "../engine/surface.h"

class FaceGenerator
{
public:
	enum { SIZE = 64, BPP = 2 };

	FaceGenerator()		{}
	~FaceGenerator()	{}
	
	Surface chins;		int nChins;
	Surface mouths;		int nMouths;
	Surface noses;		int nNoses;
	Surface hairs;		int nHairs;
	Surface eyes;		int nEyes;
	Surface glasses;	int nGlasses;

	struct FaceParam {
		FaceParam() : chin( 0 ),  unshaven( false ),
					  eyes( 0 ),  eyeOffset( 0 ),   eyesFlip( false ),
					  mouth( 0 ), mouthOffset( 0 ), mouthFlip( false ),
					  nose( 0 ),  noseOffset( 0 ), 
					  hair( 0 ),  hairFlip( false )
		{
			skinColor.Set( 255, 0, 0, 255 );
			hairColor.Set( 0, 255, 0, 255 );
			glassesColor.Set( 0, 0, 255, 255 );
		}


		void Generate( int gender, int seed );

		int chin;
		bool unshaven;
		
		int eyes;
		int eyeOffset;
		bool eyesFlip;

		int mouth;
		int mouthOffset;
		bool mouthFlip;

		int nose;
		int noseOffset;

		int hair;
		bool hairFlip;

		int glasses;
		int glassesOffset;

		grinliz::Color4U8 skinColor;
		grinliz::Color4U8 hairColor;
		grinliz::Color4U8 glassesColor;
	};

	void GenerateFace( const FaceParam& param, Surface* surface );

private:
	void ChangeColor( Surface* surface, const U16* src, const U16* dst, int count );
	grinliz::Color4U8 CalcShadowColor( grinliz::Color4U8, float ratio );
	void Composite( const Surface& srcSurface, const grinliz::Rect2I& srcRect, 
					Surface* dstSurface, const grinliz::Rect2I& dstRect, bool flip=false );

};

#endif // FACE_GENERATOR_INCLUDED
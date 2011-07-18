#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glcolor.h"

#include "../engine/surface.h"

class FaceGenerator
{
public:
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
		{}

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

		Surface::RGBA skinColor;
		Surface::RGBA hairColor;
		Surface::RGBA glassesColor;
	};

	void GenerateFace( const FaceParam& param, Surface* surface );

private:
	void ChangeColor( Surface* surface, U16 src, U16 dst );
	Surface::RGBA CalcShadowColor( Surface::RGBA, float ratio );
	void Composite( const Surface& srcSurface, const grinliz::Rect2I& srcRect, 
					Surface* dstSurface, const grinliz::Rect2I& dstRect );

};

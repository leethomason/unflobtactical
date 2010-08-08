#ifndef UFOATTACK_STATE_MANAGER_INCLUDED
#define UFOATTACK_STATE_MANAGER_INCLUDED

#include <stdint.h>
#include "../grinliz/gldebug.h"
#include "vertex.h"

class Texture;
namespace grinliz {
	class Matrix4;
};

class GPUShader 
{
public:
	virtual ~GPUShader();


	enum MatrixType {
		TEXTURE_MATRIX,
		MODELVIEW_MATRIX,
		PROJECTION_MATRIX
	};

	static void ResetState();
	static void Clear();
	static void SetMatrixMode( MatrixType m );

	void SetVertex( int components, int stride, const void* ptr ) {
		vertexComponents = components;
		vertexStride = stride;
		vertexPtr = ptr;
	}

	void SetNormal( int stride, const void* ptr ) {
		normalStride = stride;
		normalPtr = ptr;
	}

	void SetColorArray( int components, int stride, const void* ptr ) {
		colorComponents = components;
		colorStride = stride;
		colorPtr = ptr;
	}

	void SetTexture0( Texture* tex ) {
		texture0 = tex;
	}

	void SetTexture0( Texture* tex, int components, int stride, const void* ptr ) {
		texture0 = tex;
		texture0Components = components;
		texture0Stride = stride;
		texture0Ptr = ptr;
	}

	void SetTexture0( int components, int stride, const void* ptr ) {
		GLASSERT( texture0 );
		texture0Components = components;
		texture0Stride = stride;
		texture0Ptr = ptr;
	}

	void SetTexture1( Texture* tex, int components, int stride, const void* ptr ) {
		texture1 = tex;
		texture1Components = components;
		texture1Stride = stride;
		texture1Ptr = ptr;
	}

	void SetColor( float r, float g, float b )				{ color.x = r; color.y = g; color.z = b; color.w = 1; }
	void SetColor( float r, float g, float b, float a )		{ color.x = r; color.y = g; color.z = b; color.w = a; }
	void SetColor( const Color4F& c )						{ color = c; }

	void PushMatrix( MatrixType type );
	void MultMatrix( MatrixType type, const grinliz::Matrix4& m );
	void PopMatrix( MatrixType type );

	void Draw( int index, const uint16_t* elements );
	void Draw( int index, const int* elements );

	int SortOrder()	const { 
		if ( blend ) return 2;
		if ( alphaTest ) return 1;
		return 0;
	}

	static void ResetTriCount()	{ trianglesDrawn = 0; drawCalls = 0; }
	static int TrianglesDrawn() { return trianglesDrawn; }
	static int DrawCalls()		{ return drawCalls; }

protected:

	GPUShader() : texture0( 0 ), texture1( 0 ), 
				 vertexPtr( 0 ), vertexStride( 0 ), vertexComponents( 3 ),
				 normalPtr( 0 ), normalStride( 0 ),
				 colorPtr( 0 ), colorStride( 0 ), colorComponents( 3 ),
				 texture0Ptr( 0 ), texture0Stride( 0 ), texture0Components( 2 ),
				 texture1Ptr( 0 ), texture1Stride( 0 ), texture1Components( 2 ),
				 blend( false ), alphaTest( 0 ), lighting( false ), lightParams( 0 ),
				 depthWrite( true ), depthTest( true )
	{
		color.Set( 1, 1, 1, 1 );
		matrixDepth[0] = matrixDepth[1] = matrixDepth[2] = 0;
	}

	static void SetState( const GPUShader& );

private:
	void SwitchMatrixMode( MatrixType type );	
	static GPUShader		current;
	static MatrixType		matrixMode;		// Note this is static and global!

protected:
	static int trianglesDrawn;
	static int drawCalls;
	static uint32_t uid;

	//virtual void SetLightParams() const	{}

	Texture* texture0;
	Texture* texture1;

	const void* vertexPtr;
	int			vertexStride;
	int			vertexComponents;

	const void* normalPtr;
	int			normalStride;

	const void* colorPtr;
	int			colorStride;
	int			colorComponents;

	const void* texture0Ptr;
	int			texture0Stride;
	int			texture0Components;

	const void* texture1Ptr;
	int			texture1Stride;
	int			texture1Components;

	bool		blend;
	bool		alphaTest;
	bool		lighting;
	uint32_t	lightParams;

	bool		depthWrite;
	bool		depthTest;

	Color4F		color;
	int			matrixDepth[3];
};


class CompositingShader : public GPUShader
{
public:
	/** Writes texture or color and neither writes nor tests z. 
		Texture/Color support
			- no texture
			- texture0, modulated by color
			- texture0 and texture1 (light map compositing)
		Blend support
	*/
	CompositingShader( bool blend=false );
	void SetBlend( bool _blend )				{ this->blend = _blend; }
};


class LightShader : public GPUShader
{
public:
	/** Texture or color. Writes & tests z. Enables lighting. */
	LightShader( const Color4F& ambient, const grinliz::Vector4F& direction, const Color4F& diffuse, bool alphaTest );
	~LightShader();

protected:
	void SetLightParams() const;

	static int locked;

	Color4F				ambient;
	grinliz::Vector4F	direction;
	Color4F				diffuse;
};


class FlatShader : public GPUShader
{
public:
	FlatShader()	{}	// totally vanilla
};


class PointParticleShader : public GPUShader
{
public:
	static bool IsSupported();

	PointParticleShader();
	void DrawPoints( Texture* texture, float pointSize, int start, int count );
};


class QuadParticleShader : public GPUShader
{
public:
	QuadParticleShader();
};

#endif

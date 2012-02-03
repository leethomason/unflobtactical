#ifndef XENOENGINE_SHADERMANAGER_INCLUDED
#define XENOENGINE_SHADERMANAGER_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glcolor.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glmatrix.h"
#include "ufoutil.h"

#include "enginelimits.h"

class Texture;
class Stream;

#if XENOENGINE_OPENGL == 2
class ShaderManager
{
public:
	ShaderManager();
	~ShaderManager();

	static ShaderManager* Instance() { if ( !instance ) instance = new ShaderManager(); return instance; }

	enum {
		TEXTURE0			= (1<<0),		// Texture is in use. Note that the sampling state (linear, nearest) is saved with the texture.
		TEXTURE0_ALPHA_ONLY = (1<<1),		// Texture is only alpha, which composites differently.
		TEXTURE0_TRANSFORM	= (1<<2),		// Texture has a texture transform.
		TEXTURE1			= (1<<3),
		TEXTURE1_ALPHA_ONLY	= (1<<4),
		TEXTURE1_TRANSFORM	= (1<<5),
		COLORS				= (1<<6),		// Per-vertex colors.
		COLOR_MULTIPLIER	= (1<<7),		// Global color multiplier.
		LIGHTING_DIFFUSE	= (1<<8)		// Diffuse lighting. Requires per vertex normals.
	};

	void DeviceLoss();
	void ActivateShader( int flags );

	// All
	void SetTransforms( const grinliz::Matrix4& mvp, const grinliz::Matrix4& normal );

	enum {
		A_TEXTURE0,
		A_TEXTURE1,
		A_POS,
		A_NORMAL,
		A_COLOR
	};
	void ClearStream();
	void SetStreamData( int id, int count, int type, int stride, const void* data );	 

	// Texture units.
	void SetTexture( int index, Texture* texture );
	void SetTextureTransform( int index, const grinliz::Matrix4& mat );

	// Color & Lights
	void SetColorMultiplier( const grinliz::Color4F& color );
	void SetDiffuse( const grinliz::Vector4F& dir, const grinliz::Vector4F& ambient, const grinliz::Vector4F& diffuse );

private:
	static ShaderManager* instance;

	enum { MAX_SHADERS = 10 };	// or I need to go in there and create proper lookups and such
								// actually, this only needs to be a reasonable at steady state
	struct Shader {
		Shader::Shader() : flags(0), vertexProg(0), fragmentProg(0), prog(0) {}

		int flags;
		U32 vertexProg;
		U32 fragmentProg;
		U32 prog;
	};

	Shader shaderArr[MAX_SHADERS];
	grinliz::GLString header;
	const Shader* active;
	CDynArray<int> activeStreams;

	const Shader* CreateProgram( int flag );
	void DeleteProgram( Shader* );

	void AppendFlag( grinliz::GLString* str, const char* flag, int set );
};
#endif


#endif //  XENOENGINE_SHADERMANAGER_INCLUDED
#ifndef XENOENGINE_SHADERMANAGER_INCLUDED
#define XENOENGINE_SHADERMANAGER_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glcolor.h"
#include "../grinliz/glstringutil.h"

#include "enginelimits.h"

class Texture;

#if XENOENGINE_OPENGL == 2
class ShaderManager
{
public:
	ShaderManager();
	~ShaderManager();

	enum {
		TEXTURE0			= 0x01,		// Texture0 is in use. Note that the sampling state (linear, nearest) is saved with the texture.
		TEXTURE0_TRANSFORM	= 0x02,		// Texture0 has a texture transform.
		TEXTURE1			= 0x04,
		TEXTURE1_TRANSFORM	= 0x08,
		COLORS				= 0x20,		// Per-vertex colors.
		COLOR_MULTIPLIER	= 0x40,		// Global color multiplier.
		LIGHTING_DIFFUSE	= 0x80		// Diffuse lighting. Requires per vertex normals.
	};

	void ActivateShader( int flags );

	// Texture units.
	void SetTexture0( const Texture* );
	void SetTexture1( const Texture* );

	// Diffuse lighting parameters.
	void SetDiffuse( const grinliz::Color4U8& ambient, const grinliz::Color4U8& diffuse );

private:
	enum { MAX_SHADERS = 10 };	// or I need to go in there and create proper lookups and such
								// actually, this only needs to be a reasonable at steady state
	struct Shader {
		int flags;
		U32 vertexProg;
		U32 fragmentProg;
		U32 prog;
	};
	Shader shaderArr[MAX_SHADERS];
	grinliz::GLString header;

	const Shader* CreateProgram( int flag );
	void DeleteProgram( Shader* );

	void AppendFlag( grinliz::GLString* str, const char* flag, int set );
};
#endif


#endif //  XENOENGINE_SHADERMANAGER_INCLUDED
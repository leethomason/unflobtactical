#ifndef XENOENGINE_SHADERMANAGER_INCLUDED
#define XENOENGINE_SHADERMANAGER_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h";
#include "../grinliz/glcolor.h"

#include "enginelimits.h"

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
		NORMALS				= 0x10,		// Per-vertex normals.
		COLORS				= 0x20,		// Per-vertex colors.
		COLOR_MULTIPLIER	= 0x40,		// Global color multiplier.
		LIGHTING_DIFFUSE	= 0x80		// Diffuse lighting.
	};

	void ActivateShader( int flags );

	// Texture units.
	void SetTexture0( const Texture* );
	void SetTexture1( const Texture* );

	// Diffuse lighting parameters.
	void SetDiffuse( const grinliz::Color4& ambient, const grinliz::Color4& diffuse );
};
#endif


#endif //  XENOENGINE_SHADERMANAGER_INCLUDED
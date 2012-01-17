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
		TEXTURE0			= 0x01,
		TEXTURE0_TRANSFORM	= 0x02,
		TEXTURE1			= 0x04,
		TEXTURE1_TRANSFORM	= 0x08,
		NORMALS				= 0x10,
		COLORS				= 0x20,
		COLOR_MULTIPLIER	] 0x40,
		LIGHTING_DIFFUSE	= 0x80
	};

	void ActivateShader( int flags );

	void SetTexture0( const Texture* );
	void SetTexture1( const Texture* );
	void SetDiffuse( const grinliz::Color4& ambient, const grinliz::Color4& diffuse );
};
#endif


#endif //  XENOENGINE_SHADERMANAGER_INCLUDED
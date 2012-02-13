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
		TEXTURE0_3COMP		= (1<<3),		// Uses 3 components for the texture (for feeding a travsform, usually.)

		TEXTURE1			= (1<<4),
		TEXTURE1_ALPHA_ONLY	= (1<<5),
		TEXTURE1_TRANSFORM	= (1<<6),
		TEXTURE1_3COMP		= (1<<7),
		
		COLORS				= (1<<8),		// Per-vertex colors.
		COLOR_MULTIPLIER	= (1<<9),		// Global color multiplier.
		LIGHTING_DIFFUSE	= (1<<10)		// Diffuse lighting. Requires per vertex normals.
	};

	void DeviceLoss();
	void ActivateShader( int flags );

	// Warning: must match gAttributeName
	enum {
		A_TEXTURE0,
		A_TEXTURE1,
		A_POS,
		A_NORMAL,
		A_COLOR,
		MAX_ATTRIBUTE
	};
	void ClearStream();
	void SetStreamData( int id, int count, int type, int stride, const void* data );	 

	// Warning: must match gUniformName
	enum {
		U_MVP_MAT,
		U_NORMAL_MAT,
		U_TEXTURE0_MAT,
		U_TEXTURE1_MAT,
		U_COLOR_MULT,
		U_LIGHT_DIR,
		U_AMBIENT,
		U_DIFFUSE,
		MAX_UNIFORM
	};

	void SetTexture( int index, Texture* texture );
	void SetUniform( int id, const grinliz::Matrix4& mat );
	void SetUniform( int id, const grinliz::Color4F& color ) {
		grinliz::Vector4F v = { color.r, color.g, color.b, color.a };
		SetUniform( id, v );
	}
	void SetUniform( int id, const grinliz::Vector4F& vector );
	void SetUniform( int id, const grinliz::Vector3F& vector );


private:
	static ShaderManager* instance;
	
	struct Shader {
		void Init() {
			flags = 0;
			vertexProg = fragmentProg = prog = 0;
			for( int i=0; i<MAX_ATTRIBUTE; ++i ) attributeLoc[i] = -1;
			for( int i=0; i<MAX_UNIFORM; ++i ) uniformLoc[i] = -1;
		}

		int flags;
		U32 vertexProg;
		U32 fragmentProg;
		U32 prog;

		int attributeLoc[MAX_ATTRIBUTE];
		int uniformLoc[MAX_UNIFORM];

		int GetAttributeLocation( int attribute );
		int GetUniformLocation( int uniform );
	};

	CDynArray< Shader > shaderArr;
	grinliz::GLString header;
	Shader* active;
	CDynArray<int> activeStreams;

	Shader* CreateProgram( int flag );
	void DeleteProgram( Shader* );

	void AppendFlag( grinliz::GLString* str, const char* flag, int set );
};
#endif


#endif //  XENOENGINE_SHADERMANAGER_INCLUDED
// TEXTURE0
// TEXTURE0_TRANSFORM
// TEXTURE1
// TEXTURE1_TRANSFORM
// COLORS
// COLOR_MULTIPLIER
// LIGHTING_DIFFUSE

uniform mat4 u_mvpMatrix;		// model-view-projection
//uniform mat4 u_mvMatrix;		// model-view

#if TEXTURE0_TRANSFORM == 1
uniform sampler2D texture0;
uniform mat4 u_texMat0;
#endif
#if TEXTURE1_TRANSFORM == 1
uniform sampler2D texture1;
uniform mat4 u_texMat1;
#endif

#if COLOR_MULTIPLIER == 1
uniform vec4 u_colorMult;		
#endif
#if LIGHTING_DIFFUSE == 1
uniform mat4 u_normalMatrix;	// normal transformation
uniform vec4 u_lightDir;		// light direction, eye space (x,y,z,0)
uniform vec4 u_ambient;			// ambient light. ambient+diffuse = 1
uniform vec4 u_diffuse;			// diffuse light
#endif

attribute vec3 a_pos;			// vertex position
#if TEXTURE0 == 1
attribute vec2 a_uv0;
#endif
#if TEXTURE1 == 1
attribute vec2 a_uv1;
#endif

#if COLORS == 1
attribute vec3 a_color;			// vertex color
#endif
#if LIGHTING_DIFFUSE > 0
attribute vec3 a_normal;		// vertex normal
#endif

vec4 v_color;					// color passed to fragment shader
#if TEXTURE0 == 1
varying vec2 v_uv0;
#endif
#if TEXTURE1 == 1
varying vec v_uv1;
#endif


void main() {
	#if COLOR_MULTIPLIER == 0
		vec4 color = vec4( 1,1,1,1 );
	#elif COLOR_MULTIPLIER == 1
		vec4 color = u_colorMult;
	#endif
	
	#if COLORS == 1
		color = color * a_color;
	#endif	
	
	#if LIGHTING_DIFFUSE	
		vec3 normal = normalize( u_normalMatrix * vec4(a_normal,0) ).xyz;
		float nDotL = max( dot( normal, u_lightDir ), 0.0 );
		vec4 light = u_ambient + u_diffuse * nDotL;
		
		color = color * light;
	#endif
	
	#if TEXTURE0 == 1
		#if TEXTURE0_TRANSFORM == 0
			v_uv0 = a_uv0;
		#else
			v_uv0 = ( u_texMat0 * vec4( a_uv0.x, a_uv0.y, 0, 1 ) ).xy;
		#endif
	#endif
	#if TEXTURE1 == 1
		#if TEXTURE1_TRANSFORM == 0
			v_uv1 = a_uv1;
		#else
			v_uv1 = ( u_texMat1 * vec4( a_uv1.x, a_uv1.y, 0, 1 ) ).xy;
		#endif
	#endif

	v_color = color;
	gl_Position = u_mvpMatrix * vec4( a_pos.x, a_pos.y, a_pos.z, 1.0 );
}


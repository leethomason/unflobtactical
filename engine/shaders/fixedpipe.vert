
#if INSTANCE == 0
	uniform mat4 u_mvpMatrix;		// model-view-projection
#else
	uniform mat4 	u_vpMatrix;		// view-projection
	uniform max4 	u_mMatrix[16];
	attribute int   a_instanceID;
#endif

#if COLOR_MULTIPLIER == 1
	uniform vec4 u_colorMult;		
#endif

attribute vec3 a_pos;			// vertex position

#if COLORS == 1
	attribute vec3 a_color;			// vertex color
#endif

#if TEXTURE0 == 1
	#if TEXTURE0_TRANSFORM == 1
		uniform mat4 u_texMat0;
	#endif	
	#if TEXTURE0_3COMP == 1
		attribute vec3 a_uv0;		// note this is vec3 when there is a transform
	#else
		attribute vec2 a_uv0;
	#endif
	varying vec2 v_uv0;
#endif

#if TEXTURE1 == 1
	#if TEXTURE1_TRANSFORM == 1
		uniform mat4 u_texMat1;
	#endif
	#if TEXTURE1_3COMP == 1
		attribute vec3 a_uv1;
	#else
		attribute vec2 a_uv1;
	#endif
	varying vec2 v_uv1;
#endif

#if LIGHTING_DIFFUSE == 1
	uniform mat4 u_normalMatrix;	// normal transformation
	uniform vec3 u_lightDir;		// light direction, eye space (x,y,z,0)
	uniform vec4 u_ambient;			// ambient light. ambient+diffuse = 1
	uniform vec4 u_diffuse;			// diffuse light

	attribute vec3 a_normal;		// vertex normal
#endif

varying vec4 v_color;

void main() {
	#if COLOR_MULTIPLIER == 0
		vec4 color = vec4( 1,1,1,1 );
	#elif COLOR_MULTIPLIER == 1
		vec4 color = u_colorMult;
	#endif
	
	#if COLORS == 1
		color *= a_color;
	#endif

	#if LIGHTING_DIFFUSE == 1
		vec3 normal = normalize( ( u_normalMatrix * vec4( a_normal.x, a_normal.y, a_normal.z, 0 ) ).xyz );
		float nDotL = max( dot( normal, u_lightDir ), 0.0 );
		vec4 light = u_ambient + u_diffuse * nDotL;
		
		color *= light;
	#endif
	
	#if TEXTURE0 == 1
		#if TEXTURE0_TRANSFORM == 1
			#if TEXTURE0_3COMP == 1
				v_uv0 = ( u_texMat0 * vec4( a_uv0.x, a_uv0.y, a_uv0.z, 1 ) ).xy;
			#else
				v_uv0 = ( u_texMat0 * vec4( a_uv0.x, a_uv0.y, 0, 1 ) ).xy;
			#endif
		#else
			v_uv0 = a_uv0;
		#endif
	#endif
	#if TEXTURE1 == 1
		#if TEXTURE1_TRANSFORM == 1
			#if TEXTURE1_3COMP
				v_uv1 = ( u_texMat1 * vec4( a_uv1.x, a_uv1.y, a_uv1.z, 1 ) ).xy;
			#else
				v_uv1 = ( u_texMat1 * vec4( a_uv1.x, a_uv1.y, 0, 1 ) ).xy;
			#endif
		#else
			v_uv1 = a_uv1;
		#endif
	#endif

	v_color = color;
	#if INSTANCE == 0 
		gl_Position = u_mvpMatrix * vec4( a_pos.x, a_pos.y, a_pos.z, 1.0 );
	#elif INSTANCE == 1
		gl_Position = (u_mMatrix[a_instanceID] * u_vpMatrix) * vec4( a_pos.x, a_pos.y, a_pos.z, 1.0 );
	#endif
}


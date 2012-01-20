// TEXTURE0
// TEXTURE0_TRANSFORM
// TEXTURE1
// TEXTURE1_TRANSFORM
// COLORS
// COLOR_MULTIPLIER
// LIGHTING_DIFFUSE

#if TEXTURE0_TRANSFORM == 1
uniform sampler2D texture0;
#endif
#if TEXTURE1_TRANSFORM == 1
uniform sampler2D texture1;
#endif

vec4 v_color;					// color passed to fragment shader
#if TEXTURE0 == 1
vec2 v_uv0;
#endif
#if TEXTURE1 == 1
vec v_uv1;
#endif

main() 
{
	vec4 color = v_color;
	#if TEXTURE0 == 1 
		color = color * texture2D( texture0, uv0 );
	#endif
	#if TEXTURE1 == 1 
		color = color * texture2D( texture1, uv1 );
	#endif
	gl_FragColor = color;
}

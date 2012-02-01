// TEXTURE0
// TEXTURE0_TRANSFORM
// TEXTURE1
// TEXTURE1_TRANSFORM
// COLORS
// COLOR_MULTIPLIER
// LIGHTING_DIFFUSE

#if 1
uniform sampler2D texture0;

varying vec2 v_uv0;
varying vec4 v_color;

void main() 
{
	vec4 color = v_color;
	color *= texture2D( texture0, v_uv0.xy );
	gl_FragColor = color;
}
#endif

#if 0
#if TEXTURE0 == 1
uniform sampler2D texture0;
#endif
#if TEXTURE1 == 1
uniform sampler2D texture1;
#endif

vec4 v_color;					// color passed to fragment shader
#if TEXTURE0 == 1
varying vec2 v_uv0;
#endif
#if TEXTURE1 == 1
varying vec2 v_uv1;
#endif

void main() 
{
	vec4 color = v_color;
	#if TEXTURE0 == 1 
		color = color * texture2D( texture0, v_uv0.xy );
	#endif
	#if TEXTURE1 == 1 
		color = color * texture2D( texture1, v_uv1.xy );
	#endif
	gl_FragColor = color;
}
#endif
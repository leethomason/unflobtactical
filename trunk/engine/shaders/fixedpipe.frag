
#if TEXTURE0 == 1
	uniform sampler2D texture0;
	varying vec2 v_uv0;
#endif
#if TEXTURE1 == 1
	uniform sampler2D texture1;
	varying vec2 v_uv1;
#endif

varying vec4 v_color;

void main() 
{
	vec4 color = v_color;
	#if TEXTURE0 == 1
		#if TEXTURE0_ALPHA_ONLY == 1
			color.a *= texture2D( texture0, v_uv0).a;
		#else 
			color *= texture2D( texture0, v_uv0 );
		#endif
	#endif
	#if TEXTURE1 == 1
		#if TEXTURE1_ALPHA_ONLY == 1
			color.a *= texture2D( texture1, v_uv1).a;
		#else
			color *= texture2D( texture1, v_uv1 );
		#endif
	#endif
	gl_FragColor = color;
}


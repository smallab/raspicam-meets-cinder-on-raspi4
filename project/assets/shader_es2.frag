// BLUR

#version 100

precision highp float;

uniform sampler2D	tex0;
uniform vec2		sample_offset;
uniform float		attenuation;

varying highp vec2		TexCoord0;

void main()
{ 
	vec3 sum = vec3( 0.0, 0.0, 0.0 );	
	sum += texture2D( tex0, TexCoord0 + -10.0 * sample_offset ).rgb * 0.009167927656011385;
	sum += texture2D( tex0, TexCoord0 +  -9.0 * sample_offset ).rgb * 0.014053461291849008;
	sum += texture2D( tex0, TexCoord0 +  -8.0 * sample_offset ).rgb * 0.020595286319257878;
	sum += texture2D( tex0, TexCoord0 +  -7.0 * sample_offset ).rgb * 0.028855245532226279;
	sum += texture2D( tex0, TexCoord0 +  -6.0 * sample_offset ).rgb * 0.038650411513543079;
	sum += texture2D( tex0, TexCoord0 +  -5.0 * sample_offset ).rgb * 0.049494378859311142;
	sum += texture2D( tex0, TexCoord0 +  -4.0 * sample_offset ).rgb * 0.060594058578763078;
	sum += texture2D( tex0, TexCoord0 +  -3.0 * sample_offset ).rgb * 0.070921288047096992;
	sum += texture2D( tex0, TexCoord0 +  -2.0 * sample_offset ).rgb * 0.079358891804948081;
	sum += texture2D( tex0, TexCoord0 +  -1.0 * sample_offset ).rgb * 0.084895951965930902;
	sum += texture2D( tex0, TexCoord0 +   0.0 * sample_offset ).rgb * 0.086826196862124602;
	sum += texture2D( tex0, TexCoord0 +  +1.0 * sample_offset ).rgb * 0.084895951965930902;
	sum += texture2D( tex0, TexCoord0 +  +2.0 * sample_offset ).rgb * 0.079358891804948081;
	sum += texture2D( tex0, TexCoord0 +  +3.0 * sample_offset ).rgb * 0.070921288047096992;
	sum += texture2D( tex0, TexCoord0 +  +4.0 * sample_offset ).rgb * 0.060594058578763078;
	sum += texture2D( tex0, TexCoord0 +  +5.0 * sample_offset ).rgb * 0.049494378859311142;
	sum += texture2D( tex0, TexCoord0 +  +6.0 * sample_offset ).rgb * 0.038650411513543079;
	sum += texture2D( tex0, TexCoord0 +  +7.0 * sample_offset ).rgb * 0.028855245532226279;
	sum += texture2D( tex0, TexCoord0 +  +8.0 * sample_offset ).rgb * 0.020595286319257878;
	sum += texture2D( tex0, TexCoord0 +  +9.0 * sample_offset ).rgb * 0.014053461291849008;
	sum += texture2D( tex0, TexCoord0 + +10.0 * sample_offset ).rgb * 0.009167927656011385;

	gl_FragColor = vec4( attenuation * sum, 1.0 );
}
#version 330 compatibility

in vec3 vN, vL, vE;
in vec2 vST;
in vec3 vModelPos;

uniform sampler2D uNoise;
uniform sampler2D uNoiseMask;
uniform float uBumpTime;
uniform float uSpinTime;
uniform float uBulbTime;

const float M_PI = 3.141593;

vec3 
RotateNormal(float angx, float angy, float angz, vec3 n)
{
	float cx = cos(angx);
	float sx = sin(angx);
	float cy = cos(angy);
	float sy = sin(angy);
	float cz = cos(angz);
	float sz = sin(angz);

	// Rotate about x;
	mat3 rX = {
		{1., 0., 0.},
		{0., cx, -sx},
		{0., sx, cx}
	};
	n *= rX;

	// Rotate about y;
	mat3 rY = {
		{cy, 0., sy},
		{0., 1., 0.},
		{-sy, 0., cy}
	};
	n *= rY;

	// Rotate about z;
	mat3 rZ = {
		{cz, -sz, 0.},
		{sz, cz, 0.},
		{0., 0., 1.}
	};
	n *= rZ;

	return normalize(n);
}

void 
main()
{
	float noiseFreq = .01;
	float noiseAmp = .1;
	vec4 nv = texture2D(uNoise, vST);
	float flowT = fract(vST.t - uSpinTime);
	float flowS = fract(vST.s - uBulbTime);
	float flow = fract(sqrt(flowT * flowT + flowS * flowS));
	float seed = 235.534 * flow * noiseFreq * uBulbTime;
	float n = fract(8. * sin(seed * M_PI) + 4. * sin(seed * 4. * M_PI) + 2. * sin(seed * 8. * M_PI) + sin(seed * 16. * M_PI));	// range is 0. -> 1.
	n *= 2.;	// range is 0. -> 2.									
	n -= 1.;	// range is -1. -> 1.
	n *= noiseAmp;

	vec3 color = vec3(0.);

	int   stripes  = 10;
	float colWidth = 1. / float(stripes);
	int   curCol   = int(vST.s / colWidth);
	float stripeThick = 0.25 / float(stripes);
	float stripeCenter = float(curCol) * colWidth + (colWidth / 2.);
	float ds = abs(vST.s - stripeCenter);
	ds += n;
	float tol = 0.5 / stripes;
	float t = smoothstep(stripeThick - tol, stripeThick + tol, ds);
	color = mix(vec3(18. / 255., 69. / 255., 1.), vec3(1.), t);	//50. / 255., 131. / 255., 168. / 255.

	float mask =  texture2D(uNoiseMask, vec2((vST.s - uBulbTime), sin(flowT * 2. * M_PI))).g;
	color = mix(vec3(1.), color, mask);//205. / 255., 242. / 255., 250. / 255.

	float mask2 = texture2D(uNoiseMask, vec2((vST.s + uBulbTime) * 10., flowT * 10.)).r;
	color = mix(vec3(0., 204. / 255., 1.), color, mask2);	//20. / 255., 69. / 255., 1.

	
	// Do bump mapping
	// Get X-angle
	float bumpFreq = 10.;
	float bumpAmp = .5;
	vec4 bV = texture(uNoise, vST * bumpFreq - uBumpTime * bumpFreq);
	//float noise = fract(sin(bV.r * 2. * M_PI) + sin(bV.g * 4. * M_PI) / 2. + sin(bV.b * 8. * M_PI) / 4. + sin(bV.a * 16. * M_PI) / 8.);
	bV *= 2.;
	bV -= 1.;

	//noise *= 2.;
	//noise -= 1.;
	
	vec3 Normal = vN + bV.xyz;
	RotateNormal(bV.x, bV.y, bV.z, Normal);
	Normal = normalize(Normal);
	vec3 Light = normalize(vL);
	vec3 Eye = normalize(vE);
	float kA = 0.1;
	float kD = 0.7;
	float kS = 0.2;
	float shininess = 500.;

	vec3 ambient = kA * color;
	float d = max(dot(Normal, Light), 0.);
	vec3 diffuse = kD * d * color;

	float s = 0.;
	if(dot(Normal, Light) > 0.)
	{
		vec3 ref = normalize(reflect(-Light, Normal));
		s = pow(max(dot(Eye, ref), 0.), shininess);
	}
	vec3 specular = kS * s * vec3(1., 1., 1.);

	gl_FragColor = vec4(vec3(ambient + diffuse + specular), 1.);
}
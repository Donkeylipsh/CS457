#version 330 compatibility

in vec3 vN, vL, vE;
in vec2 vST;
in vec3 vModelPos;

uniform sampler2D uNoise;
uniform sampler2D uNoiseMask;
uniform sampler2D uWhoosh;
uniform float uBumpTime;
uniform float uSpinTime;
uniform float uBulbTime;
uniform float uStarTime;

const float M_PI = 3.141593;

float 
rand21(float s, float t)
{
	float num1 = fract(sin(s * 342.54 * 2. * M_PI) + sin(t * 987.21 * 2. * M_PI));
	float num2 = fract(cos(s * 28.54 * 2. * M_PI) + cos(t * 635.17 * 2. * M_PI));
	return dot(num1, num2);
}

vec4 
CreateStarfield()
{
	float snFreq = 10.;
	float snAmp = .5;
	vec4 nv = texture2D(uNoise, snFreq * vST - uStarTime);
	float n = nv.r + nv.g + nv.b + nv.a;
	n = (n - 2.);
	n *= snAmp;
	//n = 0.;
	vec4 col = vec4(0.);
	
	
	vec2 st = fract(vST);

	st.t -= uSpinTime;
	//st.t *= 4.;
	

	int cols = 2;
	int rows = 5;

	float width = 1. / float(cols);
	float height = 1. / float(rows);
	float rA = width / 2.;
	float rB = height / 2.;
	int curCol = int(st.s / width);
	int curRow = int(st.t / height);
	
	vec2 starPos = vec2(curCol * width + rA, curRow * height + rB);
	float dS = starPos.s - st.s;
	float dT = starPos.t - st.t;
	float sSquare = dS * dS;
	float tSquare = dT * dT;
	float dist = sqrt(sSquare + tSquare);
	float seed = rand21(float(curCol), float(curRow));
	float nD = dist + n;
	float scale = nD / dist;
	dS *= scale;
	dS /= rA;
	dT *= scale;
	dT /= rB;

	float border = dS * dS + dT * dT;
	
	float tol = 0.1;
	float t = smoothstep(1. - tol, 1. + tol, border);
	col = mix(vec4(18. / 255., 69. / 255., 1. ,1.), col, t);

	//if(dist < .01)
	//{
		//col += vec3(1. - dist * 10.);
	//}
	return col;
}

void main()
{
	// do the star field
	vec4 starColor = CreateStarfield();

	vec2 whooshST = vST;
	whooshST -= uBulbTime;
	whooshST.s *= 3. + .05 * sin(whooshST.t * 32. + uBulbTime * 32. * M_PI);
	whooshST.t *= 4. + .1 * sin(whooshST.s * 32. + uBulbTime * 32. * M_PI);
	whooshST.t *= 3.;
	vec4 col = texture2D(uWhoosh, fract(whooshST));
	col.a *= abs(sin(vST.t * 6. - uSpinTime * 4. * M_PI));
	if(col.rgb == vec3(0.)){
		col.a = 0.;
	}
	col += starColor;
	col = clamp(col, 0., 1.);
	if(col.a < .98){
		discard;
	}

	gl_FragColor = col;
}
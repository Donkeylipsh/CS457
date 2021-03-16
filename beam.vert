#version 330 compatibility

uniform vec3	uLightPos;
uniform float	uBulbTime;
uniform float	uSpinTime;
uniform int		uIsWhoosh;

out vec3 vN, vL, vE;
out vec2 vST;
out vec3 vModelPos;

const float M_PI = 3.141593;

void 
main()
{
	// Set texture out
	vST = gl_MultiTexCoord0.st;
	vec3 normal = gl_Normal;
	// Do major bulbs
	float bulbT = fract(vST.t - uBulbTime);
	float bulbS = fract(vST.s - uSpinTime + bulbT);
	int bulbs = 2;
	float bulbWidth = 1. / float(bulbs);
	int curBulb = int(bulbT / bulbWidth);
	float bulbCenter = float(curBulb) * bulbWidth + bulbWidth / 2.;
	float bulbRadius = 0.1 / float(bulbs);
	float t = smoothstep(bulbRadius, 0.0, bulbT - bulbCenter) - smoothstep(0.0, bulbRadius * 3., bulbCenter - bulbT);
	vModelPos = gl_Vertex.xyz;
	vModelPos.x *= (1 + t * 0.5 + 0.03 * cos(bulbS * 2. * M_PI * 10.));
	vModelPos.y *= (1 + t * 0.5 + 0.03 * sin(bulbS * 2. * M_PI * 10.));
	normal.x *= (1 + t * 0.5 + 0.03 * cos(bulbS * 2. * M_PI * 10.));
	normal.y *= (1 + t * 0.5 + 0.03 * sin(bulbS * 2. * M_PI * 10.));
	// Do mini blubs
	int miniBulbs = 25;
	float miniBulbWidth = 1. / float(miniBulbs);
	int curMiniBulb = int(bulbT / miniBulbWidth);
	float miniBulbCenter = float(curMiniBulb) * miniBulbWidth + miniBulbWidth / 2.;
	float miniBulbRadius = 0.1 / float(miniBulbs);
	float mbT = smoothstep(miniBulbRadius, 0.0, bulbT - miniBulbCenter) - smoothstep(0.0, miniBulbRadius, miniBulbCenter - bulbT);
	mbT *= (1. - t);
	vModelPos.x *= (1 + mbT * 0.05 + 0.0075 * cos(bulbS * 2. * M_PI * 10.));
	vModelPos.y *= (1 + mbT * 0.05 + 0.0075 * sin(bulbS * 2. * M_PI * 10.));
	normal.x *= (1 + mbT * 0.05 + 0.0075 * cos(bulbS * 2. * M_PI * 10.));
	normal.y *= (1 + mbT * 0.05 + 0.0075 * sin(bulbS * 2. * M_PI * 10.));
	normal = normalize(normal);
	float whooshScale = 0.05;
	vModelPos.x += (float(uIsWhoosh) * normal.x * whooshScale);
	vModelPos.y += (float(uIsWhoosh) * normal.y * whooshScale);
	vModelPos.z += (float(uIsWhoosh) * normal.z * whooshScale);

	// Set lighting outs
	vN = normalize(gl_NormalMatrix * normal);//gl_Normal);
	vec3 worldPos = (gl_ModelViewMatrix * vec4(vModelPos, 1.)).xyz;
	vL = uLightPos - worldPos;
	vE = vec3(0.) - worldPos;

	// Set position
	gl_Position = gl_ModelViewProjectionMatrix * vec4(vModelPos, 1.);
}
#version 330 compatibility

uniform int uIndex;


layout (std140, binding = 4) buffer Pos{
	vec4 Positions[];
};

layout(std140, binding = 5) buffer Vel{
    vec4 Velocities[];
};

layout(std140, binding = 6) buffer Col{
    vec4 Colors[];
};

void 
main()
{
	vec4 pos = Positions[1];

	gl_Position = gl_ModelViewProjectionMatrix * pos;
}
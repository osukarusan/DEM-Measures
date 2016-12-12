#version 330 core

in vec3 vertex;
out vec3 pos;

uniform mat4 ProjMatrix;
uniform mat4 ViewMatrix;

void main()  {
	pos = vertex;
    gl_Position = ProjMatrix * ViewMatrix * vec4(vertex, 1.0);
}

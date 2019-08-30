#version 450 core

#define MAX_BONES 100

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 norm;
layout (location = 2) in vec2 uv;

uniform mat4 mvp;

uniform mat4 bones_transform[MAX_BONES];

void main(){

	gl_Position = mvp * vec4(pos, 1.0);
	
}
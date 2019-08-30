#version 450 core

#define MAX_BONES 100

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 norm;
layout (location = 2) in vec2 uv;
layout (location = 3) in ivec3 bones_indices;
layout (location = 4) in vec3 bones_weight;

uniform mat4 mvp;

uniform mat4 bones_transform[MAX_BONES];

void main(){

	mat4 boneTransform =	bones_transform[bones_indices.x] * bones_weight.x +
							bones_transform[bones_indices.y] * bones_weight.y +
							bones_transform[bones_indices.z] * bones_weight.z;

	vec4 new_pos = boneTransform * vec4(pos, 1.0);
	
	gl_Position = mvp * new_pos;
	
}
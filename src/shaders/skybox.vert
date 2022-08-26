#version 450

layout (location = 0) out vec3 texCoord;

vec3 positions[] = {
	vec3(-1, -1, -1),
	vec3(-1,  1,  1),
	vec3(-1, -1,  1),

	vec3( 1, -1, -1),
	vec3(-1,  1, -1),
	vec3(-1, -1, -1),

	vec3( 1, -1,  1),
	vec3( 1,  1, -1),
	vec3( 1, -1, -1),

	vec3(-1, -1,  1),
	vec3( 1,  1,  1),
	vec3( 1, -1,  1),

	vec3(-1, -1,  1),
	vec3( 1, -1, -1),
	vec3(-1, -1, -1),

	vec3( 1,  1,  1),
	vec3(-1,  1, -1),
	vec3( 1,  1, -1),

	vec3(-1, -1, -1),
	vec3(-1,  1, -1),
	vec3(-1,  1,  1),

	vec3( 1, -1, -1),
	vec3( 1,  1, -1),
	vec3(-1,  1, -1),

	vec3( 1, -1,  1),
	vec3( 1,  1,  1),
	vec3( 1,  1, -1),

	vec3(-1, -1,  1),
	vec3(-1,  1,  1),
	vec3( 1,  1,  1),

	vec3(-1, -1,  1),
	vec3( 1, -1,  1),
	vec3( 1, -1, -1),

	vec3( 1,  1,  1),
	vec3(-1,  1,  1),
	vec3(-1,  1, -1),
};

layout (set = 0, binding = 0) uniform CameraBuffer {
	mat4 view;
	mat4 projection;
	vec3 position;
} camera;

layout (set = 0, binding = 1) uniform SceneData {
	vec4 fogColor; // w is for exponent
	vec4 fogDistances; // x = min, y = max, zw unused
	vec4 ambientColor; // w is for power
	vec4 sunlightDirection; // w = sun power
	vec4 sunlightColor;
} sceneData;

void main() {
	mat4 view = camera.view;
	view[3] -= vec4(view[3].xyz, 0);

	vec4 pos = camera.projection * view * vec4(positions[gl_VertexIndex], 1.0);
	pos.z = 0;
	gl_Position = pos;

	texCoord = positions[gl_VertexIndex];
}


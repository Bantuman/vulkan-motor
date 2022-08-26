#version 450

layout (location = 0) out vec2 texCoord;

vec2 positions[3] = vec2[](
	vec2(3.0, -1.0),
	vec2(-1.0, 3.0),
	vec2(-1.0, -1.0)
);

vec3 colors[3] = vec3[](
	vec3(1.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, 0.0, 1.0)
);

layout (set = 0, binding = 1) uniform CameraBuffer {
	mat4 view;
	mat4 projection;
	vec3 position;
} camera;

void main() {
	gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
	//fragColor = colors[gl_VertexIndex];
	texCoord = positions[gl_VertexIndex].xy * 0.5 + vec2(0.5);
}


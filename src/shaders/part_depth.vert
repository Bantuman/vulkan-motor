#version 460

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec3 texCoord;

layout (location = 4) in mat4x3 model;
layout (location = 8) in vec4 scale;
layout (location = 9) in vec4 color;
layout (location = 10) in uint surfaceIDs;

layout (set = 0, binding = 0) uniform CameraBuffer {
	mat4 view;
	mat4 projection;
	vec3 position;
} camera;

void main() {
	const mat4 mv = camera.view * mat4(model);
	const mat4 mvp = camera.projection * mv;
	const vec4 scaledPos = vec4(position * scale.xyz, 1.0);

	gl_Position = mvp * scaledPos;
}


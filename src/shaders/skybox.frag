#version 450
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec3 texCoord;

layout (location = 0) out vec4 outColor;

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

layout (set = 1, binding = 0) uniform samplerCube skybox;

void main() {
	outColor = vec4(texture(skybox, texCoord).xyz, 1.0);
}


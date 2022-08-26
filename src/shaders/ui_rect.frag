#version 450
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec2 texCoord;
layout (location = 1) in vec4 inColor;
layout (location = 2) flat in uvec2 inImageIndices;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler samplers[2];
layout (set = 0, binding = 1) uniform texture2D textures[32];

void main() {
	vec4 color = inColor * texture(
			sampler2D(textures[inImageIndices.x], samplers[inImageIndices.y]),
			texCoord);
	outColor = vec4(pow(color.rgb, vec3(2.199)), color.a);
}


#version 450
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec4 texLayout;
layout (location = 1) in vec4 inColor;
layout (location = 2) in mat3x2 transform;
layout (location = 5) in uvec2 inImageIndices;

layout (location = 0) out vec2 texCoord;
layout (location = 1) out vec4 outColor;
layout (location = 2) out uvec2 outImageIndices;

vec2 positions[6] = vec2[](
	vec2(-0.5, -0.5),
	vec2(-0.5, 0.5),
	vec2(0.5, 0.5),

	vec2(0.5, 0.5),
	vec2(0.5, -0.5),
	vec2(-0.5, -0.5)
);

vec2 texCoords[6] = vec2[](
	vec2(0, 0),
	vec2(0, 1),
	vec2(1, 1),

	vec2(1, 1),
	vec2(1, 0),
	vec2(0, 0)
);

void main() {
	gl_Position = vec4(transform * vec3(positions[gl_VertexIndex], 1.0), 0.0, 1.0);
	texCoord = fma(texCoords[gl_VertexIndex], texLayout.zw, texLayout.xy);
	outColor = inColor;
	outImageIndices = inImageIndices;
}


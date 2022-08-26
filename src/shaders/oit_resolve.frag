#version 450

//layout (location = 0) in vec2 texCoord;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0, rgba32ui) uniform readonly uimage2D colorBuffer;
layout (set = 0, binding = 1, rgba8) uniform image2D visibilityBuffer;

void main() {
	ivec2 coord = ivec2(gl_FragCoord.xy);

	vec4 vis = imageLoad(visibilityBuffer, coord);

	imageStore(visibilityBuffer, coord, vec4(1.0));

	if (vis.x == 1.0) {
		discard;
	}

	uvec4 color = imageLoad(colorBuffer, coord);

	vec3 fragColor = unpackUnorm4x8(color[0]).rgb;
	float v = vis[0];

	fragColor += unpackUnorm4x8(color[1]).rgb * v;
	v *= vis[1];

	fragColor += unpackUnorm4x8(color[2]).rgb * v;
	v *= vis[2];

	fragColor += unpackUnorm4x8(color[3]).rgb * v;
	v *= vis[3];

	outColor = vec4(fragColor, v);
}


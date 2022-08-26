#version 460

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec3 texCoord;

layout (location = 4) in mat4x3 model;
layout (location = 8) in vec3 scale;
layout (location = 9) in vec4 color;
layout (location = 10) in uint imageIndex;

layout (location = 0) out vec2 outTexCoord;
layout (location = 1) out vec4 outColor;
layout (location = 2) out uint outImageIndex;
layout (location = 3) out vec3 outTangentFragPos;
layout (location = 4) out vec3 outTangentLightDir;

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
	const mat4 mv = camera.view * mat4(model);
	const mat4 mvp = camera.projection * mv;
	const vec4 scaledPos = vec4(position * scale, 1.0);

	gl_Position = mvp * scaledPos;

	const vec3 B = cross(normal, tangent);
	const mat3 TBN = transpose(mat3(mv) * mat3(B, tangent, normal));

	outTexCoord = -texCoord.xy; // ugly hack, I should figure out why its negated
	outColor = color;
	outImageIndex = imageIndex;
	outTangentFragPos = TBN * (mv * scaledPos).xyz;
	outTangentLightDir = TBN * sceneData.sunlightDirection.xyz;
}


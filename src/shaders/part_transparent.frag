#version 450

layout (location = 0) in vec4 inColor;
layout (location = 1) in vec3 inPosition;
layout (location = 2) in vec3 inTexCoord;
layout (location = 3) in vec3 inLightDir;

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

layout (set = 0, binding = 3) uniform sampler2D texDiffuse;
layout (set = 0, binding = 4) uniform sampler2D texNormal;

void main() {
	const vec2 texCoord = vec2(inTexCoord.x, fma(mod(inTexCoord.y, 1.f), 0.0625f, inTexCoord.z));
	const vec3 normal = normalize(fma(texture(texNormal, texCoord).xyz, vec3(2.0), vec3(-1.0)));

	const vec3 viewDir = normalize(-inPosition);
	const vec3 reflectDir = reflect(-inLightDir, normal);

	const float diff = max(dot(normal, inLightDir), 0.0);
	const float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16) * 0.5;

	const vec3 light = sceneData.ambientColor.xyz * sceneData.ambientColor.w
			+ sceneData.sunlightColor.xyz * (diff + spec);
	
	const vec3 texColor = texture(texDiffuse, texCoord).xyz;

	outColor = vec4(inColor.xyz * texColor * light, inColor.w);
}


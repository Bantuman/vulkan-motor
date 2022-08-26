#version 460

layout (location = 0) in vec2 texCoord;
layout (location = 1) in vec4 color;
layout (location = 2) flat in uint imageIndex;
layout (location = 3) in vec3 tangentFragPos;
layout (location = 4) in vec3 tangentLightDir;

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

layout (set = 1, binding = 0) uniform sampler2D textures[32];

void main() {
	const vec3 normal = vec3(0, 0, 1);
	const vec3 viewDir = normalize(-tangentFragPos);
	const vec3 reflectDir = reflect(-tangentLightDir, normal);

	const float diff = max(dot(normal, tangentLightDir), 0.0);
	//const float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16) * 0.5;
	const vec3 halfDir = normalize(tangentLightDir + viewDir);
	const float spec = pow(max(dot(halfDir, normal), 0.0), 16) * 0.5;
	
	const vec3 light = sceneData.ambientColor.xyz * sceneData.ambientColor.w
			+ sceneData.sunlightColor.xyz * (diff + spec);
	
	const vec4 texColor = texture(textures[imageIndex], texCoord);

	outColor = vec4(color.rgb * texColor.rgb * light, color.a * texColor.a);
}


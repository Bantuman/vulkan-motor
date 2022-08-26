#version 460

layout (location = 0) in VS_OUT {
	vec2 texCoord;
	vec3 tangentFragPos;
	vec3 tangentLightDir;
	flat uvec2 imageIndices;
	//mat4 debug;
} vs_out;

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

layout (set = 2, binding = 0) uniform sampler2D textures[32];

void main() {
	const vec3 normal = normalize(fma(texture(textures[vs_out.imageIndices.y],
			vs_out.texCoord).xyz, vec3(2.0), vec3(-1.0)));

	const vec3 viewDir = normalize(-vs_out.tangentFragPos);
	const vec3 reflectDir = reflect(-vs_out.tangentLightDir, normal);

	const float diff = max(dot(normal, vs_out.tangentLightDir), 0.0);
	//const float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16) * 0.5;
	const vec3 halfDir = normalize(vs_out.tangentLightDir + viewDir);
	const float spec = pow(max(dot(halfDir, normal), 0.0), 16) * 0.5;

	const vec3 texColor = texture(textures[vs_out.imageIndices.x], vs_out.texCoord).rgb;
	
	const vec3 light = sceneData.ambientColor.xyz * sceneData.ambientColor.w
			+ sceneData.sunlightColor.xyz * (diff + spec);

	outColor = vec4(texColor * light, 1.0);
	//outColor = vec4(vs_out.tangentLightDir, 1.0);
}


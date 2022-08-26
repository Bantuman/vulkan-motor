#version 450

layout (location = 0) in VS_OUT {
	vec4 color;
	vec3 texCoord;
	vec3 tangentFragPos;
	vec3 tangentLightDir;
	mat3 TBN;
	float reflectance;
	vec3 globalFragPos;
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

layout (set = 0, binding = 3) uniform sampler2D texDiffuse;
layout (set = 0, binding = 4) uniform sampler2D texNormal;
layout (set = 0, binding = 5) uniform samplerCube skybox;

void main() {
	const vec2 texCoord = vec2(vs_out.texCoord.x, fma(mod(vs_out.texCoord.y, 1.f), 0.0625f,
				vs_out.texCoord.z));
	const vec3 normal = normalize(fma(texture(texNormal, texCoord).xyz, vec3(2.0), vec3(-1.0)));

	const vec3 viewDir = normalize(-vs_out.tangentFragPos);
	const vec3 reflectDir = reflect(-vs_out.tangentLightDir, normal);

	//const float diff = max(dot(normal, vs_out.tangentLightDir), 0.0);
	const float nDotL = dot(normal, vs_out.tangentLightDir);
	const float diff = smoothstep(0, 0.01, nDotL);
	//const float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16) * 0.5;
	const vec3 halfDir = normalize(vs_out.tangentLightDir + viewDir);
	float spec = pow(max(dot(halfDir, normal), 0.0), 32 * 32);
	spec = smoothstep(0.005, 0.01, spec);

	float rimDot = 1.0 - dot(viewDir, normal);
	float rimAmount = 0.716;
	float rimIntensity = rimDot * nDotL;
	rimIntensity = smoothstep(rimAmount - 0.01, rimAmount + 0.01, rimIntensity);
	
	const vec3 light = sceneData.ambientColor.xyz * sceneData.ambientColor.w
			+ sceneData.sunlightColor.xyz * (diff + spec + rimIntensity);
	
	const vec3 texColor = texture(texDiffuse, texCoord).xyz;

	const vec3 color = mix(vs_out.color.xyz, texture(skybox, reflect(vs_out.globalFragPos
			- camera.position, vs_out.TBN * normal)).xyz, vs_out.reflectance);

	outColor = vec4(color * texColor * light, vs_out.color.w);
}


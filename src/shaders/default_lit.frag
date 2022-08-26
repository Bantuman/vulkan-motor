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
	vec4 projInfo;
	vec2 screenSize;
} camera;

layout (set = 0, binding = 1) uniform SceneData {
	vec4 fogColor; // w is for exponent
	vec4 fogDistances; // x = min, y = max, zw unused
	vec4 ambientColor; // w is for power
	vec4 sunlightDirection; // w = sun power
	vec4 sunlightColor;
} sceneData;

layout (set = 1, binding = 0) uniform sampler2D texDiffuse;
layout (set = 1, binding = 1) uniform sampler2D texNormal;
layout (set = 1, binding = 2) uniform samplerCube skybox;

layout (set = 2, binding = 0) uniform sampler2D aoMap;

void main() {
	const vec2 texCoord = vec2(vs_out.texCoord.x, fma(mod(vs_out.texCoord.y, 1.f), 0.0625f,
				vs_out.texCoord.z));
	const vec3 normal = normalize(fma(texture(texNormal, texCoord).xyz, vec3(2.0), vec3(-1.0)));

	const float ao = texture(aoMap, gl_FragCoord.xy / camera.screenSize).r;

	const vec3 viewDir = normalize(-vs_out.tangentFragPos);
	const vec3 reflectDir = reflect(-vs_out.tangentLightDir, normal);

	const float diff = max(dot(normal, vs_out.tangentLightDir), 0.0);
	//const float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16) * 0.5;
	const vec3 halfDir = normalize(vs_out.tangentLightDir + viewDir);
	const float spec = pow(max(dot(halfDir, normal), 0.0), 16) * 0.5;
	
	const vec3 light = (sceneData.ambientColor.xyz * (sceneData.ambientColor.w * ao))
			+ sceneData.sunlightColor.xyz * (diff + spec);
	
	const vec3 texColor = texture(texDiffuse, texCoord).xyz;

	const vec3 color = mix(vs_out.color.xyz, texture(skybox, reflect(vs_out.globalFragPos
			- camera.position, vs_out.TBN * normal)).xyz, vs_out.reflectance);

	outColor = vec4(color * texColor * light, vs_out.color.w);
}


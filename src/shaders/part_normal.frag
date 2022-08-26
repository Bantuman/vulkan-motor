#version 450

layout (location = 0) in VS_OUT {
	vec3 texCoord;
	mat3 TBN;
	vec3 tempPosition;
} vs_out;

layout (location = 0) out vec4 outNormal;

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

layout (set = 1, binding = 0) uniform sampler2D texDiffuse;
layout (set = 1, binding = 1) uniform sampler2D texNormal;
layout (set = 1, binding = 2) uniform samplerCube skybox;

void main() {
	const vec2 texCoord = vec2(vs_out.texCoord.x, fma(mod(vs_out.texCoord.y, 1.f), 0.0625f,
				vs_out.texCoord.z));
	const vec3 normal = normalize(fma(texture(texNormal, texCoord).xyz, vec3(2.0), vec3(-1.0)));

	outNormal = vec4(fma(vs_out.TBN * normal, vec3(0.5), vec3(0.5)), 1.0);
	//outNormal = vec4(fma(vs_out.tempPosition, vec3(0.005), vec3(0.005)), 1.0);
	//outNormal = vec4(vs_out.tempPosition, 1.0);
}


#version 460

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec3 texCoord;

layout (location = 4) in mat4x3 model;
layout (location = 8) in vec4 scale;
layout (location = 9) in vec4 color;
layout (location = 10) in uint surfaceIDs;

layout (location = 0) out VS_OUT {
	vec4 color;
	vec3 texCoord;
	vec3 tangentFragPos;
	vec3 tangentLightDir;
	mat3 TBN;
	float reflectance;
	vec3 globalFragPos;
} vs_out;

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
	const vec4 scaledPos = vec4(position * scale.xyz, 1.0);

	gl_Position = mvp * scaledPos;

	//const mat3 TBN = transpose(mat3(mv) * mat3(-bitangent, tangent, normal));
	const vec3 B = cross(normal, tangent);
	const mat3 TBN = transpose(mat3(mv) * mat3(B, tangent, normal));
	vs_out.color = color;
	vs_out.tangentFragPos = TBN * (mv * scaledPos).xyz;
	vs_out.tangentLightDir = TBN * sceneData.sunlightDirection.xyz;

	const uint faceIndex = uint(texCoord.z);
	const vec2 scale2D =
			(float(faceIndex == 0 || faceIndex == 3) * scale.zy +
			float(faceIndex == 1 || faceIndex == 4) * scale.zx +
			float(faceIndex == 2 || faceIndex == 5) * scale.xy) * 0.5f;
	const float surfaceID =
			float((surfaceIDs >> (4 * faceIndex)) & 0xF);

	vs_out.texCoord = vec3(texCoord.xy * scale2D, surfaceID * 0.0625f);
	vs_out.reflectance = scale.w;
	//vs_out.TBN = mat3(model) * mat3(-bitangent, tangent, normal);
	//vs_out.TBN = mat3(model) * mat3(tangent, B, normal);
	vs_out.TBN = mat3(model) * mat3(-B, tangent, normal);
	vs_out.globalFragPos = (model * scaledPos).xyz;
}


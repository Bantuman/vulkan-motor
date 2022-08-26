#version 460

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec3 bitangent;
layout (location = 4) in vec3 texCoord;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec3 outPosition;
layout (location = 2) out vec3 outTexCoord;
layout (location = 3) out vec3 outLightDir;
layout (location = 4) out float outReflectance;

layout (set = 0, binding = 0) uniform CameraBuffer {
	mat4 view;
	mat4 projection;
	vec3 position;
} camera;

struct InstanceData {
	mat4x3 transform;
	vec4 scale;
	vec4 color;
	uint surfaceIDs;
	//uint padding[3];
};

layout (std140, set = 1, binding = 2, row_major) readonly buffer ObjectBuffer {
	InstanceData objects[];
} objectBuffer;

layout (set = 0, binding = 1) uniform SceneData {
	vec4 fogColor; // w is for exponent
	vec4 fogDistances; // x = min, y = max, zw unused
	vec4 ambientColor; // w is for power
	vec4 sunlightDirection; // w = sun power
	vec4 sunlightColor;
} sceneData;

void main() {
	const mat4x3 model = objectBuffer.objects[gl_InstanceIndex].transform;
	const mat4 mv = camera.view * mat4(model);
	const mat4 mvp = camera.projection * mv;
	const vec3 scale = objectBuffer.objects[gl_InstanceIndex].scale.xyz;
	const vec4 scaledPos = vec4(position * scale, 1.0);

	gl_Position = mvp * scaledPos;

	const mat3 TBN = transpose(mat3(mv) * mat3(-bitangent, tangent, normal));
	outColor = objectBuffer.objects[gl_InstanceIndex].color;
	outPosition = TBN * (mv * scaledPos).xyz;
	outLightDir = TBN * sceneData.sunlightDirection.xyz;

	const uint faceIndex = uint(texCoord.z);
	const vec2 scale2D =
			(float(faceIndex == 0 || faceIndex == 3) * scale.yz +
			float(faceIndex == 1 || faceIndex == 4) * scale.xz +
			float(faceIndex == 2 || faceIndex == 5) * scale.yx) * 0.5f;
	const float surfaceID =
			float((objectBuffer.objects[gl_InstanceIndex].surfaceIDs >> (4 * faceIndex)) & 0xF);

	outTexCoord = vec3(texCoord.xy * scale2D, surfaceID * 0.0625f);
	outReflectance = objectBuffer.objects[gl_InstanceIndex].scale.w;
}


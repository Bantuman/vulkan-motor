#version 460

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec2 texCoord;
layout (location = 4) in vec4 boneWeights;
layout (location = 5) in uvec4 boneIndices;

layout (location = 6) in mat4x3 transform;
layout (location = 10) in float reflectance;
layout (location = 11) in vec4 color;
layout (location = 12) in uvec3 indices;

layout (location = 0) out VS_OUT {
	vec2 texCoord;
	vec3 tangentFragPos;
	vec3 tangentLightDir;
	uvec2 imageIndices;
	//mat4 debug;
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

layout (set = 1, binding = 0) readonly buffer Rig {
	mat4 bones[];
} rig;

vec3 minor(in vec3 r0, in vec3 r1, in vec3 r2) {
	return r0 * (r1.yxx * r2.zzy - r2.yxx * r1.zzy);
	//return r0 * cross(r1, r2) * vec3(1, -1, 1);
}

vec3 minor3(in mat4 src, int r0, int r1, int r2) {
	const mat3 r = transpose(mat3(
		minor(src[r0].yzw, src[r1].yzw, src[r2].yzw),
		minor(src[r0].xzw, src[r1].xzw, src[r2].xzw),
		minor(src[r0].xyw, src[r1].xyw, src[r2].xyw)
	));

	return r[0] - r[1] + r[2];
}

mat3 fast_cofactor(in mat4 src) {
	return mat3(
		minor3(src, 1, 2, 3) * vec3(1, -1, 1),
		minor3(src, 0, 2, 3) * vec3(-1, 1, -1),
		minor3(src, 0, 1, 3) * vec3(1, -1, 1)
	);
}

float get_outline_camera_fov_and_distance_fix_multiplier(float viewSpacePositionZ) {
	float cameraMulFix = clamp(abs(viewSpacePositionZ), 0.0, 1.0) * 70.0;
		// original formula multiplies by the FOV, so 70 hardcoded
	return cameraMulFix * 0.00005; // hardcoded constant from tutorial
}

vec3 get_outline_position_model_space(in vec3 posModelSpace, float viewSpacePosZ,
		in vec3 modelSpaceNormal) {
	const float outlineSize = 1.0;
	float outlineExpandAmount = outlineSize * get_outline_camera_fov_and_distance_fix_multiplier(viewSpacePosZ);
	return fma(modelSpaceNormal, vec3(outlineExpandAmount), posModelSpace);
}

vec4 get_new_clip_pos_with_z_offset(in vec4 originalClipPos, float viewSpaceZOffsetAmount) {
	vec2 projZW = vec2(camera.projection[2][2], camera.projection[3][2]);
	float modifiedViewPosZ = -originalClipPos.w - viewSpaceZOffsetAmount; // push imaginary vertex
	float modifiedClipPosZ = fma(modifiedViewPosZ, projZW.x, projZW.y);

	vec4 result = originalClipPos;
	result.z = modifiedClipPosZ * originalClipPos.w / (-modifiedViewPosZ);
	return result;
}

void main() {
	const uint baseIndex = indices.z;

	const mat4 jointTransform = rig.bones[baseIndex + boneIndices[0]] * boneWeights[0]
			+ rig.bones[baseIndex + boneIndices[1]] * boneWeights[1]
			+ rig.bones[baseIndex + boneIndices[2]] * boneWeights[2]
			+ rig.bones[baseIndex + boneIndices[3]] * boneWeights[3];

	const mat4 mv = camera.view * (mat4(transform) * jointTransform);
	const mat3 cof = fast_cofactor(mv);
	const mat4 mvp = camera.projection * mv;

	const vec4 viewPos = mv * vec4(position, 1.0);
	const vec3 outlinePos = get_outline_position_model_space(position, viewPos.z, normal);

	float outlineZOffset = 0.0001;

	gl_Position = get_new_clip_pos_with_z_offset(mvp * vec4(outlinePos, 1.0), outlineZOffset + 0.03);

	vec3 T = normalize(cof * tangent);
	const vec3 N = normalize(cof * normal);
	T = normalize(T - dot(T, N) * N);
	const vec3 B = cross(T, N);

	const mat3 TBN = transpose(mat3(B, T, N));
	//const mat3 TBN = transpose(mat3(T, -B, N));

	vs_out.tangentFragPos = TBN * (mv * vec4(outlinePos, 1.0)).xyz;
	vs_out.tangentLightDir = TBN * sceneData.sunlightDirection.xyz;
	//vs_out.tangentLightDir = fma(tangent, vec3(0.5), vec3(0.5));//TBN * sceneData.sunlightDirection.xyz;
	vs_out.texCoord = texCoord;
	vs_out.imageIndices = indices.xy;
}


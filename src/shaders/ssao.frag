#version 450

// Total number of direct samples to take at each pixel
#define NUM_SAMPLES			(11)

// If using depth mip levels, the log of the maximum pixel offset before we need to switch to a lower 
// miplevel to maintain reasonable spatial locality in the cache
// If this number is too small (< 3), too many taps will land in the same pixel, and we'll get bad variance that manifests as flashing.
// If it is too high (> 5), we'll get bad performance because we're not using the MIP levels effectively
#define LOG_MAX_OFFSET		3

// This must be less than or equal to the MAX_MIP_LEVEL defined in C++
#define MAX_MIP_LEVEL 5

/** Used for preventing AO computation on the sky (at infinite depth) and defining the clip space Z to bilateral depth key scaling. 
    This need not match the real far plane*/
#define FAR_PLANE_Z (300.0)

// This is the number of turns around the circle that the spiral pattern makes.  This should be prime to prevent
// taps from lining up.  This particular choice was tuned for NUM_SAMPLES == 9
#define NUM_SPIRAL_TURNS (7)

//layout (location = 0) in vec2 texCoord;

layout (location = 0) out vec4 outSSAO;

layout (set = 0, binding = 0) uniform CameraBuffer {
	mat4 view;
	mat4 projection;
	vec3 position;
	vec4 projInfo;
} camera;

layout (set = 0, binding = 1) uniform SSAOInfo {
	float projScale;
	float bias;
	float intensity;
	float radius;
	float radius2; // radius * radius
	float intensityDivR6; // intensity / (radius^6)
} ssao;

layout (set = 0, binding = 2) uniform sampler2D depthBuffer;

vec3 reconstruct_camera_space_position(vec2 pixelPos, float z) {
	return vec3(fma(pixelPos, camera.projInfo.xx * vec2(1.0, -1.0), camera.projInfo.yz),
			camera.projInfo.w) / z;
}

vec3 reconstruct_camera_space_normal(in vec3 positionCS) {
	return normalize(cross(dFdy(positionCS), dFdx(positionCS)));
}

vec3 get_offset_position(ivec2 coord, vec2 unitOffset, float ssR) {
	const int mipLevel = clamp(findMSB(int(ssR)) - LOG_MAX_OFFSET, 0, MAX_MIP_LEVEL);
	const ivec2 ssP = ivec2(ssR * unitOffset) + coord;

	// We need to divide by 2^mipLevel to read the appropriately scaled coordinate from a mipmap
	// Manually clamped to the texture size because texelFetch bypasses the texture unit
	const ivec2 mipP = clamp(ssP >> mipLevel, ivec2(0),
			textureSize(depthBuffer, mipLevel) - ivec2(1));

	return reconstruct_camera_space_position(vec2(ssP) + vec2(0.5), 
			texelFetch(depthBuffer, mipP, mipLevel).r);
}

// Returns a unit vector and a screen-space radius for the tap on a unit disk
// The caller should scale by the actual disk radius
vec2 tap_location(int sampleNumber, float spinAngle, out float ssR) {
	const float alpha = float(sampleNumber + 0.5) * (1.0 / NUM_SAMPLES);
	const float angle = alpha * (NUM_SPIRAL_TURNS * 6.28) + spinAngle;

	ssR = alpha;
	return vec2(cos(angle), sin(angle));
}

// Compute the occlusion due to sample with index i about the pixel at coord that corresponds
// to camera-space point positionCS with unit normal normalCS, using maximum screen-space sampling
// radius ssDiskRadius.
float sample_ao(in ivec2 coord, in vec3 positionCS, in vec3 normalCS, float ssDiskRadius,
		int tapIndex, float randomPatternRotationAngle) {
	// Offset on the unit disk, spun for this pixel
	float ssR;
	vec2 unitOffset = tap_location(tapIndex, randomPatternRotationAngle, ssR);
	ssR *= ssDiskRadius;

	// The occluding point in camera space
	vec3 Q = get_offset_position(coord, unitOffset, ssR);

	vec3 v = Q - positionCS;

	float vv = dot(v, v);
	float vn = dot(v, normalCS);

	const float epsilon = 0.01;

	// A: From the HPG12 paper
	// Note large epsilon to avoid overdarkening within cracks
	// return float(vv < ssao.radius2) * max((vn - ssao.bias) / (epsilon + vv), 0.0)
	//		* ssao.radius2 * 0.6;
	
	// B: Smoother transition to zero (lowers contrast, smoothing out corners.) [Recommended]
	float f = max(ssao.radius2 - vv, 0.0);
	return f * f * f * max((vn - ssao.bias) / (epsilon + vv), 0.0);

	// C: Medium contrast (which looks better at high radii), no division. Note that the
	// contribution still falls off with radius^2, but we've adjusted the rate in a way that is
	// more computationally efficient and happens to be aesthetically pleasing.
	// return 4.0 * max(1.0 - vv * ssao.invRadius2, 0.0) * max(vn - ssao.bias, 0.0);
	
	// D: Low contrast, no division operation
	// return 2.0 * float(vv < ssao.radius2) * max(vn - ssao.bias, 0.0);
}

// Used for packing Z into the GB channels
float z_to_key(float z) {
	return clamp(z * (1.0 / FAR_PLANE_Z), 0.0, 1.0);
}

vec2 pack_key(float key) {
	// Round to the nearest 1/256.0
	const float temp = floor(key * 256.0);
	return vec2(temp * (1.0 / 256.0), key * 256.0 - temp);
}

void main() {
	const ivec2 coord = ivec2(gl_FragCoord.xy);

	const float depth = texelFetch(depthBuffer, coord, 0).r;

	const vec3 positionCS = reconstruct_camera_space_position(gl_FragCoord.xy, depth);
	const vec3 normalCS = reconstruct_camera_space_normal(positionCS);

	// Hash function used in the HPG12 AlchemyAO paper
	const float randomPatternRotationAngle = (3 * coord.x ^ coord.y + coord.x * coord.y) * 10;

	// Choose the screens-apce sample radius proportional to the projected area of the sphere
	const float ssDiskRadius = -ssao.projScale * ssao.radius / positionCS.z;

	float sum = 0.0;

	for (int i = 0; i < NUM_SAMPLES; ++i) {
		sum += sample_ao(coord, positionCS, normalCS, ssDiskRadius, i, randomPatternRotationAngle);
	}

	float A = max(0.0, 1.0 - sum * ssao.intensityDivR6 * (5.0 / NUM_SAMPLES));

	// Bilateral box-filter over a quad for free, respecting depth edges
	// (the difference that this maxes is subtle)
	if (abs(dFdx(positionCS.z)) < 0.02) {
		A -= dFdx(A) * ((coord.x & 1) - 0.5);
	}

	if (abs(dFdy(positionCS.z)) < 0.02) {
		A -= dFdy(A) * ((coord.y & 1) - 0.5);
	}

	outSSAO = vec4(vec3(A, pack_key(z_to_key(depth))), 1.0);
	//outSSAO = vec4(vec3(A), 1.0);
	//outSSAO = vec4(fma(normalCS, vec3(0.5), vec3(0.5)), 1.0);
	//outSSAO = vec4(fma(positionCS, vec3(0.005), vec3(0.005)), 1.0);
}


#version 450

layout (early_fragment_tests) in;

layout (location = 0) in VS_OUT {
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

layout (set = 1, binding = 0) uniform sampler2D texDiffuse;
layout (set = 1, binding = 1) uniform sampler2D texNormal;
layout (set = 1, binding = 2) uniform samplerCube skybox;

layout (set = 2, binding = 0, rgba32ui) uniform coherent uimage2D colorBuffer;
layout (set = 2, binding = 1, rgba16ui) uniform coherent uimage2D depthBuffer;
layout (set = 2, binding = 2, rgba8) uniform coherent image2D visibilityBuffer;
layout (set = 2, binding = 3, r32ui) uniform coherent uimage2D lock;

uint encode_depth(in float depthIn) {
	return bitfieldExtract(floatBitsToUint(depthIn), 14, 16);
}

void swap_node(inout float a_vis, inout uint a_depth, inout uint a_col, inout float b_vis,
		inout uint b_depth, inout uint b_col) {
	float t_vis = a_vis;
	uint t_depth = a_depth;
	uint t_col = a_col;

	a_vis = b_vis;
	a_depth = b_depth;
	a_col = b_col;

	b_vis = t_vis;
	b_depth = t_depth;
	b_col = t_col;
}

void insert_node(in ivec2 coord, in uint depthIn, in float visIn, in uint colorIn) {
	memoryBarrierImage();

	uvec4 depth = imageLoad(depthBuffer, coord);
	vec4 vis = imageLoad(visibilityBuffer, coord);

	bvec4 notValidMask = equal(vec4(1.0), vis);
	depth = floatBitsToUint(mix(uintBitsToFloat(depth), vec4(0.0), notValidMask));
	bvec4 closerMask = greaterThanEqual(uvec4(depthIn), depth);
	vec4 maskedVis = mix(vis, vec4(1.0), closerMask);

	uvec4 color = imageLoad(colorBuffer, coord);
	color = floatBitsToUint(mix(uintBitsToFloat(color), vec4(0.0), notValidMask));

	if (closerMask[0]) {
		swap_node(vis[0], depth[0], color[0], visIn, depthIn, colorIn);
	}

	if (closerMask[1]) {
		swap_node(vis[1], depth[1], color[1], visIn, depthIn, colorIn);
	}

	if (closerMask[2]) {
		swap_node(vis[2], depth[2], color[2], visIn, depthIn, colorIn);
	}

	if (closerMask[3]) {
		swap_node(vis[3], depth[3], color[3], visIn, depthIn, colorIn);
	}

	if (!notValidMask[3]) {
		color[3] = packUnorm4x8(unpackUnorm4x8(color[3]) + unpackUnorm4x8(colorIn) * vis[3]);
		vis[3] *= visIn;
	}

	imageStore(colorBuffer, coord, color);
	imageStore(depthBuffer, coord, depth);
	imageStore(visibilityBuffer, coord, vis);

	memoryBarrierImage();
}

void main() {
	const vec2 texCoord = vec2(vs_out.texCoord.x, fma(mod(vs_out.texCoord.y, 1.f), 0.0625f,
				vs_out.texCoord.z));
	const vec3 normal = normalize(fma(texture(texNormal, texCoord).xyz, vec3(2.0), vec3(-1.0)));

	const vec3 viewDir = normalize(-vs_out.tangentFragPos);
	const vec3 reflectDir = reflect(-vs_out.tangentLightDir, normal);

	const float diff = max(dot(normal, vs_out.tangentLightDir), 0.0);
	const float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16) * 0.5;

	const vec3 light = sceneData.ambientColor.xyz * sceneData.ambientColor.w
			+ sceneData.sunlightColor.xyz * (diff + spec);
	
	const vec3 texColor = texture(texDiffuse, texCoord).xyz;

	const vec3 color = mix(vs_out.color.xyz, texture(skybox, reflect(vs_out.globalFragPos
			- camera.position, vs_out.TBN * normal)).xyz, vs_out.reflectance);

	// COMMIT OIT

	const ivec2 coord = ivec2(gl_FragCoord.xy);

	uint depth = encode_depth(gl_FragCoord.z);
	uint packedColor = packUnorm4x8(vec4(color * texColor * light * vs_out.color.w, 0.0));

	for (bool done = gl_HelperInvocation; !done;) {
		if (imageAtomicExchange(lock, coord, 1u) == 0u) {
			insert_node(coord, depth, 1.0 - vs_out.color.w, packedColor);
			imageStore(lock, coord, uvec4(0u));
			done = true;
		}
	}
}


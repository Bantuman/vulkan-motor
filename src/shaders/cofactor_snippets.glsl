
vec3 minor3(in vec3 r0, in vec3 r1, in vec3 r2) {
	return r0 * (r1.yxx * r2.zzy - r2.yxx * r1.zzy);
	//return r0 * cross(r1, r2);
}

vec4 minor4(in mat4 src, int r0, int r1, int r2) {
	const mat3x4 r = transpose(mat4x3(
		minor3(src[r0].yzw, src[r1].yzw, src[r2].yzw),
		minor3(src[r0].xzw, src[r1].xzw, src[r2].xzw),
		minor3(src[r0].xyw, src[r1].xyw, src[r2].xyw),
		minor3(src[r0].xyz, src[r1].xyz, src[r2].xyz)
	));

	return r[0] - r[1] + r[2];
}

mat4 fast_cofactor(in mat4 src) {
	return mat4(
		minor4(src, 1, 2, 3) * vec4(1, -1, 1, -1),
		minor4(src, 0, 2, 3) * vec4(-1, 1, -1, 1),
		minor4(src, 0, 1, 3) * vec4(1, -1, 1, -1),
		minor4(src, 0, 1, 2) * vec4(-1, 1, -1, 1)
	);
}

vec3 minor(in vec3 r0, in vec3 r1, in vec3 r2) {
	return r0 * (r1.yxx * r2.zzy - r2.yxx * r1.zzy);
	//return r0 * cross(r1, r2);
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


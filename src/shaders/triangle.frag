#version 450

layout (location = 0) in vec2 texCoord;

layout (location = 0) out vec4 outColor;

//layout (set = 0, binding = 4) uniform sampler2D tex0;
layout (set = 0, binding = 0) uniform samplerCube tex0;

vec2 normal_encode(vec3 n) {
	const vec2 enc = normalize(n.xy) * sqrt(fma(-n.z, 0.5, 0.5));
	return fma(enc, vec2(0.5), vec2(0.5));
}

vec3 normal_decode(vec4 enc) {
	vec4 nn = fma(enc, vec4(2.0, 2.0, 0.0, 0.0), vec4(-1.0, -1.0, 0.0, -1.0));
	const float l = dot(nn.xyz, -nn.xyw);

	nn.z = l;
	nn.xy *= sqrt(l);

	return fma(nn.xyz, vec3(2.0), vec3(0.0, 0.0, -1.0));
}

void main() {
	vec3 texValue = texture(tex0, vec3(texCoord, 1)).xyz;
	outColor = vec4(texValue, 1.0);
}


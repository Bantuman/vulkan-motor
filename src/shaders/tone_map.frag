#version 450

layout (location = 0) out vec4 outColor;

//layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput inputColor;
layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInputMS inputColor;

vec3 rgb2hsv(vec3 c) {
	vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
	vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
	vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

	float d = q.x - min(q.w, q.y);
	float e = 1.0e-10;
	return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c) {
	vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
	vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
	return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
	vec4 inColor = (subpassLoad(inputColor, 0) + subpassLoad(inputColor, 1)
			+ subpassLoad(inputColor, 2) + subpassLoad(inputColor, 3)) * 0.25;
	//vec4 inColor = subpassLoad(inputColor);

	//outColor = vec4(pow(subpassLoad(inputColor).rgb, vec3(2.2)), 1.0);
	outColor = vec4(inColor.rgb, 1.0);
	//vec3 color = subpassLoad(inputColor).rgb;
	//color = mix(vec3(0.5), color, 1.1); // contrast
	//color += vec3(0.05); // brightness
	//color = vec3(1.0) - exp(-color * 1.5); // exposure
	
	//vec3 hsv = rgb2hsv(color);
	// hsv.x += hueShift / 360.0;
	// hsv.yz = clamp(hsv.yz * satAndValueModifiers, vec2(0.0), vec2(1.0));
	
	//hsv.y *= 3.0;

	//color = hsv2rgb(hsv);

	//outColor = vec4(color, 1.0);
}


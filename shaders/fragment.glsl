#version 130

flat in uint frag_polarity;
in vec3 frag_v;

out vec4 frag_color;

uniform uint max_time;

#define USE_COLORS false 

void main() {
	float z = frag_v.z;
	z = (z + 1.0) / 2.0;
	z = z * z * z;

	if (!USE_COLORS || (USE_COLORS && frag_polarity == 1u)) {
		frag_color = z * 0.8 * vec4(0.1, 0.3, 0.7, 1.0);
	}
	else {
		frag_color = z * 0.8 * vec4(0.7, 0.3, 0.1, 1.0);
	}
}


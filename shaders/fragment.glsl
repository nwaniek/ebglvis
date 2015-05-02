#version 130

in vec3 frag_v;
in float frag_polarity;

out vec4 frag_color;

void main() {
	// float polarity = (frag_v[0] + 1.0) / 2.0;
	float polarity = frag_polarity;
	if (polarity < 0.5) {
		frag_color = 0.3 * vec4(0.1, 0.3, 0.7, 1.0);
	}
	else {
		frag_color = 0.3 * vec4(0.7, 0.3, 0.1, 1.0);
	}
}


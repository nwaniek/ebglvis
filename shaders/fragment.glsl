#version 130

flat in uint frag_polarity;

out vec4 frag_color;

uniform uint max_time;

void main() {
	if (frag_polarity == 1u) {
		frag_color = 0.3 * vec4(0.1, 0.3, 0.7, 1.0);
	}
	else {
		frag_color = 0.3 * vec4(0.7, 0.3, 0.1, 1.0);
	}
}


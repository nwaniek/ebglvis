#version 130

in vec4 vp;
out vec4 frag_color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

varying vec4 model_v;
varying vec4 world_v;

void main() {
	// nice color:
	float d = (model_v[2] + 1.0) / 2.0;
	frag_color = 0.3 * vec4(0.1, 0.3, 0.7, 1.0);
	//frag_color = 0.3 * vec4(0.7, 0.3, 0.1, 1.0);
}


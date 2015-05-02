#version 130

in float polarity;
in vec3 v;

// out vec3 frag_v;
out float frag_polarity;

uniform mat4 mvp;

void main() {
	// transform vp to homogeneous coordinate
	vec4 vh = vec4(v, 1.0);
	gl_Position = mvp * vh; 
	gl_PointSize = 1.0;
	frag_polarity = 0.0001 * polarity; // (v[0] + 1.0) / 2.0 * 1.0 * polarity;
	// frag_v = v;
}


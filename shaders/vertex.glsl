#version 130

in uint polarity;
in vec2 v;
in uint t;

flat out uint frag_polarity;
out vec3 frag_v;

uniform uint max_time;
uniform mat4 mvp;

void main() {
	// transform vp to homogeneous coordinate
	float z = -2.0 * float(t) / float(max_time) + 1.0;

	vec4 vh = vec4(v, z, 1.0);
	frag_v = vec3(v, z);

	gl_Position = mvp * vh; 
	gl_PointSize = 1.5;
	frag_polarity = polarity;
}

#version 130

in uint polarity;
in vec2 v;
in uint t;

flat out uint color_id;
out vec3 frag_v;

uniform uint dvs_size;
uniform uint max_time;
uniform mat4 mvp;

void main() {
	// transform vp to homogeneous coordinate
	float z = -2.0 * float(t) / float(max_time) + 1.0;

	// transform the DVS coordinates + time into cube-coordinates
	vec4 vh = vec4(
		2.0f * float(v.y) / float(dvs_size) - 1.0f,
		2.0f * float(v.x) / float(dvs_size) - 1.0f,
		z, 
		1.0);
	frag_v = vec3(vh.x, vh.y, z);

	gl_Position = mvp * vh; 
	gl_PointSize = 1.5;
	color_id = polarity;
}

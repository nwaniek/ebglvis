#version 130

flat in uint color_id;
in vec3 frag_v;

out vec4 frag_color;

uniform uint dvs_size;
uniform uint max_time;

// color lookup table
vec4 colors[2] = vec4[2](
	vec4(0.1, 0.3, 0.7, 1.0),
	vec4(0.7, 0.3, 0.1, 1.0)
);


void main() {
	float z = frag_v.z;
	z = (z + 1.0) / 2.0;
	z = z * z;
	frag_color = z * 0.8 * colors[color_id];
}


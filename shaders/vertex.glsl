#version 130

in vec3 vp;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

varying vec4 model_v;
varying vec4 world_v;

void main() {
	// transform vp to homogeneous coordinate
	vec4 v = vec4(vp, 1.0);

	// apply model view matrix
	model_v = v;
	world_v = model * model_v;
	gl_Position = projection * view * world_v;
	gl_PointSize = 1.0;
}


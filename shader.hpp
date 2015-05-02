#ifndef __SHADER_HPP__BC4996ED_75D5_49E3_A657_13ABC8C38DD2
#define __SHADER_HPP__BC4996ED_75D5_49E3_A657_13ABC8C38DD2

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <cstdarg>


struct program;

struct shader {
	friend struct program;

	shader(GLuint type);
	shader(GLuint type, std::string src);
	void compile();
	void load_from_file(std::string filename);

private:
	bool compilation_successful();
	void printlog();

	GLuint _type;
	GLuint _shader;
	std::string _src;
};


struct program {
	program();
	// TODO: variadic template argument
	void link(int n, shader *s0, ...);
	void use();

	GLuint getId() { return _program; }
	GLuint getUniformLocation(std::string name);

private:
	bool link_successful();
	void printlog();

	GLuint _program;
};

#endif /* __SHADER_HPP__BC4996ED_75D5_49E3_A657_13ABC8C38DD2 */


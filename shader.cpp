#include "shader.hpp"
#include "glutil.hpp"
#include <GL/glew.h>
#include <iostream>
#include <fstream>
#include <sstream>


shader::shader(GLuint type)
: shader(type, "")
{ }

shader::shader(GLuint type, std::string src)
: _type(type), _shader(glCreateShader(_type)), _src(src)
{ }

void shader::
load_from_file(std::string filename) {
	_src.clear();
	std::ifstream f(filename, std::ios::in);
	if (f.is_open()) {
		std::cout << "I: reading shader from file" << filename << std::endl;
		std::string line;
		while (std::getline(f, line)) {
			_src += "\n" + line;
		}
		f.close();
	}
}


void shader::compile() {
	// vertex shader
	const GLchar *src = _src.c_str();
	glShaderSource(_shader, 1, &src, NULL);
	glCompileShader(_shader);
	GL_CHECK_ERROR();
	if (!compilation_successful()) {
		printf("e: shader not compiled properly\n");
		printlog();
		exit(EXIT_FAILURE);
	}
}

bool shader::
compilation_successful() {
	int status;
	glGetShaderiv(_shader, GL_COMPILE_STATUS, &status);
	return status == GL_TRUE;
}

void shader::
printlog() {
	int max_length;
	glGetShaderiv(_shader, GL_INFO_LOG_LENGTH, &max_length);
	char* const buf = new char[max_length];

	int length;
	glGetShaderInfoLog(_shader, max_length, &length, &buf[0]);
	printf("E: %s", buf);

	delete[] buf;
}

program::program()
: _program(glCreateProgram())
{ }


void program::
link(int n, shader *s0, ...) {
	glAttachShader(_program, s0->_shader);

	va_list args;
	va_start(args, s0);
	for (--n; n; n--) {
		shader *s = va_arg(args, shader*);
		glAttachShader(_program, s->_shader);
	}
	va_end(args);

	glLinkProgram(_program);
	GL_CHECK_ERROR();
	if (!link_successful()) {
		printf("E: Program could not be linked\n");
		printlog();
		exit(EXIT_FAILURE);
	}
}

bool program::
link_successful() {
	int status;
	glGetProgramiv(_program, GL_LINK_STATUS, &status);
	return status == GL_TRUE;
}

void program::
use() {
	glUseProgram(_program);
}

GLuint program::
getUniformLocation(std::string name) {
	return glGetUniformLocation(_program, name.c_str());
}

void program::
printlog() {
	int max_length;
	glGetProgramiv(_program, GL_INFO_LOG_LENGTH, &max_length);
	char* const buf = new char[max_length];

	int length;
	glGetProgramInfoLog(_program, max_length, &length, &buf[0]);
	printf("E: %s", buf);

	delete[] buf;
}

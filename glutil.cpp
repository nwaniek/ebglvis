#include "glutil.hpp"
#include <iostream>

void
gl_print_info() {
	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* version = glGetString(GL_VERSION);
	const GLubyte* shading_version = glGetString(GL_SHADING_LANGUAGE_VERSION);
	std::cout
		<< "I: Renderer: " << renderer
		<< ", Version: " << version
		<< ", Shader Version: " << shading_version
		<< std::endl;
}

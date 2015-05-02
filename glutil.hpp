#ifndef __GLUTIL_HPP__51DC4625_85DC_44A7_A7A8_4C4B1C2AC783
#define __GLUTIL_HPP__51DC4625_85DC_44A7_A7A8_4C4B1C2AC783

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstdio>


#define GL_CHECK_ERROR() gl_check_error(__LINE__, __FILE__, __FUNCTION__)

static inline void
gl_check_error(int line, const char *file, const char *func) {
	GLuint error;
	while ((error = glGetError()))
		printf("E: OpenGL Error %s %s %d, %s\n", file, func, line, gluErrorString(error));
}


void gl_print_info();




#endif /* __GLUTIL_HPP__51DC4625_85DC_44A7_A7A8_4C4B1C2AC783 */


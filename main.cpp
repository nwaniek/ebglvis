#include <iostream>
#include <cstdlib>
#include <cstddef>
#include <vector>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include "shader.hpp"
#include "glutil.hpp"

typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::milliseconds ms;


template<typename T>
T deg2rad(T d) {
	return static_cast<T>(d * M_PI / 180.0);
}


struct Event {
 	GLuint  polarity;
	GLfloat x, y;
	GLuint  t;
};

// maximal time to display in the cube
GLuint max_time = 1000;

// number of events to generate in each 'timeframe'
#define N_EVENTS  1000

// size of the dvs
#define dvs_size  128

// vector of events
std::vector<Event> events;

// vertex buffer object for the events we wish to render
GLuint vbo = 0;
GLuint vao = 0;

// shaders and program
shader *fs;
shader *vs;
program *p;


struct Camera {
	glm::vec3 pos;
	glm::vec3 target;
	glm::vec3 up;
};

// model view matrices
glm::mat4 model = glm::mat4(1.0f);
float model_rotation = 0.0;
bool model_rotation_changed = false;

glm::mat4 view = glm::lookAt(
		glm::vec3(1.2f, 1.1f, 1.4f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f));
glm::mat4 projection = glm::perspective(
		deg2rad(100.0f),
		4.0f / 3.0f,
		0.1f, 100.0f);
glm::mat4 mvp;

GLuint polarity_vert = 0;
GLuint v_vert = 1;
GLuint t_vert = 2;

void
generate_buffers() {
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glEnableVertexAttribArray(polarity_vert);
	glEnableVertexAttribArray(v_vert);
	glEnableVertexAttribArray(t_vert);

	glVertexAttribIPointer(polarity_vert, 1, GL_UNSIGNED_INT, sizeof(Event),
			reinterpret_cast<void*>(offsetof(Event, polarity)));
	GL_CHECK_ERROR();

	glVertexAttribPointer(v_vert, 2, GL_FLOAT, GL_FALSE, sizeof(Event),
			reinterpret_cast<void*>(offsetof(Event, x)));
	GL_CHECK_ERROR();

	glVertexAttribIPointer(t_vert, 1, GL_UNSIGNED_INT, sizeof(Event),
			reinterpret_cast<void*>(offsetof(Event, t)));
	GL_CHECK_ERROR();
}

void
render_points() {
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Event) * events.size(), &events[0], GL_STREAM_DRAW);

	glDisable(GL_DEPTH_TEST);
	//glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glBlendEquation(GL_FUNC_ADD);

	// draw
	glDrawArrays(GL_POINTS, 0, events.size());

	// re-enable depth sorting for everything else
	glDepthMask(GL_TRUE);
}


static void
glfw_error_callback(int error, const char *description) {
	std::cerr << "E: " << description << " (" << error << ")" << std::endl;
}


static void
glfw_key_callback(GLFWwindow *win, int key, int /*scancode*/, int action, int /*mode*/) {
	// if (action != GLFW_PRESS) return;

	switch (key) {
	case GLFW_KEY_ESCAPE:
	case GLFW_KEY_Q:
		if (action == GLFW_PRESS)
			glfwSetWindowShouldClose(win, GL_TRUE);
		break;

	case GLFW_KEY_LEFT:
		model_rotation = 0.05f;
		model_rotation_changed = true;
		break;

	case GLFW_KEY_RIGHT:
		model_rotation = -0.05f;
		model_rotation_changed = true;
		break;

	default:
		break;
	}
}


int
rand_range(int min, int max) {
	return min + (rand() % (int)(max - min + 1));
}


/*
 * TODO: receive more data from the DVS
 */
void
update_data() {
	static Time::time_point last_call_time = Time::now();
	Time::time_point now = Time::now();

	// compute the time delta
	ms dt = std::chrono::duration_cast<ms>(now - last_call_time);

	// increase age -> decrease position
	for (auto &e: events) {
		e.t += dt.count();
	}

	// remove events that are out of reach (t=0 meanse position = -1.0)
	events.erase(std::remove_if(
				events.begin(),
				events.end(),
				[](Event e){ return e.t > 1000; }),
			events.end());

	// pump data from the DVS to OpenGL
	for (size_t i = 0; i < N_EVENTS; i++) {
		Event e = {
			(unsigned)rand_range(0,1),
			// (float)i * ((float)dvs_size / (float)N_EVENTS) * (2.0f / (float)dvs_size) - 1.0f,
			2.0f * (float)rand_range(0, dvs_size-1) / (float)dvs_size - 1.0f,
			// sinf(r),
			2.0f * (float)rand_range(0, dvs_size-1) / (float)dvs_size - 1.0f,
			0,
		};
		events.push_back(e);
	}
	last_call_time = now;
}


void
destroy_data() {
	events.clear();
}


GLFWwindow*
gl_setup() {
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit()) {
		std::cerr << "E: Could not init GLFW" << std::endl;
		exit(EXIT_FAILURE);
	}

	// create a window
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	GLFWwindow *window = glfwCreateWindow(640, 480, "particles", NULL, NULL);
	if (!window) {
		std::cerr << "E: Could not create window" << std::endl;
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	// make the window's context current
	glfwMakeContextCurrent(window);

	// capture keys
	glfwSetKeyCallback(window, glfw_key_callback);
	GL_CHECK_ERROR();

	// start GLEW stuff
	glewExperimental = GL_TRUE;
	glewInit();
	GL_CHECK_ERROR();

	// print openGL information
	gl_print_info();

	glEnable(GL_PROGRAM_POINT_SIZE);
	GL_CHECK_ERROR();

	// tell OpenGL only to draw onto a pixel if the shape is closer to the viewer

	return window;
}


void
gl_destroy(GLFWwindow *window) {
	glfwDestroyWindow(window);
	glfwTerminate();
}




void
render_loop(GLFWwindow *window) {

	// get IDs to access the data
	GLuint mvpId = p->getUniformLocation("mvp");
	GLuint max_time_id = p->getUniformLocation("max_time");
	GL_CHECK_ERROR();

	while (!glfwWindowShouldClose(window)) {

		update_data();
		GL_CHECK_ERROR();

		// update the model matrix if we want to have a rotation
		if (model_rotation_changed) {
			model = glm::rotate(model, model_rotation, glm::vec3(0, 1, 0));
			model_rotation_changed = false;
		}

		mvp = projection * view * model;

		glViewport(0, 0, 640, 480);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.1f, 0.1f, 0.1f, 0.0f);

		p->use();

		// set params to shaders
		glUniformMatrix4fv(mvpId, 1, GL_FALSE, glm::value_ptr(mvp));
		glUniform1ui(max_time_id, max_time);
		render_points();

		glfwPollEvents();
		glfwSwapBuffers(window);
	}
}


int
main(int, char *[]) {
	auto window = gl_setup();

	vs = new shader(GL_VERTEX_SHADER);
	vs->load_from_file("shaders/vertex.glsl");

	fs = new shader(GL_FRAGMENT_SHADER);
	fs->load_from_file("shaders/fragment.glsl");

	p = new program();

	fs->compile();
	vs->compile();
	glBindAttribLocation(p->getId(), polarity_vert, "polarity");
	glBindAttribLocation(p->getId(), v_vert, "v");
	glBindAttribLocation(p->getId(), t_vert, "t");

	p->link(2, vs, fs);
	generate_buffers();

	render_loop(window);
	gl_destroy(window);
	destroy_data();

	delete p;
	delete vs;
	delete fs;
	return EXIT_SUCCESS;
}

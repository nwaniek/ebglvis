#include <iostream>
#include <cstdlib>
#include <cstddef>
#include <vector>
#include <algorithm>
#include <chrono>
#include <memory>
#include <cmath>
#include <cstring>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include "shader.hpp"
#include "glutil.hpp"
#include <Edvs/EventStream.hpp>

typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::milliseconds ms;
typedef std::shared_ptr<Edvs::IEventStream> stream_t;

int window_size_x = 800;
int window_size_y = 600;
bool stereo_setup = false;

void
window_size_callback(GLFWwindow * /*win*/, int width, int height)
{
	window_size_x = width;
	window_size_y = height;
}


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
GLuint max_time = 1000u;
GLuint dvs_size = 128u;

// number of events to generate in each 'timeframe'
#define N_EVENTS  1000

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
		(float)window_size_x / (float)window_size_y,
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


void read_data(stream_t *streams, unsigned id) {
	auto dvs_events = streams[id]->read();
	if (!dvs_events.empty()) {
		for (const Edvs::Event &dvs_e : dvs_events) {
			events.push_back({
					id, // dvs_e.parity ? 1u : 0u,
					(float)dvs_e.x,
					(float)dvs_e.y,
					0,
					});
		}
	}
}


void
update_data(stream_t *streams) {
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

	read_data(streams, 0);
	if (stereo_setup) read_data(streams, 1);

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
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
	GLFWwindow *window = glfwCreateWindow(window_size_x, window_size_y, "particles", NULL, NULL);
	if (!window) {
		std::cerr << "E: Could not create window" << std::endl;
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	glfwSetWindowSizeCallback(window, window_size_callback);

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
render_loop(GLFWwindow *window, stream_t *streams) {

	// get IDs to access the data
	GLuint mvpId = p->getUniformLocation("mvp");
	GLuint max_time_id = p->getUniformLocation("max_time");
	GLuint dvs_size_id = p->getUniformLocation("dvs_size");
	GL_CHECK_ERROR();

	while (!glfwWindowShouldClose(window)) {

		update_data(streams);
		GL_CHECK_ERROR();

		// update the model matrix if we want to have a rotation
		if (model_rotation_changed) {
			model = glm::rotate(model, model_rotation, glm::vec3(0, 1, 0));
			model_rotation_changed = false;
		}

		mvp = projection * view * model;

		glViewport(0, 0, window_size_x, window_size_y);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.1f, 0.1f, 0.1f, 0.0f);

		// want to use the program
		p->use();

		// set params to shaders
		glUniformMatrix4fv(mvpId, 1, GL_FALSE, glm::value_ptr(mvp));
		glUniform1ui(max_time_id, max_time);
		glUniform1ui(dvs_size_id, dvs_size);

		//
		render_points();

		glfwPollEvents();
		glfwSwapBuffers(window);
	}
}

void
print_help() {

}


int
main(int argc, char *argv[]) {
	// TODO: command line argument parsing
	std::string dvs_uri1("/dev/ttyUSB0?baudrate=4000000&htsm=0&dtsm=0");
	std::string dvs_uri2("/dev/ttyUSB1?baudrate=4000000&htsm=0&dtsm=0");

	if (argc > 1) {
		if (!strcmp("-h", argv[1])) {
			print_help();
			exit(EXIT_SUCCESS);
		}
		dvs_uri1 = std::string(argv[1]);
	}
	if (argc > 2) {
		dvs_uri2 = std::string(argv[2]);
		stereo_setup = true;
	}

	// try to open the DVS
	stream_t streams[2];
	streams[0] = Edvs::OpenEventStream(dvs_uri1);
	if (stereo_setup)
		streams[1] = Edvs::OpenEventStream(dvs_uri2);


	auto window = gl_setup();

	p = new program();

	vs = new shader(GL_VERTEX_SHADER);
	vs->load_from_file("shaders/vertex.glsl");
	vs->compile();

	fs = new shader(GL_FRAGMENT_SHADER);
	fs->load_from_file("shaders/fragment.glsl");
	fs->compile();


	glBindAttribLocation(p->getId(), polarity_vert, "polarity");
	glBindAttribLocation(p->getId(), v_vert, "v");
	glBindAttribLocation(p->getId(), t_vert, "t");

	p->link(2, vs, fs);
	generate_buffers();

	render_loop(window, streams);
	gl_destroy(window);
	destroy_data();

	delete p;
	delete vs;
	delete fs;
	return EXIT_SUCCESS;
}

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
#include <boost/program_options.hpp>

typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::microseconds us;
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
GLuint max_time = 1000000u;
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
glfw_key_callback(GLFWwindow *win, int key, int /*scancode*/, int action, int /*mode*/)
{
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


void read_data(std::vector<stream_t> *streams, unsigned id) {
	auto dvs_events = (*streams)[id]->read();
	if (!dvs_events.empty()) {
		for (const Edvs::Event &dvs_e : dvs_events) {
			events.push_back({
					(unsigned)id, // dvs_e.parity ? 1u : 0u,
					(float)dvs_e.x,
					(float)dvs_e.y,
					0,
					});
		}
	}
}


void
update_data(std::vector<stream_t> *streams) {
	static Time::time_point last_call_time = Time::now();
	Time::time_point now = Time::now();

	// compute the time delta
	us dt = std::chrono::duration_cast<us>(now - last_call_time);

	// increase age -> decrease position
	for (auto &e: events)
		e.t += dt.count();

	// remove events that are out of reach (t=0 meanse position = -1.0)
	events.erase(std::remove_if(
				events.begin(),
				events.end(),
				[](Event e){ return e.t > max_time; }),
			events.end());

	for (size_t i = 0; i < streams->size(); i++)
		read_data(streams, i);

	last_call_time = now;
}

void
destroy_buffers() {
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
}

void
destroy_data() {
	events.clear();
}


GLFWwindow*
create_gl_context() {
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

	return window;
}


void
gl_destroy(GLFWwindow *window) {
	glfwDestroyWindow(window);
	glfwTerminate();
}


void
render_loop(GLFWwindow *window, std::vector<stream_t> *streams) {

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


std::shared_ptr<std::vector<std::string>>
parse_arguments(int argc, char *argv[]) {
	typedef std::vector<std::string> vstr;

	std::shared_ptr<vstr> uris;
	uris = std::make_shared<vstr>();

	namespace po = boost::program_options;
	po::options_description desc("usage");
	std::vector<std::string> po_uris;
	desc.add_options()
		("help,h", "show the help message")
		("uris,u", po::value(&po_uris)->multitoken(), "URI(s) to event stream(s)")
		("one,o", "shortcut option to select device /dev/ttyUSB1")
		("two,t", "shortcut option to select devices /dev/ttyUSB{1,2}")
		;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	// display some help information
	if (vm.count("help")) {
		std::cout << desc << std::endl;
		exit(EXIT_SUCCESS);
	}

	// try to guess what the user wants
	if (vm.count("one"))
		uris->push_back("/dev/ttyUSB0?baudrate=4000000&htsm=0&dtsm=0");
	else if (vm.count("two")) {
		uris->push_back("/dev/ttyUSB0?baudrate=4000000&htsm=0&dtsm=0");
		uris->push_back("/dev/ttyUSB1?baudrate=4000000&htsm=0&dtsm=0");
	}
	else {
		auto it = std::next(po_uris.begin(), 0);
		std::move(po_uris.begin(), it, std::back_inserter(*uris));
	}

	if (!uris->size()) {
		std::cerr << "E: specify at least one device path, or use -o, or -t" << std::endl;
		exit(EXIT_FAILURE);
	}

	return uris;
}

int
main(int argc, char *argv[]) {

	auto uris = parse_arguments(argc, argv);
	std::vector<stream_t> streams;
	for (auto it = uris->begin(); it != uris->end(); ++it)
		streams.push_back(Edvs::OpenEventStream(*it));

	auto window = create_gl_context();

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

	render_loop(window, &streams);

	destroy_buffers();
	gl_destroy(window);
	destroy_data();

	delete p;
	delete vs;
	delete fs;
	return EXIT_SUCCESS;
}

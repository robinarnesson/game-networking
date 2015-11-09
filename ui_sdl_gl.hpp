#ifndef UI_SDL_GL_HPP_
#define UI_SDL_GL_HPP_

#include <cstdint>
#include <fstream>
#include <list>
#include <math.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "glm/gtc/matrix_transform.hpp"
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "keyboard.hpp"
#include "misc.hpp"
#include "world.hpp"

class ui_sdl_gl : public ui {
public:
  static const int WINDOW_WIDTH  = 1280;
  static const int WINDOW_HEIGHT = 800;

  ui_sdl_gl(std::string title)
    : geometry_mesh_size_(0),
      angles_last_read_ms_(misc::get_time_ms())
  {
    //
    // init SDL
    //

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
      throw std::runtime_error(std::string("error, SDL_Init(): ") + SDL_GetError());

    window_ = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

    if (!window_)
      throw std::runtime_error(std::string("error, SDL_CreateWindow(): ") + SDL_GetError());

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    context_ = SDL_GL_CreateContext(window_);

    if (!context_)
      throw std::runtime_error(std::string("error, SDL_GL_CreateContext(): ") + SDL_GetError());

    //
    // init glew
    //

    glewExperimental = true;

    if (glewInit() != GLEW_OK)
      throw std::runtime_error(std::string("error, glewInit() failed"));

    //
    // init opengl and shader program
    //

    program_id_ = create_shader_program("vertex_shader.glsl", "fragment_shader.glsl");

    if (!program_id_)
      throw std::runtime_error(std::string("error when building shader program"));

    glUseProgram(program_id_);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glCullFace(GL_BACK);
    glClearColor(0.1, 0.1, 0.1, 0);

    uni_model_id_ = glGetUniformLocation(program_id_, "model");
    uni_view_id_ = glGetUniformLocation(program_id_, "view");
    uni_projection_id_ = glGetUniformLocation(program_id_, "projection");
    uni_color_id_ = glGetUniformLocation(program_id_, "color");

    projection_matrix_ =
        glm::perspective(70.0f, (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);

    glUniformMatrix4fv(uni_projection_id_, 1, GL_FALSE, &projection_matrix_[0][0]);

    //
    // get geometry data
    //

    std::vector<GLfloat> model_vertices, model_normals;

    if(!get_geometry_data_from_file("mask.obj", model_vertices, model_normals))
      throw std::runtime_error(std::string("error when loading model geometry"));

    geometry_mesh_size_ = model_vertices.size();

    //
    // upload geometry
    //

    // create vertex array object
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // create vertex buffer objects
    GLuint vbo[2];
    glGenBuffers(2, vbo);

    // put vertices in first buffer
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, model_vertices.size() * sizeof(GLfloat), &model_vertices[0],
        GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    // put normals in second buffer
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, model_normals.size() * sizeof(GLfloat), &model_normals[0],
        GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);
  }

  ~ui_sdl_gl() {
    SDL_DestroyWindow(window_);
    SDL_GL_DeleteContext(context_);
    SDL_Quit();
  }

  virtual void poll_events() {
    events_.resize(64);

    int events_returned = SDL_PeepEvents(
        &events_.front(),
        events_.capacity(),
        SDL_GETEVENT,
        SDL_FIRSTEVENT,
        SDL_LASTEVENT);

    if (events_returned == -1) {
      INFO("error, SDL_PeepEvents(): " << SDL_GetError());
      events_returned = 0;
    }

    events_.resize(events_returned);

    // show/hide cursor
    for (auto& e : events_) {
      if (e.type != SDL_WINDOWEVENT)
        continue;

      if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
        SDL_ShowCursor(SDL_TRUE);

      if (e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
        SDL_ShowCursor(SDL_FALSE);
    }
  }

  virtual bool check_event_quit() {
    for (auto& e : events_)
      if (e.type == SDL_QUIT)
        return true;

    return false;
  }

  virtual bool check_event_button_released(keyboard::button button) {
    for (auto& e : events_) {
      if (e.type != SDL_KEYUP)
        continue;

      int code = e.key.keysym.scancode;

      for (int i = 1; i != keyboard::button::end; i <<= 1) {
        std::vector<int> codes = get_sdl_scancodes(i);

        for (auto &c : codes)
          if (code == c && i == button)
            return true;
      }
    }

    return false;
  }

  virtual int get_pressed_buttons() {
    SDL_PumpEvents();
    const Uint8* state = SDL_GetKeyboardState(nullptr);

    int pressed = 0;

    for (int i = 1; i != keyboard::button::end; i <<= 1) {
      std::vector<int> codes = get_sdl_scancodes(i);

      for (auto& c : codes)
        if (state[c])
          pressed |= i;
    }

    return pressed;
  }

  virtual void get_delta_angles(float& horizontal, float& vertical) {
    // if window is not in focus
    if (!(SDL_GetWindowFlags(window_) & SDL_WINDOW_MOUSE_FOCUS) ||
        !(SDL_GetWindowFlags(window_) & SDL_WINDOW_INPUT_FOCUS)) {
      horizontal = vertical = 0.0;
      angles_last_read_ms_ = misc::get_time_ms();

      return;
    }

    int x, y;
    SDL_GetMouseState(&x, &y);

    float delta_time = (float) (misc::get_time_ms() - angles_last_read_ms_) / 1000;
    horizontal = 0.05 * delta_time * float(WINDOW_WIDTH / 2 - x);
    vertical = 0.05 * delta_time * float(WINDOW_HEIGHT / 2 - y);

    SDL_WarpMouseInWindow(window_, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);
    angles_last_read_ms_ = misc::get_time_ms();
  }

  virtual void draw_clear() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  virtual void draw_world(world& world, uint8_t perspective_player_id, bool draw_phantoms) {
    // get player to render scene for
    boost::optional<player&> cam_player = world.get_player(perspective_player_id);

    glm::vec3 cam_position = glm::vec3(
        cam_player.get().get_x(),
        cam_player.get().get_y(),
        cam_player.get().get_z());

    glm::vec3 cam_direction(
        cos(cam_player.get().get_vert_angel()) * cos(cam_player.get().get_horz_angel()),
        sin(cam_player.get().get_vert_angel()),
        -(cos(cam_player.get().get_vert_angel()) * sin(cam_player.get().get_horz_angel())));

    // set view matrix
    glm::mat4 view = glm::lookAt(cam_position, cam_position + cam_direction, glm::vec3(0, 1, 0));

    glUniformMatrix4fv(uni_view_id_, 1, GL_FALSE, &view[0][0]);

    // draw rest of players
    for (player& p : world.get_players()) {
      if (p.get_id() == perspective_player_id)
        continue;

      // set player color
      glm::vec4 color = glm::vec4(
          (p.get_color_AABBGGRR() & 0xff) / 255.f,
          ((p.get_color_AABBGGRR() >> 8) & 0xff) / 255.f,
          ((p.get_color_AABBGGRR() >> 16) & 0xff) / 255.f,
          (draw_phantoms) ? 0.2 : 1.0);

      glUniform4fv(uni_color_id_, 1, &color[0]);

      // set model matrix
      glm::mat4 model =
        glm::translate(glm::mat4(1.0), glm::vec3(p.get_x(), p.get_y(), p.get_z())) *
        glm::rotate(glm::mat4(1.0), p.get_horz_angel(), glm::vec3(0, 1, 0)) *
        glm::rotate(glm::mat4(1.0), p.get_vert_angel(), glm::vec3(0, 0, 1));

      glUniformMatrix4fv(uni_model_id_, 1, GL_FALSE, &model[0][0]);

      // draw player
      glDrawArrays(GL_TRIANGLES, 0, geometry_mesh_size_ / 2);
    }
  }

  virtual void draw_update() {
    SDL_GL_SwapWindow(window_);
  }

private:
  std::vector<int> get_sdl_scancodes(int button) {
    switch (button) {
      case keyboard::button::up:         return std::vector<int> { SDL_SCANCODE_SPACE };
      case keyboard::button::down:       return std::vector<int> { SDL_SCANCODE_LCTRL };
      case keyboard::button::forward:    return std::vector<int> { SDL_SCANCODE_UP,
                                                                   SDL_SCANCODE_W };
      case keyboard::button::backward:   return std::vector<int> { SDL_SCANCODE_DOWN,
                                                                   SDL_SCANCODE_S };
      case keyboard::button::left:       return std::vector<int> { SDL_SCANCODE_LEFT };
      case keyboard::button::right:      return std::vector<int> { SDL_SCANCODE_RIGHT };
      case keyboard::button::step_left:  return std::vector<int> { SDL_SCANCODE_A };
      case keyboard::button::step_right: return std::vector<int> { SDL_SCANCODE_D };
      case keyboard::button::quit:       return std::vector<int> { SDL_SCANCODE_ESCAPE };
      case keyboard::button::f1:         return std::vector<int> { SDL_SCANCODE_F1 };
      case keyboard::button::f2:         return std::vector<int> { SDL_SCANCODE_F2 };
    }

    return std::vector<int>();
  }

  bool get_geometry_data_from_file(const char* file, std::vector<GLfloat>& vertices,
      std::vector<GLfloat>& normals) {
    std::ifstream file_stream(file);

    if (!file_stream.is_open()) {
      INFO("error, could not open file: " << file);
      return false;
    }

    std::vector<std::string> vertex_lines, normal_lines;

    while (file_stream.good()) {
      std::string line;
      getline(file_stream, line);

      // read all strings holding vertices and normals
      if (line[0] == 'v') {
        if (line[1] == 'n')
          normal_lines.push_back(line);
        else
          vertex_lines.push_back(line);

        continue;
      }

      // read strings holding faces and put there values into argument vectors
      if (line[0] == 'f') {
        std::vector<std::string> edges;
        boost::split(edges, line, boost::is_any_of(" ")); // split "f 1//1 2//2 4//3"

        for (size_t i = 1; i < edges.size(); i++) {
          std::vector<std::string> indices;
          boost::split(indices, edges[i], boost::is_any_of("/")); // split "1//1"

          GLfloat temp;

          std::istringstream vertex_stream(vertex_lines[stoi(indices[0]) - 1]);
          vertex_stream.ignore(32, ' '); // skip "v"
          while (vertex_stream >> temp)
            vertices.push_back(temp);

          std::istringstream normal_stream(normal_lines[stoi(indices[2]) - 1]);
          normal_stream.ignore(32, ' '); // skip "vn"
          while (normal_stream >> temp)
            normals.push_back(temp);
        }
      }
    }

    file_stream.close();

    return true;
  }

  GLuint create_shader_program(const char* vertex_file_path, const char* fragment_file_path) {
    std::string vertex_shader_code = misc::get_file_content(vertex_file_path);
    if (vertex_shader_code.empty()) {
      INFO("error, got no data from: " << vertex_file_path);
      return 0;
    }

    std::string fragment_shader_code = misc::get_file_content(fragment_file_path);
    if (fragment_shader_code.empty()) {
      INFO("error, got no data from: " << fragment_file_path);
      return 0;
    }

    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    GLint result = GL_FALSE;
    int info_log_length;

    // compile vertex shader
    char const* vertex_source_ptr = vertex_shader_code.c_str();
    glShaderSource(vertex_shader_id, 1, &vertex_source_ptr , NULL);
    glCompileShader(vertex_shader_id);

    // check vertex shader
    glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &result);
    if (result != GL_TRUE) {
      glGetShaderiv(vertex_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
      std::vector<char> msg(info_log_length);
      glGetShaderInfoLog(vertex_shader_id, info_log_length, NULL, &msg[0]);
      INFO("error, compiling vertex shader: " << &msg[0]);
      return 0;
    }

    // compile fragment shader
    char const* fragment_source_ptr = fragment_shader_code.c_str();
    glShaderSource(fragment_shader_id, 1, &fragment_source_ptr , NULL);
    glCompileShader(fragment_shader_id);

    // check fragment shader
    glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &result);
    if (result != GL_TRUE) {
      glGetShaderiv(fragment_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
      std::vector<char> msg(info_log_length);
      glGetShaderInfoLog(fragment_shader_id, info_log_length, NULL, &msg[0]);
      INFO("error, compiling fragment shader: " << &msg[0]);
      glDeleteShader(vertex_shader_id);
      return 0;
    }

    // link program
    GLuint program_id = glCreateProgram();
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    glLinkProgram(program_id);

    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // check program
    glGetProgramiv(program_id, GL_LINK_STATUS, &result);
    if (result != GL_TRUE) {
      glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &info_log_length);
      std::vector<char> msg(std::max(info_log_length, int(1)));
      glGetProgramInfoLog(program_id, info_log_length, NULL, &msg[0]);
      INFO("error, linking shaders: " << &msg[0]);
      return 0;
    }

    return program_id;
  }

  // sdl
  SDL_Window* window_;
  SDL_GLContext context_;
  std::vector<SDL_Event> events_;

  // gl
  GLuint program_id_;
  GLuint uni_model_id_;
  GLuint uni_view_id_;
  GLuint uni_projection_id_;
  GLuint uni_color_id_;

  // other
  size_t geometry_mesh_size_;
  uint64_t angles_last_read_ms_;
  glm::mat4 projection_matrix_;
};

#endif // UI_SDL_GL_HPP_

#ifndef UI_SDL_HPP_
#define UI_SDL_HPP_

#include <cstdint>
#include <math.h>
#include <stdexcept>
#include "glm/gtc/matrix_transform.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_image.h>
#include "keyboard.hpp"
#include "world.hpp"

class ui_sdl : public ui {
public:
  static const int WINDOW_WIDTH  = 500;
  static const int WINDOW_HEIGHT = 500;

  ui_sdl(std::string title) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
      throw std::runtime_error(std::string("error, SDL_Init(): ") + SDL_GetError());

    window_ = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN);

    if (!window_)
      throw std::runtime_error(std::string("error, SDL_CreateWindow(): ") + SDL_GetError());

    renderer_ = SDL_CreateRenderer(window_, 0, SDL_RENDERER_ACCELERATED);

    if (!renderer_)
      throw std::runtime_error(std::string("error, SDL_CreateRenderer(): ") + SDL_GetError());
  }

  ~ui_sdl() {
    SDL_DestroyWindow(window_);
    SDL_DestroyRenderer(renderer_);
    SDL_Quit();
  }

  void poll_events() {
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
  }

  bool check_event_quit() {
    for (auto& e : events_)
      if (e.type == SDL_QUIT)
        return true;

    return false;
  }

  bool check_event_button_released(keyboard::button button) {
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

  int get_pressed_buttons() {
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

  void get_delta_angles(float& horizontal, float& vertical) {
    horizontal = vertical = 0.0;
  }

  void draw_clear() {
    int clear_color = 0x202020ff;  // 0xRRGGBBAA

    SDL_SetRenderDrawColor(
        renderer_,
        (clear_color >> 24) & 0xff,
        (clear_color >> 16) & 0xff,
        (clear_color >> 8) & 0xff,
        clear_color & 0xff);

    SDL_RenderClear(renderer_);
  }

  void draw_world(world& world, uint8_t perspective_player_id, bool draw_phantoms) {
    // draw each player
    for (player& p : world.get_players()) {
      float a = p.get_horz_angel();

      float temp[] = {
          std::cos(a), -std::sin(a),
          std::sin(a), std::cos(a)};

      glm::mat2 rotation_matrix = glm::make_mat2(temp);

      // set triangle edges
      glm::vec2 c1 = rotation_matrix * glm::vec2(-15, -15);
      glm::vec2 c2 = rotation_matrix * glm::vec2(24, 0);
      glm::vec2 c3 = rotation_matrix * glm::vec2(-15, 15);

      int center_horz = WINDOW_WIDTH / 2;
      int center_vert = WINDOW_HEIGHT / 2;

      if (draw_phantoms) {
        trigonColor(renderer_,
            center_horz + c1.x + p.get_x() * 50, center_vert + c1.y + p.get_z() * 50,
            center_horz + c2.x + p.get_x() * 50, center_vert + c2.y + p.get_z() * 50,
            center_horz + c3.x + p.get_x() * 50, center_vert + c3.y + p.get_z() * 50,
            p.get_color_AABBGGRR());
      } else {
        filledTrigonColor(renderer_,
            center_horz + c1.x + p.get_x() * 50, center_vert + c1.y + p.get_z() * 50,
            center_horz + c2.x + p.get_x() * 50, center_vert + c2.y + p.get_z() * 50,
            center_horz + c3.x + p.get_x() * 50, center_vert + c3.y + p.get_z() * 50,
            p.get_color_AABBGGRR());
      }
    }
  }

  void draw_update() {
    SDL_RenderPresent(renderer_);
  }

private:
  std::vector<int> get_sdl_scancodes(int button) {
    switch (button) {
      case keyboard::button::forward:  return std::vector<int> { SDL_SCANCODE_UP };
      case keyboard::button::backward: return std::vector<int> { SDL_SCANCODE_DOWN };
      case keyboard::button::left:     return std::vector<int> { SDL_SCANCODE_LEFT };
      case keyboard::button::right:    return std::vector<int> { SDL_SCANCODE_RIGHT };
      case keyboard::button::quit:     return std::vector<int> { SDL_SCANCODE_ESCAPE };
      case keyboard::button::f1:       return std::vector<int> { SDL_SCANCODE_F1 };
      case keyboard::button::f2:       return std::vector<int> { SDL_SCANCODE_F2 };
    }

    return std::vector<int>();
  }

  // sdl
  SDL_Window* window_;
  SDL_Renderer* renderer_;
  std::vector<SDL_Event> events_;
};

#endif // UI_SDL_HPP_

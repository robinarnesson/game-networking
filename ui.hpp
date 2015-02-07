#ifndef UI_HPP_
#define UI_HPP_

#include <cstdint>
#include <math.h>
#define GLM_FORCE_RADIANS
#include "glm/gtc/matrix_transform.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_image.h>
#include "keyboard.hpp"
#include "world.hpp"

class ui {
public:
  static const uint32_t WINDOW_BG_COLOR = 0x272822ff; // 0xRRGGBBAA

  bool start(const std::string window_title) {
    const std::string loading_image_file_path = "loading.png";

    window_ = nullptr;
    renderer_ = nullptr;
    texture_loading_ = nullptr;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
      INFO("error, SDL_Init(): " << SDL_GetError());
      return false;
    }

    window_ = SDL_CreateWindow(window_title.c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        world::WIDTH, world::HEIGTH,
        SDL_WINDOW_SHOWN);
    if (!window_) {
      INFO("error, SDL_CreateWindow(): " << SDL_GetError());
      stop();
      return false;
    }

    renderer_ = SDL_CreateRenderer(window_, 0, SDL_RENDERER_ACCELERATED);
    if (!renderer_) {
      INFO("error, SDL_CreateRenderer(): " << SDL_GetError());
      stop();
      return false;
    }

    texture_loading_ = IMG_LoadTexture(renderer_, loading_image_file_path.c_str());
    if (!texture_loading_) {
      INFO("error, could not load image " << loading_image_file_path);
      stop();
      return false;
    }
    rect_loading_.w = 50; // image width
    rect_loading_.h = 50; // image height
    rect_loading_.x = world::WIDTH / 2 - 25; // center x
    rect_loading_.y = world::HEIGTH / 2 - 25; // center y

    return true;
  }

  void stop() {
    SDL_DestroyTexture(texture_loading_);
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
    for (const SDL_Event& e : events_)
      if (e.type == SDL_QUIT)
        return true;
    return false;
  }

  bool check_event_button_released(keyboard::button button) {
    for (const SDL_Event& e : events_)
      if (e.type == SDL_KEYUP && keyboard::get_code(button) == e.key.keysym.scancode)
          return true;
    return false;
  }

  uint32_t get_pressed_buttons() {
    // get keyboard state
    SDL_PumpEvents();
    const Uint8* state = SDL_GetKeyboardState(nullptr);

    // get pressed keys
    uint32_t buttons = 0;
    for (uint32_t i=1; i<keyboard::button::last; i<<=1)
      buttons |= state[keyboard::get_code(static_cast<keyboard::button>(i))] ? i : 0;

    return buttons;
  }

  void draw_loading() {
    // draw background
    set_render_draw_background_color();
    SDL_RenderClear(renderer_);

    // draw loading image
    SDL_RenderCopy(renderer_, texture_loading_, nullptr, &rect_loading_);
    SDL_RenderPresent(renderer_);
  }

  void draw_world(world& world, bool draw_player_edges_only) {
    // draw background
    if (!draw_player_edges_only) {
      set_render_draw_background_color();
      SDL_RenderClear(renderer_);
    }

    // draw each player
    for (player& p : world.get_players()) {
      // make rotation matrix
      float a = p.get_angel();
      float temp[] = {
          std::cos(a), -std::sin(a),
          std::sin(a),  std::cos(a)};
      glm::mat2 rotation_matrix = glm::make_mat2(temp);

      // set triangle edges
      glm::vec2 c1 = rotation_matrix * glm::vec2(-15, 15);
      glm::vec2 c2 = rotation_matrix * glm::vec2(15, 15);
      glm::vec2 c3 = rotation_matrix * glm::vec2(0, -24);

        // draw player as triangle
      if (draw_player_edges_only) {
        trigonColor(renderer_,
            c1.x + p.get_x(), c1.y + p.get_y(),
            c2.x + p.get_x(), c2.y + p.get_y(),
            c3.x + p.get_x(), c3.y + p.get_y(),
            p.get_color_AABBGGRR());
      } else {
        filledTrigonColor(renderer_,
            c1.x + p.get_x(), c1.y + p.get_y(),
            c2.x + p.get_x(), c2.y + p.get_y(),
            c3.x + p.get_x(), c3.y + p.get_y(),
            p.get_color_AABBGGRR());
      }
    }

    SDL_RenderPresent(renderer_);
  }

private:
  void set_render_draw_background_color() {
    SDL_SetRenderDrawColor(renderer_,
        (WINDOW_BG_COLOR >> 24) & 0xff,
        (WINDOW_BG_COLOR >> 16) & 0xff,
        (WINDOW_BG_COLOR >> 8) & 0xff,
        WINDOW_BG_COLOR & 0xff);
  }

  SDL_Window* window_;
  SDL_Renderer* renderer_;
  SDL_Texture* texture_loading_;
  SDL_Rect rect_loading_;
  std::vector<SDL_Event> events_;
};

#endif // UI_HPP_
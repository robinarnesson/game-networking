#ifndef KEYBOARD_HPP_
#define KEYBOARD_HPP_

#include <SDL2/SDL.h>

namespace keyboard {
  enum button {
    up     = 0x00000001,
    down   = 0x00000002,
    left   = 0x00000004,
    right  = 0x00000008,
    escape = 0x00000010,
    n1     = 0x00000020, // number key 1
    n2     = 0x00000040, // number key 2
    last   = 0x00000080  // enum terminator
  };

  int get_code(keyboard::button button) {
    switch (button) {
      case keyboard::button::up:
        return SDL_SCANCODE_UP;
        break;
      case keyboard::button::down:
        return SDL_SCANCODE_DOWN;
        break;
      case keyboard::button::left:
        return SDL_SCANCODE_LEFT;
        break;
      case keyboard::button::right:
        return SDL_SCANCODE_RIGHT;
        break;
      case keyboard::button::escape:
        return SDL_SCANCODE_ESCAPE;
        break;
      case keyboard::button::n1:
        return SDL_SCANCODE_1;
        break;
      case keyboard::button::n2:
        return SDL_SCANCODE_2;
        break;
      default:
        return 0;
        break;
    }
  }
}

#endif // KEYBOARD_HPP_
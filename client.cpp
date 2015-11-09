#include <iostream>
#include "client.hpp"
#include "ui_sdl.hpp"
#include "ui_sdl_gl.hpp"
#include "misc.hpp"

int main(int argc, char const *argv[]) {
  if (argc < 4 || !misc::is_number(argv[2]) || (strcmp(argv[3], "2d") && strcmp(argv[3], "3d"))) {
    std::cout << "Usage: " << argv[0] << " <host> <port> 2d|3d" << std::endl << std::endl;
    std::cout << "2d-controls: " << std::endl;
    std::cout << "  Move around with the arrow keys" << std::endl;
    std::cout << "3d-controls: " << std::endl;
    std::cout << "  W: Move forward" << std::endl;
    std::cout << "  S: Move backward" << std::endl;
    std::cout << "  A: Step left" << std::endl;
    std::cout << "  D: Step right" << std::endl;
    std::cout << "  Space: Move up" << std::endl;
    std::cout << "  Left ctrl: Move down" << std::endl;
    std::cout << "  Mouse look" << std::endl;
    std::cout << "Other: " << std::endl;
    std::cout << "  F1: Toggle debug mode" << std::endl;
    std::cout << "  F2: Toggle prediction and interpolation" << std::endl;
    std::cout << "  Escape: Quit" << std::endl;

    return 1;
  }

  std::string title(std::string("client program, ") + argv[1] + ":" + argv[2]);

  try {
    ui* interface;

    if (!strcmp(argv[3], "3d"))
      interface = new ui_sdl_gl(title); // 3d
    else
      interface = new ui_sdl(title); // 2d

    client(argv[1], argv[2], *interface).join_game();

    delete interface;
  } catch (std::exception& e) {
    std::cerr << "exception: " << e.what() << std::endl;

    return 2;
  }

  return 0;
}

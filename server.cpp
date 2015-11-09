#include <iostream>
#include <string>
#include "server.hpp"
#include "misc.hpp"

int main(int argc, char const *argv[]) {
  if (argc < 2 || !misc::is_number(argv[1])) {
    std::cout << "Usage: " << argv[0] << " <port>" << std::endl;

    return 1;
  }

  try {
    server s(std::stoi(argv[1]));
    std::cin.get(); // exit on key pressed
  } catch (std::exception& e) {
    std::cerr << "exception: " << e.what() << std::endl;

    return 2;
  }

  return 0;
}
